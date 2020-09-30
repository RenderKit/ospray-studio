// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Importer.h"
// tiny_gltf
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"
// rkcommon
#include "rkcommon/os/FileName.h"

#include "glTF/buffer_view.h"
#include "glTF/gltf_types.h"

#include "../visitors/PrintNodes.h"

// Note: may want to disable warnings/errors from TinyGLTF
#define REPORT_TINYGLTF_WARNINGS

#define DEBUG std::cout << prefix << "(D): "
#define INFO std::cout << prefix << "(I): "
#define WARN std::cout << prefix << "(W): "
#define ERROR std::cerr << prefix << "(E): "

namespace ospray {
  namespace sg {

  struct glTFImporter : public Importer
  {
    glTFImporter()           = default;
    ~glTFImporter() override = default;

    void importScene() override;

  };

  OSP_REGISTER_SG_NODE_NAME(glTFImporter, importer_gltf);
  OSP_REGISTER_SG_NODE_NAME(glTFImporter, importer_glb);

  // Helper types /////////////////////////////////////////////////////////////

  static const auto prefix = "#importGLTF";

  struct GLTFData
  {
    GLTFData(NodePtr rootNode, const FileName &fileName)
        : fileName(fileName), rootNode(rootNode)
    {
    }

   public:
    const FileName &fileName;

    bool parseAsset();
    void createMaterials(MaterialRegistry &materialRegistry);
    void createGeometries();
    void buildScene();
    void loadAssetInfo(NodePtr sgNode);
    void addReferenceLinkInfo(const int nid, NodePtr sgNode);

    // load animations AFTER loading scene nodes and their transforms
    void loadChannels();

    std::vector<NodePtr> ospMeshes;

    bool animate{false};

    bool hasAnimations{false};

    std::map<float, int> g_allTimesteps;

   private:
    std::map<int, NodePtr> animatedNodes;

    tinygltf::Model model;

    NodePtr rootNode;

    std::vector<NodePtr> ospMaterials;

    size_t baseMaterialOffset = 0; // set in createMaterials()

    void loadKeyframeInput(int accessorID, std::vector<float> &kfInput);

    void loadKeyframeOutput(int accessorID, std::vector<affine3f> &kfOutput, std::string &propertyName);

    void visitNode(NodePtr sgNode,
        const int nid,
        const int level); // XXX level is just for debug

    affine3f nodeTransform(const tinygltf::Node &node);

    NodePtr createOSPMesh(
        const std::string &primBaseName, tinygltf::Primitive &primitive);

    NodePtr createOSPMaterial(const tinygltf::Material &material);

    NodePtr createOSPTexture(const std::string &texParam,
        const tinygltf::Texture &texture,
        const bool preferLinear);

    void setOSPTexture(NodePtr ospMaterial,
        const std::string &texParam,
        int texIndex,
        bool preferLinear = true);
  };

  // Helper functions /////////////////////////////////////////////////////////

  // Pads a std::string to optional length.  Useful for numbers.
  inline std::string pad(std::string string, char p = '0', int length = 4)
  {
    return std::string(std::max(0, length - (int)string.length()), p) + string;
  }

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
    INFO << "... " << model.animations.size() << " animations\n";

    return ret;
  }

  void GLTFData::loadAssetInfo(NodePtr sgNode) {
    auto asset = model.asset;

    // currently load BIT_asset_info extension with partial properties only
    // For reference links to work we need the 'id', 'type' and 'title'
    auto fnd = asset.extensions.find("BIT_asset_info");
    if (fnd != asset.extensions.end()) {
      auto &assetExt = asset.extensions["BIT_asset_info"];

      auto assetId = assetExt.Get("id").Get<std::string>();
      auto assetTitle = assetExt.Get("title").Get<std::string>();
      auto assetType = assetExt.Get("type").Get<std::string>();

      sgNode->createChild("asset_id", "string", assetId);
      sgNode->createChild("asset_title", "string", assetTitle);
      sgNode->createChild("asset_type", "string", assetType);
    }
  }

