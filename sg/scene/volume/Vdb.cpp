// ======================================================================== //
// Copyright 2009-2020 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "Vdb.h"
#include <sstream>
#include <string>

namespace ospray::sg {

  // VdbVolume definitions /////////////////////////////////////////////////////

  VdbVolume::VdbVolume() : Volume("vdb")
  {
  }

#if USE_OPENVDB
#define VKL_VDB_NUM_LEVELS 4
  /*
   * This builder can traverse the OpenVDB tree, generating
   * nodes for us along the way.
   */
  template <typename VdbNodeType, typename Enable = void>
  struct Builder
  {
    using ChildNodeType = typename VdbNodeType::ChildNodeType;
    static constexpr uint32_t level{VKL_VDB_NUM_LEVELS - 1 -
                                    VdbNodeType::LEVEL};
    static constexpr uint32_t nextLevel{level + 1};

    static void visit(const VdbNodeType &vdbNode,
                      std::vector<float> &tiles,
                      std::vector<uint32_t> &bufLevel,
                      std::vector<vec3i> &bufOrigin,
                      std::vector<cpp::SharedData> &bufData)
    {
      for (auto it = vdbNode.cbeginValueOn(); it; ++it) {
        const auto &coord = it.getCoord();
        bufLevel.emplace_back(nextLevel);
        bufOrigin.emplace_back(coord[0], coord[1], coord[2]);
        // Note: we can use a pointer to a value we just pushed back because
        //       we preallocated using reserve!
        tiles.push_back(*it);
        bufData.emplace_back(&tiles.back(), 1ul);
      }

      for (auto it = vdbNode.cbeginChildOn(); it; ++it)
        Builder<ChildNodeType>::visit(*it, tiles, bufLevel, bufOrigin, bufData);
    }
  };

  template <typename VdbNodeType, class Enable>
  constexpr uint32_t Builder<VdbNodeType, Enable>::nextLevel;

  /*
   * We stop the recursion one level above the leaf level.
   * By default, this is level 2.
   */
  template <typename VdbNodeType>
  struct Builder<VdbNodeType,
                 typename std::enable_if<(VdbNodeType::LEVEL == 1)>::type>
  {
    static constexpr uint32_t level{VKL_VDB_NUM_LEVELS - 1 -
                                    VdbNodeType::LEVEL};
    static constexpr uint32_t nextLevel{level + 1};
    static_assert(nextLevel == VKL_VDB_NUM_LEVELS - 1,
                  "OpenVKL is not compiled to match OpenVDB::FloatTree");

    static void visit(const VdbNodeType &vdbNode,
                      std::vector<float> &tiles,
                      std::vector<uint32_t> &bufLevel,
                      std::vector<vec3i> &bufOrigin,
                      std::vector<cpp::SharedData> &bufData)
    {
      const uint32_t storageRes = 16;
      const uint32_t childRes   = 8;
      const auto *vdbVoxels     = vdbNode.getTable();
      const vec3i origin =
          vec3i(vdbNode.origin()[0], vdbNode.origin()[1], vdbNode.origin()[2]);

      // Note: OpenVdb stores data in z-major order!
      uint64_t vIdx = 0;
      for (uint32_t x = 0; x < storageRes; ++x)
        for (uint32_t y = 0; y < storageRes; ++y)
          for (uint32_t z = 0; z < storageRes; ++z, ++vIdx) {
            const bool isTile  = vdbNode.isValueMaskOn(vIdx);
            const bool isChild = vdbNode.isChildMaskOn(vIdx);

            if (!(isTile || isChild))
              continue;

            const auto &nodeUnion   = vdbVoxels[vIdx];
            const vec3i childOrigin = origin + childRes * vec3i(x, y, z);
            if (isTile) {
              bufLevel.emplace_back(nextLevel);
              bufOrigin.emplace_back(childOrigin);
              // Note: we can use a pointer to a value we just pushed back because
              //       we preallocated using reserve!
              tiles.push_back(nodeUnion.getValue());
              bufData.emplace_back(&tiles.back(), 1ul);
            } else if (isChild) {
              bufLevel.emplace_back(nextLevel);
              bufOrigin.emplace_back(childOrigin);
              bufData.emplace_back(nodeUnion.getChild()->buffer().data(),
                                   vec3ul(childRes));
            }
          }
    }
  };

  template <typename VdbNodeType>
  constexpr uint32_t Builder<VdbNodeType,
                 typename std::enable_if<(VdbNodeType::LEVEL == 1)>::type>::nextLevel;

#endif  // USE_OPENVDB

  ///! \brief file name of the xml doc when the node was loaded from xml
  /*! \detailed we need this to properly resolve relative file names */
  void VdbVolume::load(const FileName &fileNameAbs)
  {
#if USE_OPENVDB
    if (!fileLoaded) {
      openvdb::initialize();  // Must initialize first! It's ok to do this
                              // multiple times.

      try {
        openvdb::io::File file(fileNameAbs.c_str());
        std::cout << "loading " << fileNameAbs << std::endl;
        file.open();
        grid = file.readGrid("density");
        file.close();
      } catch (const std::exception &e) {
        throw std::runtime_error(e.what());
      }

      // We only support the default topology in this loader.
      if (grid->type() != std::string("Tree_float_5_4_3"))
        throw std::runtime_error(std::string("Incorrect tree type: ") +
                                 grid->type());

      // Preallocate memory for all leaves; this makes loading the tree much
      // faster.
      openvdb::FloatGrid::Ptr vdb =
          openvdb::gridPtrCast<openvdb::FloatGrid>(grid);

      const auto &indexToObject = grid->transform().baseMap();
      if (!indexToObject->isLinear())
        throw std::runtime_error(
            "OpenVKL only supports linearly transformed volumes");

      // Transpose; OpenVDB stores column major (and a full 4x4 matrix).
      const auto &ri2o = indexToObject->getAffineMap()->getMat4();
      const auto *i2o  = ri2o.asPointer();
      std::vector<float> bufI2o = {static_cast<float>(i2o[0]),
                                   static_cast<float>(i2o[4]),
                                   static_cast<float>(i2o[8]),
                                   static_cast<float>(i2o[1]),
                                   static_cast<float>(i2o[5]),
                                   static_cast<float>(i2o[9]),
                                   static_cast<float>(i2o[2]),
                                   static_cast<float>(i2o[6]),
                                   static_cast<float>(i2o[10]),
                                   static_cast<float>(i2o[12]),
                                   static_cast<float>(i2o[13]),
                                   static_cast<float>(i2o[14])};
      createChildData("indexToObject", bufI2o);

      // Preallocate buffers!
      const size_t numTiles  = vdb->tree().activeTileCount();
      const size_t numLeaves = vdb->tree().leafCount();
      const size_t numNodes  = numTiles + numLeaves;

      std::vector<uint32_t> level;
      std::vector<vec3i> origin;
      std::vector<cpp::SharedData> data;
      level.reserve(numNodes);
      origin.reserve(numNodes);
      data.reserve(numNodes);
      tiles.reserve(numTiles);

      const auto &root = vdb->tree().root();
      for (auto it = root.cbeginChildOn(); it; ++it)
        Builder<openvdb::FloatTree::RootNodeType::ChildNodeType>::visit(
            *it, tiles, level, origin, data);

      createChildData("node.level", level);
      createChildData("node.origin", origin);
      createChildData("node.data", data);
      fileLoaded = true;
    }
#endif  // USE_OPENVDB
  }

  OSP_REGISTER_SG_NODE_NAME(VdbVolume, volume_vdb);
}  // namespace ospray::sg
