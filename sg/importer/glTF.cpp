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
// tiny_gltf
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"
// ospcommon
#include "ospcommon/os/FileName.h"

#include "glTF/buffer_view.h"
#include "glTF/gltf_types.h"

#define INFO std::cout << prefix << "(I): "
#define WARN std::cout << prefix << "(W): "
#define ERROR std::cerr << prefix << "(E): "

// Pads a std::string to optional length.  Useful for numbers.
inline std::string pad(std::string string, char p = '0', int length = 3)
{
  return std::string(length - string.length(), p) + string;
}

namespace ospray::sg {

  struct glTFImporter : public Importer
  {
    glTFImporter()           = default;
    ~glTFImporter() override = default;

    void importScene(
        std::shared_ptr<MaterialRegistry> materialRegistry) override;
  };

  OSP_REGISTER_SG_NODE_NAME(glTFImporter, importer_gltf);
  OSP_REGISTER_SG_NODE_NAME(glTFImporter, importer_glb);

  // Helper types /////////////////////////////////////////////////////////////

  static const auto prefix = "#importGLTF";

  struct GLTFData
  {
    GLTFData(Node &rootNode, const FileName &fileName) : fileName(fileName), rootNode(rootNode) {}

   public:
    const FileName &fileName;

    bool parseAsset();
    void createMaterials(MaterialRegistry &materialRegistry);
    void createGeometries();
    void buildScene();

    std::vector<NodePtr> ospMeshes;

   private:
    tinygltf::Model model;

    Node &rootNode;

    std::vector<NodePtr> ospTextures;
    std::vector<NodePtr> ospMaterials;
    // NodePtr ospMaterialList; Not used anymore

    void visitNode(const int nid,
                   const affine3f xfm,
                   const int level);  // XXX level is just for debug

    affine3f nodeTransform(const tinygltf::Node &node);

    NodePtr createOSPMesh(const std::string &primBaseName,
                              tinygltf::Primitive &primitive);

    NodePtr createOSPMaterial(const tinygltf::Material &material);

    NodePtr createOSPTexture(const tinygltf::Texture &texture);

    void setOSPTexture(NodePtr ospMaterial,
                       const std::string &texParam,
                       int texIndex,
                       bool preferLinear = true);
  };

  // Helper functions /////////////////////////////////////////////////////////

  bool GLTFData::parseAsset()
  {
    INFO << "TinyGLTF loading: " << fileName << "\n";

    tinygltf::TinyGLTF context;
    std::string err, warn;
    bool ret;

    const auto isASCII = (fileName.ext() == "gltf");
    if (isASCII)
      ret = context.LoadASCIIFromFile(&model, &err, &warn, fileName);
    else
      ret = context.LoadBinaryFromFile(&model, &err, &warn, fileName);

#if defined(REPORT_TINYGLTF_WARNINGS)
    if (!warn.empty()) {
      WARN << "gltf parsing warning(s)...\n" << warn << std::endl;
    }

    if (!err.empty()) {
      ERROR << "gltf parsing errors(s)...\n" << err << std::endl;
    }
#endif

    if (!ret) {
      ERROR << "FATAL error parsing gltf file,"
            << " no geometry added to the scene!" << std::endl;
      return false;
    }

    auto asset = model.asset;

    if (std::stof(asset.version) < 2.0) {
      ERROR << "FATAL: incompatible glTF file, version " << asset.version
            << std::endl;
      return false;
    }

    if (!asset.copyright.empty())
      INFO << "   Copyright: " << asset.copyright << "\n";
    if (!asset.generator.empty())
      INFO << "   Generator: " << asset.generator << "\n";
    if (!asset.extensions.empty())
      INFO << "   Extensions Listed:\n";

    // XXX Warn on any extensions used
    if (!model.extensionsUsed.empty()) {
      WARN << "   ExtensionsUsed:\n";
      for (const auto &ext : model.extensionsUsed)
        WARN << "      " << ext << "\n";
    }
    if (!model.extensionsRequired.empty()) {
      WARN << "   ExtensionsRequired:\n";
      for (const auto &ext : model.extensionsRequired)
        WARN << "      " << ext << "\n";
    }

    // XXX
    INFO << "... " << model.accessors.size() << " accessors\n";
    INFO << "... " << model.animations.size() << " animations\n";
    INFO << "... " << model.buffers.size() << " buffers\n";
    INFO << "... " << model.bufferViews.size() << " bufferViews\n";
    INFO << "... " << model.materials.size() << " materials\n";
    INFO << "... " << model.meshes.size() << " meshes\n";
    INFO << "... " << model.nodes.size() << " nodes\n";
    INFO << "... " << model.textures.size() << " textures\n";
    INFO << "... " << model.images.size() << " images\n";
    INFO << "... " << model.skins.size() << " skins\n";
    INFO << "... " << model.samplers.size() << " samplers\n";
    INFO << "... " << model.cameras.size() << " cameras\n";
    INFO << "... " << model.scenes.size() << " scenes\n";
    INFO << "... " << model.lights.size() << " lights\n";

    return ret;
  }