  void GLTFData::addReferenceLinkInfo(const int nid, NodePtr sgNode) {
    const tinygltf::Node &n = model.nodes[nid];
    auto refLinkExtension = n.extensions.find("BIT_reference_link")->second;

    auto &refId =
      refLinkExtension.Get("id").Get<std::string>(); 
    auto &refType =
      refLinkExtension.Get("type").Get<std::string>(); 
    auto &refTitle =
      refLinkExtension.Get("title").Get<std::string>();

    auto refLinkInfoNode = createNode("refLink_" + refTitle , "Node");
    refLinkInfoNode->createChild("id", "string", refId);
    refLinkInfoNode->createChild("title", "string", refType);
    refLinkInfoNode->createChild("type", "string", refTitle);

    sgNode->add(refLinkInfoNode);

  }

  void GLTFData::createMaterials(MaterialRegistry &materialRegistry)
  {
    //DEBUG << "Create Materials\n";

    // Create materials and textures
    // (adding 1 for default material)
    ospMaterials.reserve(model.materials.size() + 1);

    // "default" material for glTF '-1' index (no material)
    ospMaterials.emplace_back(createNode("default", "obj"));

    // Create materials (also sets textures to material params)
    for (const auto &material : model.materials) {
      ospMaterials.push_back(createOSPMaterial(material));
    }

    baseMaterialOffset = materialRegistry.children().size();
    for (auto m : ospMaterials) {
      materialRegistry.add(m);
      materialRegistry.matImportsList.push_back(m->name());
    }
  }

  void GLTFData::createGeometries()
  {
    //DEBUG << "Create Geometries\n";

    ospMeshes.reserve(model.meshes.size());
    for (auto &m : model.meshes) {  // -> Model
      static auto nModel = 0;
      auto modelName     = m.name + "_" + pad(std::to_string(nModel++));

      // XXX Is there a better way to represent this "group" than a transform
      // node?
      auto ospModel = createNode(
          modelName + "_model", "Transform", affine3f{one});  // Model "group"
      // DEBUG << pad("", '.', 3) << "mesh." + modelName << "\n";

      NodePtr ospMesh;

      for (auto &prim : m.primitives) {  // -> TriangleMesh
        // Create per 'primitive' geometry
        auto ospMesh = createOSPMesh(modelName, prim);
        ospModel->add(ospMesh);
      }

      ospMeshes.push_back(ospModel);
    }
  }

  // create animation channels and load sampler information
  // link nodes with the channels that animate them in animatedNodes map

  void GLTFData::loadChannels()
  {
    auto numAnimations = model.animations.size();

    std::vector<std::vector<NodePtr>> samplerNodes(numAnimations);

    int currentAnim = 0;
    for (auto &a : model.animations) {
      samplerNodes[currentAnim].resize(a.samplers.size());
      int samplerID = 0;

      for (auto &s : a.samplers) {
        auto sIn = s.input;
        auto sOut = s.output;
        auto interpolation = s.interpolation;

        auto sampler = createNode("sampler");
        sampler->createChild("inputAccessor", "int", sIn);
        sampler->createChild("outputAccessor", "int", sOut);
        sampler->createChild("interpolation", "string", interpolation);

        samplerNodes[currentAnim][samplerID] = sampler;

        samplerID++;
      }

      int channelID = 0;
      for (auto &c : a.channels) {
        auto targetNode = c.target_node;
        auto targetPath = c.target_path;
        auto samplerID = c.sampler;

        auto channel = createNode("channel_" + std::to_string(channelID) + "_"
            + std::to_string(currentAnim));
        channel->createChild("targetNode", "int", targetNode);
        channel->createChild("targetPath", "string", targetPath);
        channel->add(samplerNodes[currentAnim][samplerID]);

        channelID++;

        animatedNodes.insert(std::make_pair(targetNode, channel));
      }
      currentAnim++;
    }
  }

  void GLTFData::buildScene()
  {
    // DEBUG << "Build Scene\n";

    if (model.defaultScene == -1)
      model.defaultScene = 0;

    // create animation channels for each animation
    loadChannels();

    // Process all nodes in default scene
    for (const auto &nid : model.scenes[model.defaultScene].nodes) {
      NodePtr sgNode = rootNode;
      // DEBUG << "... Top Node (#" << nid << ")\n";
      // recursively process node hierarchy
      visitNode(sgNode, nid, 1);
    }
  }

