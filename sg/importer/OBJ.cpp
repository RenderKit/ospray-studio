// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Importer.h"
// tiny_obj_loader
#include "tiny_obj_loader.h"
// rkcommon
#include "rkcommon/os/FileName.h"

namespace ospray {
  namespace sg {

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

  static inline void parseParameterString(std::string typeAndValueString,
                                          std::string &paramType,
                                          rkcommon::utility::Any &paramValue)
  {
    std::stringstream typeAndValueStream(typeAndValueString);
    std::string paramValueString;
    getline(typeAndValueStream, paramValueString);

    std::vector<float> floats;
    std::stringstream valueStream(typeAndValueString);
    float val;
    while (valueStream >> val)
      floats.push_back(val);

    if (floats.size() == 1) {
      paramType  = "float";
      paramValue = floats[0];
    } else if (floats.size() == 2) {
      paramType  = "vec2f";
      paramValue = vec2f(floats[0], floats[1]);
    } else if (floats.size() == 3) {
      paramType  = "vec3f";
      paramValue = vec3f(floats[0], floats[1], floats[2]);
    } else {
      // Unknown type.
      paramValue = typeAndValueString;
    }
  }

  std::shared_ptr<sg::Texture2D> createSGTex(const std::string &map_name,
                                             const FileName &texName,
                                             const FileName &containingPath,
                                             const bool preferLinear  = false,
                                             const bool nearestFilter = false)
  {
    std::shared_ptr<sg::Texture2D> sgTex =
        std::static_pointer_cast<sg::Texture2D>(
            sg::createNode(map_name, "texture_2d"));

    auto &tex2D = *sgTex;
    sgTex->load(containingPath + texName, preferLinear, nearestFilter);

    return sgTex;
  }

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
        // If already planning to reload, skip next loop
        if (needsReload)
          break;
        for (auto numVertsInFace : shape.mesh.num_face_vertices) {
          numTriangles += (numVertsInFace == 3);
          numQuads += (numVertsInFace == 4);

          if (numVertsInFace < 3) {
            std::cerr << "Warning: less than 3 verts in face!\n"
                      << "         Lines and points not supported.\n";
            needsReload = true;
            break;
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

  static std::vector<NodePtr> createMaterials(const OBJData &objData,
                                              FileName fileName)
  {
    std::vector<NodePtr> retval;
    const std::string containingPath = fileName.path();

    for (const auto &m : objData.materials) {
      std::vector<NodePtr> paramNodes;
      std::string matType{"obj"};
      // first check the type and parameters for this material.
      // our full parameter names and material types get caught in tinyobj's
      // unknown_parameter list. e.g. "anisotropy" instead of "aniso"
      for (auto &param : m.unknown_parameter) {
        auto paramName     = param.first;
        auto paramValueStr = param.second;

        // Parse OSPRay 1.8.5 material 'type' names.  Material name
        // capitalization has changes between v1.8 and v2.x
        if (paramName == "type") {
          const static std::map<std::string, std::string> TypeConvertMap = {
              // Old name, new name
              {"OBJMaterial", "obj"},
              {"Principled", "principled"},
              {"CarPaint", "carPaint"},
              {"Metal", "metal"},
              {"Alloy", "alloy"},
              {"Glass", "glass"},
              {"ThinGlass", "thinGlass"},
              {"MetallicPaint", "metallicPaint"},
              {"Luminous", "luminous"}};
          auto s = TypeConvertMap.find(paramValueStr);
          if (s != TypeConvertMap.end())
            paramValueStr = s->second;
        } else {
          // Parse OSPRay 1.8.5 material '*Map[.]' parameter names
          // In materials other than OBJ, textures were specified in the form
          // <name>Map[.transform]. Rename them to map_<name>[.transform]
          // ie: cut "Map" out and prepend with "map_"
          auto pos = paramName.find("Map");
          if (pos != std::string::npos)
            paramName = "map_" + paramName.erase(pos, 3);
        }

        if (paramName == "type") {
          if (paramValueStr != "obj")
            matType = paramValueStr;
        } else {
          // this line is declaring a parameter for this material
          std::string paramType;

          // Only process the texture name here
          // map_<name> with no [.<transform>]
          if (paramName.find("map_") != std::string::npos &&
              paramName.find(".") == std::string::npos) {
            bool preferLinear = false;
            bool nearestFilter =
                (paramName.find("rotation") != std::string::npos) ||
                (paramName.find("Rotation") != std::string::npos);

            auto map_misc = createSGTex(paramName,
                                        paramValueStr,
                                        containingPath,
                                        preferLinear,
                                        nearestFilter);
            paramNodes.push_back(map_misc);

          } else {
            rkcommon::utility::Any paramValue;
            parseParameterString(paramValueStr, paramType, paramValue);
            // Principled::thin is the only 'bool' material parameter
            // but, needs to be treated with a "bool" paramType
            if (paramName == "thin") {
              paramType = "bool";
              paramValue = paramValue == 0 ? false : true;
            } else if ((paramName.find("Color") != std::string::npos
                           || paramName.find("color") != std::string::npos)
                && paramType == "vec3f") {
              // rgb type allows for ImGui color editor
              paramType = "rgb";
            }

            try {
              auto newParam = createNode(paramName, paramType, paramValue);
              paramNodes.push_back(newParam);
            } catch (const std::runtime_error &) {
              // NOTE(jda) - silently move on if parsed node type doesn't
              // exist maybe it's a texture, try it
              std::cout << "attempting to load param as texture: " << paramName
                        << " " << paramValueStr << std::endl;
              auto map_misc =
                  createSGTex(paramName, paramValueStr, containingPath);
              paramNodes.push_back(map_misc);
            }
          }
        }
      }

      // actually create a material of the given type
      // and provide it the parsed parameters
      if (matType == "obj") {
        auto objMatNode = createNode(fileName.name() + "::" + m.name, "obj");
        auto &mat       = *objMatNode;

        mat.createChild("kd", "rgb", vec3f(m.diffuse));
        mat.createChild("ks", "rgb", vec3f(m.specular));
        mat.createChild("ns", "float", m.shininess);
        mat.createChild("d", "float", m.dissolve);

        // keeping texture names consistent with ospray's; lowercase snakecase
        // ospray documentation inconsistent
        if (!m.diffuse_texname.empty()) {
          mat.add(
              createSGTex("map_kd", m.diffuse_texname, containingPath, true));
        }

        if (!m.specular_texname.empty()) {
          mat.add(
              createSGTex("map_ks", m.specular_texname, containingPath, true));
        }

        if (!m.specular_highlight_texname.empty()) {
          mat.add(createSGTex(
              "map_ns", m.specular_highlight_texname, containingPath, true));
        }

        if (!m.bump_texname.empty()) {
          mat.add(
              createSGTex("map_bump", m.bump_texname, containingPath, true));
        }

        if (!m.alpha_texname.empty()) {
          mat.add(createSGTex("map_d", m.alpha_texname, containingPath, true));
        }
        for (auto &param : paramNodes)
          mat.add(param);

        retval.push_back(objMatNode);
      } else {
        auto matNode = createNode(fileName.name() + "::" + m.name, matType);
        auto &mat    = *matNode;
        for (auto &param : paramNodes)
          mat.add(param);
        retval.push_back(matNode);
      }
    }

    return retval;
  }

  // OBJImporter definitions /////////////////////////////////////////////

  void OBJImporter::importScene()
  {
    // Create a root Transform/Instance off the Importer, under which to build
    // the import hierarchy
    std::string baseName = fileName.name() + "_rootXfm";
    auto rootNode = createNode(baseName, "Transform", affine3f{one});

    auto objData = loadFromFile(fileName);

    auto materialNodes = createMaterials(objData, fileName);
    // If the model provided no materials, create a default
    if (materialNodes.empty())
      materialNodes.emplace_back(createNode("default", "obj"));

    size_t baseMaterialOffset = materialRegistry->children().size();

    for (auto m : materialNodes) {
      materialRegistry->add(m);
      materialRegistry->matImportsList.push_back(m->name());
    }

    auto &attrib = objData.attrib;

    int shapeId = 0;

    for (auto &shape : objData.shapes) {
      auto numSrcIndices = shape.mesh.indices.size();

      // Mesh indices.size == 0 indicates a non-mesh primitive
      // (points, lines, curves and surfaces)
      if (numSrcIndices == 0)
        continue;

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

      auto name = std::to_string(shapeId++) + '_' + shape.name;

      auto &mesh = rootNode->createChild(name, "geometry_triangles");

      std::vector<uint32_t> mIDs(shape.mesh.material_ids.size());
      std::transform(shape.mesh.material_ids.begin(),
                     shape.mesh.material_ids.end(),
                     mIDs.begin(),
                     [&](int i) { return i + baseMaterialOffset; });
      mesh.createChildData("material", mIDs);
      mesh.child("material").setSGOnly();

      mesh.createChildData("vertex.position", v);
      mesh.createChildData("index", vi);
      if (!vn.empty())
        mesh.createChildData("vertex.normal", vn);
      if (!vt.empty())
        mesh.createChildData("vertex.texcoord", vt);
    }

    // Finally, add node hierarchy to importer parent
    add(rootNode);

    std::cout << "...finished import!\n";
  }

  }  // namespace sg
} // namespace ospray