  void GLTFData::createMaterials(MaterialRegistry &materialRegistry)
  {
    INFO << "Create Materials\n";
    // Create materials and textures
    // (adding 1 for default material)
    ospMaterials.reserve(model.materials.size() + 1);
    ospTextures.reserve(model.textures.size());

#if 0
    // "default" material for glTF '-1' index (no material)
    ospMaterials.emplace_back(createNode("default", "material_obj"));
#endif

    WARN << "Skipping materials at the moment -- needs 2.0 implmentation\n";
#if 0
    // Create textures (references model.images[])
    for (const auto &texture : model.textures) {
      ospTextures.push_back(createOSPTexture(texture));
    }

    // Create materials (also sets textures to material params)
    for (const auto &material : model.materials) {
      ospMaterials.push_back(createOSPMaterial(material));
    }

    // Create materials list
    // XXX Is there a better way to do this?!
    auto materialsList =
        createNode("materialList", "MaterialList")->nodeAs<MaterialList>();
    for (const auto &ospMat : ospMaterials)
      materialsList->push_back(ospMat);

    ospMaterialList = materialsList;
#endif

    size_t baseMaterialOffset = materialRegistry.children().size();
    for (auto m : ospMaterials) {
      materialRegistry.add(m);
      materialRegistry.matImportsList.push_back(m->name());
    }
  }

  void GLTFData::createGeometries()
  {
    INFO << "Create Geometries\n";
    ospMeshes.reserve(model.meshes.size());
    for (auto &m : model.meshes) {  // -> Model
      static auto nModel = 0;
      auto modelName     = m.name + "_" + pad(std::to_string(nModel++));

      INFO << pad("", '.', 3) << "mesh." + modelName << "\n";

      for (auto &prim : m.primitives) {  // -> TriangleMesh
        // Create per 'primitive' geometry
        auto ospMesh = createOSPMesh(modelName, prim);
        ospMeshes.push_back(ospMesh);
      }
    }
  }

  void GLTFData::buildScene()
  {
    INFO << "Build Scene\n";
    if (model.defaultScene == -1)
      model.defaultScene = 0;

    // Process all nodes in default scene
    for (const auto &nid : model.scenes[model.defaultScene].nodes) {
      const tinygltf::Node &n = model.nodes[nid];
      INFO << "... Top Node (#" << nid << ")\n";
      affine3f xfm{one};
      // recursively process node hierarchy
      visitNode(nid, xfm, 1);
    }
  }