  void GLTFData::visitNode(NodePtr sgNode,
                           const int nid,
                           const int level)  // XXX just for debug
  {
    const tinygltf::Node &n = model.nodes[nid];
    // DEBUG << pad("", '.', 3 * level) << nid << ":" << n.name
    //     << " (children:" << n.children.size() << ")\n";

    // Apply any transform in this node -> xfm
    const auto nodeXfm = nodeTransform(n);

    static auto nNode = 0;
    auto nodeName = n.name + "_" + pad(std::to_string(nNode++));
    // DEBUG << pad("", '.', 3 * level) << "..node." + nodeName << "\n";
    // DEBUG << pad("", '.', 3 * level) << "....xfm\n";
    auto newXfm = createNode(
        nodeName + "_xfm_" + std::to_string(level), "Transform", nodeXfm);
    sgNode->add(newXfm);
    sgNode = newXfm;

    if (n.extensions.find("BIT_reference_link") != n.extensions.end())
        addReferenceLinkInfo(nid, sgNode);

    if (animate) {
    // for each scene node check if it is animated (if it's node ID exist in animatedNodes map)
    // and load animation transforms
    std::vector<float> kfInput;
    std::vector<affine3f> kfOutput;

    if (animatedNodes.find(nid) != animatedNodes.end()) {
      auto &animChannel = animatedNodes[nid];
      auto &propertyName = animChannel->child("targetPath").valueAs<std::string>();
      auto &sampler = animChannel->child("sampler");
      int inputAcc = sampler.child("inputAccessor").valueAs<int>();

      loadKeyframeInput(inputAcc, kfInput);

      auto outputAcc =
          sampler.child("outputAccessor").valueAs<int>();

      loadKeyframeOutput(outputAcc, kfOutput, propertyName);
    }

    static int numTimestep = 1;

    if (kfInput.size() != 0 || kfOutput.size() != 0) {
      static auto nAnimation = 0;
      auto &animParent = sgNode->createChild("animationNode_" + std::to_string(nAnimation), "animation");

      for (int i = 0; i < kfInput.size(); ++i) {
        auto newXfm = createNode(
            "anim_" + std::to_string(nAnimation) + "_" + std::to_string(i), "Transform", kfOutput[i]);
        newXfm->createChild("timestep", "float", kfInput[i]);
        animParent.add(newXfm);
        g_allTimesteps.insert(std::make_pair(kfInput[i], numTimestep));
        numTimestep++;
      }
      nAnimation++;
    }

    kfInput.clear();
    kfOutput.clear();

    }

    if (n.mesh != -1) {
      // DEBUG << pad("", '.', 3 * level) << "....mesh\n";
      sgNode->add(ospMeshes[n.mesh]);
    }

    if (n.camera != -1) {
      WARN << "unsupported node-type: camera\n";
    }

    if (n.skin != -1) {
      WARN << "unsupported node-type: skin\n";
    }

    // recursively process children nodes
    for (const auto &cid : n.children) {
      visitNode(sgNode, cid, level + 1);
    }

  }

  void GLTFData::loadKeyframeInput(int accessorID, std::vector<float> &kfInput)
  {
    auto &acc = model.accessors[accessorID];

    if (acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
      Accessor<float> input_accessor(acc, model);

      kfInput.resize(input_accessor.size());
      for (size_t i = 0; i < input_accessor.size(); ++i)
        kfInput[i] = input_accessor[i];
    }

  }

