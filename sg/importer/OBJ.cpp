// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Importer.h"
// tiny_obj_loader
#include "tiny_obj_loader.h"
// rkcommon
#include "rkcommon/os/FileName.h"
#include "sg/scene/geometry/Geometry.h"
#include "sg/Util.h"

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

    // Parse #hexcode colors, (must be #rgb 3 component, html web format)
    // Understands either s#RGB for sRGB parsing, or #RGB for linear
    if (paramValueString.front() == '#'
        || (paramValueString.size() > 1 && paramValueString.at(1) == '#')) {
      bool isSRGB = (paramValueString.front() == 's');
      std::string format = std::string(isSRGB ? "s#" : "#") + "%02x%02x%02x";
      int r, g, b;
      rgb RGB = vec3f(1.f, 0.f, 1.f); // error color

      bool failedParse = false;
      // Expect a string of the exact correct length
      if (paramValueString.size() != 6 + 1 + (isSRGB ? 1 : 0))
        failedParse = true;

      if (!failedParse) {
        sscanf(paramValueString.c_str(), format.c_str(), &r, &g, &b);

        if (min(r, min(g, b)) < 0 || max(r, max(g, b)) > 255)
          failedParse = true;
      }

      if (!failedParse) {
        clamp(r, 0x00, 0xff);
        clamp(g, 0x00, 0xff);
        clamp(b, 0x00, 0xff);
        RGB = vec3f(r, g, b) / 255.f;
        if (isSRGB)
          srgb_to_linear(RGB);
      } else {
        std::cerr << "Error: malformed color string '" << paramValueString
                  << "'." << std::endl;
      }

      floats.push_back(RGB.x);
      floats.push_back(RGB.y);
      floats.push_back(RGB.z);

    } else {
      std::stringstream valueStream(typeAndValueString);
      float val;
      while (valueStream >> val)
        floats.push_back(val);
    }

    if (floats.size() == 1) {
      paramType  = "float";
      paramValue = floats[0];
    } else if (floats.size() == 2) {
      paramType  = "vec2f";
      paramValue = vec2f(floats[0], floats[1]);
    } else if (floats.size() == 3) {
      paramType  = "vec3f";
      paramValue = vec3f(floats[0], floats[1], floats[2]);
    } else if (floats.size() == 4) {
      paramType  = "linear2f";
      paramValue = linear2f(floats[0], floats[1], floats[2], floats[3]);
    } else {
      // Unknown type.
      paramValue = typeAndValueString;
    }
  }

  std::shared_ptr<Texture2D> createSGTex(const std::string &map_name,
                                             const FileName &texName,
                                             const FileName &containingPath,
                                             const bool preferLinear  = false,
                                             const bool nearestFilter = false)
  {
    auto sgTex = createNodeAs<Texture2D>(map_name, "texture_2d");

    // Set parameters affecting texture load and usage
    sgTex->samplerParams.preferLinear = preferLinear;
    sgTex->samplerParams.nearestFilter = nearestFilter;

    // If load fails, remove the texture node
    if (!sgTex->load(containingPath + texName))
      sgTex = nullptr;

    return sgTex;
  }

  static OBJData loadFromFile(FileName fileName)
  {
    const std::string containingPath = fileName.path();
    // Try to load scene without auto-triangulation.  If point, lines,
    // or polygons are found; reload the scene as a triangle mesh.
    bool needsReload = false;

    do {
      OBJData retval;
      std::string warn;
      std::string err;

      tinyobj::LoadObj(&retval.attrib,
          &retval.shapes,
          &retval.materials,
          &warn,
          &err,
          fileName.c_str(),
          containingPath.c_str(),
          needsReload); // triangulate meshes if true

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

        if (!warn.empty()) {
          std::cerr << "#ospsg: obj parsing warning(s)...\n"
                    << warn << std::endl;
        }
        if (!err.empty()) {
          std::cerr << "#ospsg: obj parsing errors(s)...\n"
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

            // color textures are typically sRGB gamma encoded, others prefer
            // linear, texture format may override this preference
            bool preferLinear = paramName.find("Color") == paramName.npos
                && paramName.find("kd") == paramName.npos
                && paramName.find("ks") == paramName.npos;

            bool nearestFilter =
                (paramName.find("rotation") != std::string::npos) ||
                (paramName.find("Rotation") != std::string::npos);

            auto map_misc = createSGTex(paramName,
                                        paramValueStr,
                                        containingPath,
                                        preferLinear,
                                        nearestFilter);

            // If texture is UDIM, set appropriate scale/translation
            if (map_misc) {
              auto &ospTex = *map_misc->nodeAs<Texture2D>();
              if (ospTex.hasUDIM()) {
                auto scale = ospTex.getUDIM_scale();
                auto translation = ospTex.getUDIM_translation();
                // Set UDIM scale and translation
                paramNodes.push_back(
                    createNode(paramName + ".scale", "vec2f", scale));
                paramNodes.push_back(createNode(
                    paramName + ".translation", "vec2f", translation));
              }
              paramNodes.push_back(map_misc);
            }

          } else {
            rkcommon::utility::Any paramValue;
            parseParameterString(paramValueStr, paramType, paramValue);
            // Principled::thin is the only non-float material parameter and
            // needs to be converted to a "bool" paramType, imported as a float
            if (paramName == "thin") {
              paramType = "bool";
              paramValue = (paramValue.get<float>() == 0.f) ? false : true;
            } else if (paramType == "vec3f") {
              // rgb type allows for ImGui color editor
              paramType = "rgb";
            }

            try {
              auto newParam = createNode(paramName, paramType, paramValue);
              paramNodes.push_back(newParam);
            } catch (const std::runtime_error &) {
              std::cout << "attempting to load param as texture: " << paramName
                        << " " << paramValueStr << std::endl;
              auto map_misc =
                  createSGTex(paramName, paramValueStr, containingPath);
              if (map_misc)
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
          // sRGB gamma encoded, texture format may override this preference
          auto tex = createSGTex("map_kd", m.diffuse_texname, containingPath);
          if (tex)
            mat.add(tex);
        }

        if (!m.specular_texname.empty()) {
          // sRGB gamma encoded, texture format may override this preference
          auto tex = createSGTex("map_ks", m.specular_texname, containingPath);
          if (tex)
            mat.add(tex);
        }

        if (!m.specular_highlight_texname.empty()) {
          auto tex = createSGTex(
              "map_ns", m.specular_highlight_texname, containingPath, true);
          if (tex)
            mat.add(tex);
        }

        if (!m.bump_texname.empty()) {
          auto tex =
              createSGTex("map_bump", m.bump_texname, containingPath, true);
          if (tex)
            mat.add(tex);
        }

        if (!m.alpha_texname.empty()) {
          auto tex =
              createSGTex("map_d", m.alpha_texname, containingPath, true);
          if (tex)
            mat.add(tex);
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
    NodePtr rootNode = hasChild(baseName) ? child(baseName).nodeAs<Node>()
                                          : createNode(baseName, "transform");

    auto objData = loadFromFile(fileName);

    auto materialNodes = createMaterials(objData, fileName);
    // If the model provided no materials, create a default
    if (materialNodes.empty())
      materialNodes.emplace_back(createNode("default", "obj"));

    // Create a child under the importer to hold the baseMaterialOffset
    // used when the asset is reloaded, to keep the same material indices
    uint32_t baseMaterialOffset = materialRegistry->baseMaterialOffSet();
    static std::string bmo = "baseMaterialOffset";
    if (hasChild(bmo)) {
      baseMaterialOffset = child(bmo).valueAs<uint32_t>();
    } else {
      createChild(bmo, "uint32_t", baseMaterialOffset);
      child(bmo).setReadOnly();
      child(bmo).setSGOnly();
      child(bmo).setSGNoUI();
    }

    for (auto m : materialNodes)
      materialRegistry->add(m);

    auto &attrib = objData.attrib;

    int shapeId = 0;

    for (auto &shape : objData.shapes) {
      auto numSrcIndices = shape.mesh.indices.size();

      // Mesh indices.size == 0 indicates a non-mesh primitive
      // (points, lines, curves and surfaces)
      if (numSrcIndices == 0)
        continue;
      
      auto name = std::to_string(shapeId++) + '_' + shape.name;
      auto mesh = createNodeAs<Geometry>(name, "geometry_triangles");
      auto &v = mesh->positions;
      v.reserve(numSrcIndices);
      auto &vi = mesh->quad_vi;
      vi.reserve(numSrcIndices);
      auto &vn = mesh->normals;
      vn.reserve(numSrcIndices);
      auto &vt = mesh->vt;
      vt.reserve(numSrcIndices);

      // OSPRay doesn't support separate arrays for vertex, normal & texcoord
      // indices.  So, reindex by creating a single index array then push_back
      // attribs according to each of their own index arrays.
      // Put all indices into a single vec4.  Triangles duplicate the last
      // index.
      size_t i = 0;
      for (int numVertsInFace : shape.mesh.num_face_vertices) {
        auto isQuad = (numVertsInFace == 4);
        // when a Quad then use same splitting diagonal in OSPRay/Embree as
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

      auto &mIDs = mesh->mIDs;
      bool sameValue = std::all_of(shape.mesh.material_ids.begin(),
          shape.mesh.material_ids.end(),
          [&](const auto &x) { return x == shape.mesh.material_ids.front(); });
      // If all materials IDs in the mesh are the same, set a single value
      // rather than entire vector of same IDs
      if (sameValue) {
        auto materialID = shape.mesh.material_ids.front() + baseMaterialOffset;
        mIDs.emplace_back(materialID);
        mesh->createChild("material", "uint32_t", mIDs.back());
        mesh->child("material").setReadOnly();
      } else {
        mIDs.resize(shape.mesh.material_ids.size());
        std::transform(shape.mesh.material_ids.begin(),
            shape.mesh.material_ids.end(),
            mIDs.begin(),
            [&](int i) { return i + baseMaterialOffset; });
        mesh->createChildData("material", mIDs, true);
      }
      mesh->child("material").setSGOnly();

      mesh->createChildData("vertex.position", v, true);
      mesh->createChildData("index", vi, true);
      if (!vn.empty())
        mesh->createChildData("vertex.normal", vn, true);
      if (!vt.empty())
        mesh->createChildData("vertex.texcoord", vt, true);

      rootNode->add(mesh);
    }

    // Finally, add node hierarchy to importer parent
    add(rootNode);

    std::cout << "...finished import!\n";
  }

  }  // namespace sg
} // namespace ospray