  void GLTFData::visitNode(const int nid,
                           const affine3f xfm,
                           const int level)  // XXX just for debug
  {
    const tinygltf::Node &n = model.nodes[nid];
    INFO << pad("", '.', 3 * level) << nid << ":" << n.name
        << " (children:" << n.children.size() << ")\n";

    // XXX !!! This is still a single-level-instance implementation, since
    // all tranforms are only applied to mesh-producing leaf nodes.
    // Explore full multi-level instancing, allowing transforms to occur
    // higher up, with the nodes that specify them.
    // On 1.8.5 SG, problems with Adam Head eyes when attempting multi-level

    // Apply any transform in this node -> xfm
    const affine3f new_xfm     = xfm * nodeTransform(n);
    const auto needXfm = (new_xfm != affine3f{one});

    // Create xfm
    // XXX Only create if there's something to add.
    if (needXfm || n.mesh != -1 || n.camera != -1 || n.skin != -1) {
#if 1 // BMCDEBUG, temp removal
      WARN << "Skipping needXfm at the moment -- needs 2.0 implmentation\n";
      if (n.mesh != -1) {
        INFO << pad("", '.', 3 * level) << "....mesh\n";
        rootNode.add(ospMeshes[n.mesh]);
      }
#else
      static auto nNode = 0;
      auto nodeName     = n.name + "_" + pad(std::to_string(nNode++));
      auto ospXfm  = createNode(nodeName + "_xfm", "Transform", affine3f::translate(vec3f(0.f)));
      INFO << pad("", '.', 3 * level) << "..node." + nodeName << "\n";

      if (needXfm) {
        // XXX, requires change to 1.8.5 SG.
        // Pushed to gitlab branch bc/add-instance-userTransform
        // Either make sure this is available in OSPRaySG 2.0, or
        // find another way to do this.
        ospXfm->remove("xfm"); // remove default xfm
        ospXfm->createChild("xfm", "Transform", new_xfm);
        INFO << pad("", '.', 3 * level) << "....xfm\n";
      }

      if (n.mesh != -1) {
        INFO << pad("", '.', 3 * level) << "....mesh\n";
        ospXfm->add(ospMeshes[n.mesh]);
      }

      if (n.camera != -1) {
        WARN << "unsupported node-type: camera\n";
      }

      if (n.skin != -1) {
        WARN << "unsupported node-type: skin\n";
      }

      rootNode.add(ospXfm);
#endif
    }

    // recursively process children nodes
    for (const auto &cid : n.children) {
      visitNode(cid, new_xfm, level + 1);
    }
  }

  affine3f GLTFData::nodeTransform(const tinygltf::Node &n)
  {
    affine3f xfm{one};

    if (!n.matrix.empty()) {
      // Matrix must decompose to T/R/S, no skew/shear
      const auto &m = n.matrix;
      xfm           = {vec3f(m[0 * 4 + 0], m[0 * 4 + 1], m[0 * 4 + 2]),
             vec3f(m[1 * 4 + 0], m[1 * 4 + 1], m[1 * 4 + 2]),
             vec3f(m[2 * 4 + 0], m[2 * 4 + 1], m[2 * 4 + 2]),
             vec3f(m[3 * 4 + 0], m[3 * 4 + 1], m[3 * 4 + 2])};
    } else {
      if (!n.scale.empty()) {
        const auto &s = n.scale;
        xfm           = affine3f::scale(vec3f(s[0], s[1], s[2])) * xfm;
      }
      if (!n.rotation.empty()) {
        // Given the quaternion, create the affine rotation matrix
        const auto &r = n.rotation;
        xfm = affine3f(linear3f(quaternionf(r[3], r[0], r[1], r[2]))) * xfm;
      }
      if (!n.translation.empty()) {
        const auto &t = n.translation;
        xfm           = affine3f::translate(vec3f(t[0], t[1], t[2])) * xfm;
      }
    }
    return xfm;
  }