  void GLTFData::loadKeyframeOutput(int accessorID, std::vector<affine3f> &xfms, std::string &propertyName)
  {
    auto &acc = model.accessors[accessorID];

    if (acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
      if (propertyName == "rotation") {
        Accessor<vec4f> rt_accessor(acc, model);
        std::vector<vec4f> rt(rt_accessor.size());

        for (size_t i = 0; i < rt_accessor.size(); ++i)
          rt[i] = rt_accessor[i];

        for (auto rt_it = rt.begin(); rt_it != rt.end(); ++rt_it) {
          affine3f xfm{one};
          auto &r = *rt_it;
          xfm = affine3f(linear3f(quaternionf(r[3], r[0], r[1], r[2]))) * xfm;
          xfms.push_back(xfm);
        }

      } else if (propertyName == "translation") {

        Accessor<vec3f> ts_accessor(acc, model);
        std::vector<vec3f> ts(ts_accessor.size());

        for (size_t i = 0; i < ts_accessor.size(); ++i)
          ts[i] = ts_accessor[i];

        for (auto ts_it = ts.begin(); ts_it != ts.end(); ++ts_it) {
          affine3f xfm{one};
          auto &t = *ts_it;
          xfm = affine3f::translate(vec3f(t[0], t[1], t[2])) * xfm;
          xfms.push_back(xfm);
        }
      } else if (propertyName == "scale") {

        Accessor<vec3f> ts_accessor(acc, model);
        std::vector<vec3f> ts(ts_accessor.size());

        for (size_t i = 0; i < ts_accessor.size(); ++i)
          ts[i] = ts_accessor[i];

        for (auto ts_it = ts.begin(); ts_it != ts.end(); ++ts_it) {
          affine3f xfm{one};
          auto &t = *ts_it;
          xfm = affine3f::scale(vec3f(t[0], t[1], t[2])) * xfm;
          xfms.push_back(xfm);
        }
      }
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
    // DEBUG << pad("", '.', 6) << "prim." + primName << "\n";
    if (prim.material > -1) {
      // DEBUG << pad("", '.', 6) << "    .uses material #" << prim.material <<
      // ": "
      //     << model.materials[prim.material].name << " (alphaMode "
      //     << model.materials[prim.material].alphaMode << ")\n";
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
        vi.emplace_back(vec3ui(index_accessor[i * 3],
                               index_accessor[i * 3 + 1],
                               index_accessor[i * 3 + 2]));
    } else if (model.accessors[prim.indices].componentType ==
               TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
      Accessor<uint16_t> index_accessor(model.accessors[prim.indices], model);
      vi.reserve(index_accessor.size() / 3);
      for (size_t i = 0; i < index_accessor.size() / 3; ++i)
        vi.emplace_back(vec3ui(index_accessor[i * 3],
                               index_accessor[i * 3 + 1],
                               index_accessor[i * 3 + 2]));
    } else if (model.accessors[prim.indices].componentType ==
               TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
      Accessor<uint32_t> index_accessor(model.accessors[prim.indices], model);
      vi.reserve(index_accessor.size() / 3);
      for (size_t i = 0; i < index_accessor.size() / 3; ++i)
        vi.emplace_back(vec3ui(index_accessor[i * 3],
                               index_accessor[i * 3 + 1],
                               index_accessor[i * 3 + 2]));
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
        for (size_t i = 0; i < col_accessor.size() / 3; ++i)
          vc.emplace_back(vec4f(col_accessor[i * 3],
                                col_accessor[i * 3 + 1],
                                col_accessor[i * 3 + 2],
                                1.f));
      } else if (model.accessors[col_attrib].componentType ==
                 TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
        Accessor<uint8_t> col_accessor(model.accessors[col_attrib], model);
        vc.reserve(col_accessor.size() / 3);
        for (size_t i = 0; i < col_accessor.size() / 3; ++i)
          vc.emplace_back(vec4f(col_accessor[i * 3],
                                col_accessor[i * 3 + 1],
                                col_accessor[i * 3 + 2],
                                1.f));
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
        //DEBUG << pad("", '.', 6) << "prim. Correcting Alpha\n";
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
    if (!vc.empty())
      ospGeom->createChildData("vertex.color", vc);
    if (!vn.empty())
      ospGeom->createChildData("vertex.normal", vn);
    if (!vt.empty())
      ospGeom->createChildData("vertex.texcoord", vt);

    // add one for default, "no material" material
    auto materialID = prim.material + 1 + baseMaterialOffset;
    std::vector<uint32_t> mIDs(v.size(), materialID);
    ospGeom->createChildData("material", mIDs);
    ospGeom->child("material").setSGOnly();

    return ospGeom;
  }

  NodePtr GLTFData::createOSPMaterial(const tinygltf::Material &mat)
  {
    static auto nMat = 0;
    auto matName     = mat.name + "_" + pad(std::to_string(nMat++));
    //DEBUG << pad("", '.', 3) << "material." + matName << "\n";
    //DEBUG << pad("", '.', 3) << "        .alphaMode:" << mat.alphaMode << "\n";
    //DEBUG << pad("", '.', 3) << "        .alphaCutoff:" << (float)mat.alphaCutoff
    //     << "\n";

    auto &pbr = mat.pbrMetallicRoughness;

    //DEBUG << pad("", '.', 3)
    //     << "        .baseColorFactor.alpha:" << (float)pbr.baseColorFactor[4]
    //     << "\n";

    auto ospMat = createNode(matName, "principled");
    ospMat->createChild("baseColor", "rgb") = vec3f(
        pbr.baseColorFactor[0], pbr.baseColorFactor[1], pbr.baseColorFactor[2]);
    ospMat->createChild("metallic", "float")  = (float)pbr.metallicFactor;
    ospMat->createChild("roughness", "float") = (float)pbr.roughnessFactor;

    // XXX will require texture tweaks to get closer to glTF spec, if needed
    // BLEND is always used, so OPAQUE can be achieved by setting all texture
    // texels alpha to 1.0.  OSPRay doesn't support alphaCutoff for MASK mode.
    if (mat.alphaMode == "OPAQUE")
      ospMat->createChild("opacity", "float") = 1.f;
    else if (mat.alphaMode == "BLEND")
      ospMat->createChild("opacity", "float") = 1.f;
    else if (mat.alphaMode == "MASK")
      ospMat->createChild("opacity", "float") = 1.f - (float) mat.alphaCutoff;

    // All textures *can* specify a texcoord other than 0.  OSPRay only
    // supports one set of texcoords (TEXCOORD_0).

    if (pbr.baseColorTexture.index != -1 &&
        pbr.baseColorTexture.texCoord == 0) {
      // Used as a color texture, must be sRGB space, not linear
      setOSPTexture(ospMat, "baseColor", pbr.baseColorTexture.index, false);
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
      ospMat->createChild("emissiveColor", "rgb") = emissiveColor;
    }

    if (mat.emissiveTexture.index != -1 && mat.emissiveTexture.texCoord == 0) {
      setOSPTexture(ospMat, "emissive", mat.emissiveTexture.index);
      WARN << "Material has emissiveTexture #" << mat.emissiveTexture.index
           << "\n";
    }

    return ospMat;
  }

  NodePtr GLTFData::createOSPTexture(const std::string &texParam,
                                     const tinygltf::Texture &tex,
                                     const bool preferLinear)
  {
    static auto nTex = 0;

    if (tex.source == -1)
      return nullptr;

    tinygltf::Image &img = model.images[tex.source];

    if (img.uri.length() < 256) {  // The uri can be a stream of data!
      img.name = FileName(img.uri).name();
    }
    auto texName = img.name + "_" + pad(std::to_string(nTex++)) + "_tex";

    //DEBUG << pad("", '.', 6) << "texture." + texName << "\n";
    //DEBUG << pad("", '.', 9) << "image name: " << img.name << "\n";
    //DEBUG << pad("", '.', 9) << "image width: " << img.width << "\n";
    //DEBUG << pad("", '.', 9) << "image height: " << img.height << "\n";
    //DEBUG << pad("", '.', 9) << "image component: " << img.component << "\n";
    //DEBUG << pad("", '.', 9) << "image bits: " << img.bits << "\n";
    //if (img.uri.length() < 256)  // The uri can be a stream of data!
    //  DEBUG << pad("", '.', 9) << "image uri: " << img.uri << "\n";
    //DEBUG << pad("", '.', 9) << "image bufferView: " << img.bufferView << "\n";
    //DEBUG << pad("", '.', 9) << "image data: " << img.image.size() << "\n";

    // XXX Only handle pre-loaded images for now
    if (img.image.empty()) {
      ERROR << "!!! no texture data!  Need to load it !!!\n";
      return nullptr;
    }

    auto ospTexNode = createNode(texParam, "texture_2d");
    auto &ospTex    = *ospTexNode->nodeAs<Texture2D>();

    ospTex["name"]  = texName;
    ospTex.size.x   = img.width;
    ospTex.size.y   = img.height;
    ospTex.channels = img.component;
    const bool hdr  = img.bits > 8;
    ospTex.depth    = hdr ? 4 : 1;

    // XXX handle different depths and channels!!!!
    if (ospTex.depth != 1)
      ERROR << "Not yet handled pixel depth: " << ospTex.depth << std::endl;
    if (ospTex.channels != 4)
      ERROR << "Not yet handled number of channels: " << ospTex.channels << std::endl;

    // XXX better way to do this?!
    std::vector<vec4uc> data(ospTex.size.x * ospTex.size.y);
    std::memcpy(data.data(), img.image.data(), img.image.size());

    ospTex.createChildData("data", data, vec2ul(img.width, img.height));

    auto texFormat =
        osprayTextureFormat(ospTex.depth, ospTex.channels, preferLinear);
    ospTex.createChild("format", "int", (int)texFormat);
    ospTex.child("format").setMinMax(
        (int)OSP_TEXTURE_RGBA8, (int)OSP_TEXTURE_R16);

    // XXX Check sampler wrap/clamp modes!!!
    auto texFilter =
        (tex.sampler != -1 && model.samplers[tex.sampler].magFilter ==
                                  TINYGLTF_TEXTURE_FILTER_NEAREST)
            ? OSP_TEXTURE_FILTER_NEAREST
            : OSP_TEXTURE_FILTER_BILINEAR;
    ospTex.createChild("filter", "int", (int)texFilter);
    ospTex.child("filter").setMinMax(
        (int)OSP_TEXTURE_FILTER_BILINEAR, (int)OSP_TEXTURE_FILTER_NEAREST);

    return ospTexNode;
  }

  void GLTFData::setOSPTexture(NodePtr ospMat,
                               const std::string &texParamBase,
                               int texIndex,
                               bool preferLinear)
  {
    // A disabled texture will have index = -1
    if (texIndex < 0)
      return;

    auto texParam = "map_" + texParamBase;
    auto ospTexNode =
        createOSPTexture(texParam, model.textures[texIndex], preferLinear);
    if (ospTexNode) {
      //auto &ospTex = *ospTexNode->nodeAs<Texture2D>();
      //DEBUG << pad("", '.', 3) << "        .setChild: " << texParam << "= "
      //     << ospTex.name() << "\n";
      //DEBUG << pad("", '.', 3) << "            "
      //     << "depth: " << ospTex.depth << " channels: " << ospTex.channels
      //     << " preferLinear: " << preferLinear << "\n";

      ospMat->add(ospTexNode);
    }
  }

  // GLTFmporter definitions //////////////////////////////////////////////////

  void glTFImporter::importScene()
  {
    // Create a root Transform/Instance off the Importer, under which to build
    // the import hierarchy
    std::string baseName = fileName.name() + "_rootXfm";
    auto rootNode = createNode(baseName, "Transform", affine3f{one});

    GLTFData gltf(rootNode, fileName);

    if (!gltf.parseAsset())
      return;

    gltf.animate = animate;
    gltf.createMaterials(*materialRegistry);
    gltf.createGeometries();
    gltf.buildScene();

    // load asset extensions as separate SG Asset-Info-nod
    gltf.loadAssetInfo(rootNode);

    if (animate){
      for (auto iter = gltf.g_allTimesteps.begin(); iter != gltf.g_allTimesteps.end(); ++iter){
        timesteps->push_back(iter->first);
      }
    }

    // Finally, add node hierarchy to importer parent
    add(rootNode);

    INFO << "finished import!\n";
  }

  }  // namespace sg
} // namespace ospray
