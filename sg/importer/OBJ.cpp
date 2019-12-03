// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include "Importer.h"
// tiny_obj_loader
#include "tiny_obj_loader.h"
// ospcommon
#include "ospcommon/os/FileName.h"

namespace ospray::sg {

  struct OBJImporter : public Importer
  {
    OBJImporter()           = default;
    ~OBJImporter() override = default;

    void importScene() override;
  };

  OSP_REGISTER_SG_NODE_NAME(OBJImporter, importer_obj);

  // Helper types /////////////////////////////////////////////////////////////

  struct OBJData
  {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
  };

  // Helper functions /////////////////////////////////////////////////////////

  static OBJData loadFromFile(FileName fileName)
  {
    const std::string containingPath = fileName.path();
    // Try to load scene without auto-triangulation.  If point, lines,
    // or polygons are found; reload the scene as a triangle mesh.
    bool ret         = false;
    bool needsReload = false;

    do {
      OBJData retval;
      std::string warn;
      std::string err;

      ret = tinyobj::LoadObj(&retval.attrib,
                             &retval.shapes,
                             &retval.materials,
                             &warn,
                             &err,
                             fileName.c_str(),
                             containingPath.c_str(),
                             needsReload);  // triangulate meshes if true

      auto numQuads     = 0;
      auto numTriangles = 0;
      needsReload       = false;
      for (auto &shape : retval.shapes) {
        for (auto numVertsInFace : shape.mesh.num_face_vertices) {
          numTriangles += (numVertsInFace == 3);
          numQuads += (numVertsInFace == 4);

          if (numVertsInFace < 3) {
            std::cerr << "Warning: less than 3 verts in face!\n"
                      << "         Lines and points not supported.\n";
            needsReload = true;
          }
          if (numVertsInFace > 4) {
            std::cerr << "Warning: more than 4 verts in face!\n"
                      << "         Polygons not supported.\n";
            needsReload = true;
            break;
          }
        }
      }

      if (needsReload) {
        std::cerr << "         Reloading as a triangle mesh.\n";
      } else {
        std::cout << "... found " << numTriangles << " triangles "
                  << "and " << numQuads << " quads.\n";

        if (!err.empty()) {
          std::cerr << "#ospsg: obj parsing warning(s)...\n"
                    << err << std::endl;
        }
        return retval;
      }
    } while (needsReload);

    return {};
  }

  // OBJImporter definitions //////////////////////////////////////////////////

  void OBJImporter::importScene()
  {
    auto file    = FileName(child("file").valueAs<std::string>());
    auto objData = loadFromFile(file);

    auto &attrib = objData.attrib;

    int shapeId = 0;

    std::string baseName = file.name() + '_';

    for (auto &shape : objData.shapes) {
      auto numSrcIndices = shape.mesh.indices.size();

      std::vector<vec3f> v;
      std::vector<vec4ui> vi;
      std::vector<vec3f> vn;
      std::vector<vec2f> vt;

      v.reserve(numSrcIndices);
      vi.reserve(numSrcIndices);
      vn.reserve(numSrcIndices);
      vt.reserve(numSrcIndices);

      // OSPRay doesn't support separate arrays for vertex, normal & texcoord
      // indices.  So, reindex by creating a single index array then push_back
      // attribs according to each of their own index arrays.
      // Put all indices into a single vec4.  Triangles duplicate the last
      // index.
      size_t i = 0;
      for (int numVertsInFace : shape.mesh.num_face_vertices) {
        auto isQuad = (numVertsInFace == 4);
        // when a Quad then use same splitting diagonale in OSPRay/Embree as
        // tinyOBJ would use
        auto prim_indices = isQuad ? vec4ui(3, 0, 1, 2) : vec4ui(0, 1, 2, 2);
        vi.push_back(i + prim_indices);
        i += numVertsInFace;
      }

      for (size_t i = 0; i < numSrcIndices; i++) {
        auto idx = shape.mesh.indices[i];

        v.emplace_back(&attrib.vertices[idx.vertex_index * 3]);

        // TODO create missing normals&texcoords if only some faces have them
        if (!attrib.normals.empty() && idx.normal_index != -1)
          vn.emplace_back(&attrib.normals[idx.normal_index * 3]);

        if (!attrib.texcoords.empty() && idx.texcoord_index != -1)
          vt.emplace_back(&attrib.texcoords[idx.texcoord_index * 2]);
      }

      auto name  = baseName + std::to_string(shapeId++) + '_' + shape.name;
      auto &mesh = createChild(name, "geometry_triangles");

      mesh.createChildData("vertex.position", v);
      mesh.createChildData("index", vi);
      if (!vn.empty())
        mesh.createChildData("vertex.normal", vn);
      if (!vt.empty())
        mesh.createChildData("vertex.texcoord", vt);
    }

    std::cout << "...finished import!\n";
  }

}  // namespace ospray::sg