  NodePtr GLTFData::createOSPMesh(const std::string &primBaseName,
                                  tinygltf::Primitive &prim)
  {
    static auto nPrim = 0;
    auto primName     = primBaseName + "_" + pad(std::to_string(nPrim++));
    INFO << pad("", '.', 6) << "prim." + primName << "\n";
    if (prim.material > -1) {
      INFO << pad("", '.', 6) << "    .uses material #" << prim.material << ": "
           << model.materials[prim.material].name << " (alphaMode "
           << model.materials[prim.material].alphaMode << ")\n";
    }

    // XXX: Create node types based on actual accessor types
    std::vector<vec3f> v;
    std::vector<vec3ui> vi;  // XXX support both 3i and 4i OSPRay 2?
    std::vector<vec4f> vc;
    std::vector<vec3f> vn;
    std::vector<vec2f> vt;

    // XXX Handle this gracefully!
    // XXX GLTF 2.0 spec supports
    // POINTS
    // LINES LINE_LOOP LINE_STRIP
    // TRIANGLES TRIANGLE_STRIP TRIANGLE_FAN
    // XXX There's code in gltf-loader.cc for convertedToTriangleList
    if (prim.mode != TINYGLTF_MODE_TRIANGLES) {
      ERROR << "Unsupported primitive mode! File must contain only "
               "triangles\n";
      throw std::runtime_error(
          "Unsupported primitive mode! Only triangles are supported");
      // continue;
    }

#if 1  // XXX: Generalize these with full component support!!!
       // In : 1,2,3,4 ubyte, ubyte(N), ushort, ushort(N), uint, float
       // Out:   2,3,4 int, float

    // Positions: vec3f
    Accessor<vec3f> pos_accessor(model.accessors[prim.attributes["POSITION"]],
                                 model);
    v.reserve(pos_accessor.size());
    for (size_t i = 0; i < pos_accessor.size(); ++i)
      v.emplace_back(pos_accessor[i]);

    // Indices: scalar ubyte/ushort/uint
    if (model.accessors[prim.indices].componentType ==
        TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
      Accessor<uint8_t> index_accessor(model.accessors[prim.indices], model);
      vi.reserve(index_accessor.size() / 3);
      for (size_t i = 0; i < index_accessor.size() / 3; ++i)
        vi.emplace_back(index_accessor[i * 3]);
    } else if (model.accessors[prim.indices].componentType ==
               TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
      Accessor<uint16_t> index_accessor(model.accessors[prim.indices], model);
      vi.reserve(index_accessor.size() / 3);
      for (size_t i = 0; i < index_accessor.size() / 3; ++i)
        vi.emplace_back(index_accessor[i * 3]);
    } else if (model.accessors[prim.indices].componentType ==
               TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
      Accessor<uint32_t> index_accessor(model.accessors[prim.indices], model);
      vi.reserve(index_accessor.size() / 3);
      for (size_t i = 0; i < index_accessor.size() / 3; ++i)
        vi.push_back(index_accessor[i * 3]);
    } else {
      ERROR << "Unsupported index type: "
            << model.accessors[prim.indices].componentType << "\n";
      throw std::runtime_error("Unsupported index component type");
    }

    // Colors: vec3/vec4 float/ubyte(N)/ushort(N) RGB/RGBA
    // accessor->normalized
    auto fnd = prim.attributes.find("COLOR_0");
    if (fnd != prim.attributes.end()) {
      auto col_attrib = fnd->second;
      if (model.accessors[col_attrib].componentType ==
          TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        Accessor<uint16_t> col_accessor(model.accessors[col_attrib], model);
        vc.reserve(col_accessor.size() / 3);
        for (size_t i = 0; i < col_accessor.size() / 3; ++i) {
          vc.push_back(vec4f(col_accessor[i * 3],
                             col_accessor[i * 3 + 1],
                             col_accessor[i * 3 + 2],
                             1.f));
        }
      } else if (model.accessors[col_attrib].componentType ==
                 TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
        Accessor<uint8_t> col_accessor(model.accessors[col_attrib], model);
        vc.reserve(col_accessor.size() / 3);
        for (size_t i = 0; i < col_accessor.size() / 3; ++i) {
          vc.push_back(vec4f(col_accessor[i * 3],
                             col_accessor[i * 3 + 1],
                             col_accessor[i * 3 + 2],
                             1.f));
        }
      } else if (model.accessors[col_attrib].componentType ==
                 TINYGLTF_COMPONENT_TYPE_FLOAT) {
        Accessor<vec4f> col_accessor(model.accessors[col_attrib], model);
        vc.reserve(col_accessor.size());
        for (size_t i = 0; i < col_accessor.size(); ++i)
          vc.emplace_back(col_accessor[i]);
      } else {
        ERROR << "Unsupported color type: "
              << model.accessors[col_attrib].componentType << "\n";
        throw std::runtime_error("Unsupported color component type");
      }

      // If alphaMode is OPAQUE (or default material), leave colors unchanged,
      // but set all alpha to 1.f
      if (prim.material == -1 ||
          model.materials[prim.material].alphaMode == "OPAQUE") {
        INFO << pad("", '.', 6) << "prim. Correcting Alpha\n";
        std::transform(vc.begin(), vc.end(), vc.begin(), [](vec4f c) {
          return vec4f(c.x, c.y, c.z, 1.f);
        });
      }
    }

    // Normals: vec3f
    fnd = prim.attributes.find("NORMAL");
    if (fnd != prim.attributes.end()) {
      Accessor<vec3f> normal_accessor(model.accessors[fnd->second], model);
      vn.reserve(normal_accessor.size());
      for (size_t i = 0; i < normal_accessor.size(); ++i)
        vn.emplace_back(normal_accessor[i]);
    }

    // TexCoords: vec2 float/ubyte(N)/ushort(N)
    // accessor->normalized
    // Note: GLTF can have texture coordinates [0,1] used by different
    // textures.  Only supporting TEXCOORD_0
    fnd = prim.attributes.find("TEXCOORD_0");
    if (fnd != prim.attributes.end()) {
      Accessor<vec2f> uv_accessor(model.accessors[fnd->second], model);
      vt.reserve(uv_accessor.size());
      for (size_t i = 0; i < uv_accessor.size(); ++i)
        vt.emplace_back(uv_accessor[i]);
    }
    fnd = prim.attributes.find("TEXCOORD_1");
    if (fnd != prim.attributes.end()) {
      WARN << "gltf found TEXCOOR_1 attribute.  Not supported...\n"
           << std::endl;
    }
#endif

    // Add attribute arrays to mesh
    auto ospGeom = createNode(primName + "_object", "geometry_triangles");

    ospGeom->createChildData("vertex.position", v);
    ospGeom->createChildData("index", vi);
    if (!vn.empty())
      ospGeom->createChildData("vertex.normal", vn);
#if 0 // Shouldn't cause any problem, but get basic geometry working first
    if (!vc.empty())
      ospGeom->createChildData("vertex.color", vc);
    if (!vn.empty())
      ospGeom->createChildData("vertex.normal", vn);
    if (!vt.empty())
      ospGeom->createChildData("vertex.texcoord", vt);
#endif

#if 0  // XXX might need to make ospGeom the child of a material node
    // Add material reference (adding 1 for default material)
    auto materialID = prim.material + 1;
    if (materialID > 0) {
      ospGeom->createChildWithValue("geom.materialID", "int", materialID);
      ospGeom->add(ospMaterialList);
    }
#else
    // XXX BMCDEBUG prevents crash until getting materials to work!
    std::vector<uint32_t> mIDs(v.size(), 0);
    ospGeom->createChildData("material", mIDs);
#endif

    return ospGeom;
  }

  NodePtr GLTFData::createOSPMaterial(const tinygltf::Material &mat)
  {
    static auto nMat = 0;
    auto matName     = mat.name + "_" + pad(std::to_string(nMat++));
    INFO << pad("", '.', 3) << "material." + matName << "\n";
    INFO << pad("", '.', 3) << "        .alphaMode:" << mat.alphaMode << "\n";
    INFO << pad("", '.', 3) << "        .alphaCutoff:" << (float)mat.alphaCutoff
         << "\n";

    auto &pbr = mat.pbrMetallicRoughness;

    INFO << pad("", '.', 3)
         << "        .baseColorFactor.alpha:" << (float)pbr.baseColorFactor[4]
         << "\n";

#if 0
    auto ospMat = createNode(matName, "Material")->nodeAs<Material>();
    ospMat->createChild("type", "string")     = std::string("Principled");
    ospMat->createChild("baseColor", "vec3f") = vec3f(
        pbr.baseColorFactor[0], pbr.baseColorFactor[1], pbr.baseColorFactor[2]);
    ospMat->createChild("metallic", "float")  = (float)pbr.metallicFactor;
    ospMat->createChild("roughness", "float") = (float)pbr.roughnessFactor;

    if (mat.alphaMode == "MASK")
      ospMat->createChild("opacity", "float") = (float)mat.alphaCutoff;

    // XXX If alphaMode is BLEND, then alpha is taken from the baseColor
    // Need to create an opacityMap from the baseColor[4] texture!!!
    if (mat.alphaMode == "BLEND")
      ospMat->createChild("opacity", "float") = (float)0.2;

    // All textures *can* specify a texcoord other than 0.  OSPRay only
    // supports one set of texcoords (TEXCOORD_0).

    NodePtr ospTex = nullptr;

    if (pbr.baseColorTexture.index != -1 &&
        pbr.baseColorTexture.texCoord == 0) {
      // Used as a color texture, must be sRGB space, not linear
      setOSPTexture(ospMat, "baseColor", pbr.baseColorTexture.index, false);

      // Create a single channel textures from the baseColorTexture[4]
      if (mat.alphaMode == "BLEND") {
      }
    }

    // XXX Not sure exactly how to map these yet.  Are they single component?
    if (pbr.metallicRoughnessTexture.index != -1 &&
        pbr.metallicRoughnessTexture.texCoord == 0) {
      setOSPTexture(ospMat, "metallic", pbr.metallicRoughnessTexture.index);
      setOSPTexture(ospMat, "roughness", pbr.metallicRoughnessTexture.index);
    }

    if (mat.normalTexture.index != -1 && mat.normalTexture.texCoord == 0) {
      // NormalTextureInfo() : index(-1), texCoord(0), scale(1.0) {}
      setOSPTexture(ospMat, "normal", mat.normalTexture.index);
      ospMat->createChild("normal", "float", (float)mat.normalTexture.scale);
    }

#if 0  // No reason to use occlusion in OSPRay
      if (mat.occlusionTexture.index != -1 &&
          mat.occlusionTexture.texCoord == 0) {
        // OcclusionTextureInfo() : index(-1), texCoord(0), strength(1.0) {}
        setOSPTexture(ospMat, "occlusion", mat.occlusionTexture.index);
        ospMat->createChild("occlusion", "float", mat.occlusionTexture.scale);
      }
#endif

    // XXX TODO Principled material doesn't have emissive params yet
    auto emissiveColor = vec3f(
        mat.emissiveFactor[0], mat.emissiveFactor[1], mat.emissiveFactor[2]);
    if (emissiveColor != vec3f(0.f)) {
      WARN << "Material has emissiveFactor = " << emissiveColor << "\n";
      ospMat->createChild("emissiveColor", "vec3f") = emissiveColor;
    }

    if (mat.emissiveTexture.index != -1 && mat.emissiveTexture.texCoord == 0) {
      setOSPTexture(ospMat, "emissive", mat.emissiveTexture.index);
      WARN << "Material has emissiveTexture #" << mat.emissiveTexture.index
           << "\n";
    }

    return ospMat;
#endif
    return {};
  }

  NodePtr GLTFData::createOSPTexture(const tinygltf::Texture &tex)
  {
#if 0
    static auto nTex = 0;

    if (tex.source == -1)
      return nullptr;

    tinygltf::Image &img = model.images[tex.source];

    if (img.uri.length() < 256) {  // The uri can be a stream of data!
      img.name = FileName(img.uri).name();
    }
    auto texName = img.name + "_" + pad(std::to_string(nTex++));

    INFO << pad("", '.', 6) << "texture." + texName << "\n";
    INFO << pad("", '.', 9) << "image name: " << img.name << "\n";
    INFO << pad("", '.', 9) << "image width: " << img.width << "\n";
    INFO << pad("", '.', 9) << "image height: " << img.height << "\n";
    INFO << pad("", '.', 9) << "image component: " << img.component << "\n";
    INFO << pad("", '.', 9) << "image bits: " << img.bits << "\n";
    if (img.uri.length() < 256)  // The uri can be a stream of data!
      INFO << pad("", '.', 9) << "image uri: " << img.uri << "\n";
    INFO << pad("", '.', 9) << "image bufferView: " << img.bufferView << "\n";
    INFO << pad("", '.', 9) << "image data: " << img.image.size() << "\n";

    // XXX Only handle pre-loaded images for now
    if (img.image.empty()) {
      ERROR << "!!! no texture data!  Need to load it !!!\n";
      return nullptr;
    }

    auto ospTex =
        createNode(texName + "_tex", "Texture2D")->nodeAs<Texture2D>();
    ospTex->setName(texName + "_tex");

    ospTex->size.x      = img.width;
    ospTex->size.y      = img.height;
    ospTex->channels    = img.component;
    const bool hdr      = img.bits > 8;
    ospTex->depth       = hdr ? 4 : 1;
    const size_t stride = ospTex->size.x * ospTex->channels * ospTex->depth;
    ospTex->data =
        alignedMalloc(sizeof(unsigned char) * ospTex->size.y * stride);

    // XXX Check sampler wrap/clamp modes!!!
    auto nearestFilter = false;
    if (tex.sampler != -1 && model.samplers[tex.sampler].magFilter ==
                                 TINYGLTF_TEXTURE_FILTER_NEAREST) {
      nearestFilter = true;
    }
    ospTex->nearestFilter = nearestFilter;

    std::memcpy(ospTex->data, img.image.data(), img.image.size());

    return ospTex;
#endif
    return {};
  }

  void GLTFData::setOSPTexture(NodePtr ospMat,
                               const std::string &texParamBase,
                               int texIndex,
                               bool preferLinear)
  {
#if 0
    auto &ospTex = ospTextures[texIndex];
    if (ospTex) {
      auto texParam = texParamBase + "Map";

      INFO << pad("", '.', 3) << "        .setChild: " << texParam << "= "
           << ospTex->name() << "\n";

      ospTex->preferLinear = preferLinear;
      ospMat->setChild(texParam, ospTex);
    }
#endif
  }

  // GLTFmporter definitions //////////////////////////////////////////////////

  void glTFImporter::importScene(
      std::shared_ptr<MaterialRegistry> materialRegistry)
  {
    auto file = FileName(child("file").valueAs<std::string>());

    // Create a root Transform/Instance off the Importer, under which to build the import hierarchy

    // XXX This messes up the OBJ importer, so don't use a top-level transform here either
    // === Still get nothing to render here though ===
#if 0
    auto &rootNode = createChild("root_node_xfm", "Transform", affine3f{one});
#else
    Node &rootNode = *this;
#endif

    GLTFData gltf(rootNode, file);

    if (!gltf.parseAsset())
      return;

    gltf.createMaterials(*materialRegistry);
    gltf.createGeometries();
    gltf.buildScene();

    INFO << "finished import!\n";
  }

}  // namespace ospray::sg
