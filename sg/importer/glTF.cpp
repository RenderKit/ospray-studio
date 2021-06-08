// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Importer.h"
// tiny_gltf
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"
// rkcommon
#include "rkcommon/os/FileName.h"

#include "glTF/buffer_view.h"
#include "glTF/gltf_types.h"

#include "../scene/geometry/Geometry.h"
#include "../visitors/PrintNodes.h"
#include "../texture/Texture2D.h"
#include "../scene/Transform.h"
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
    GLTFData(NodePtr rootNode,
        const FileName &fileName,
        std::shared_ptr<sg::MaterialRegistry> _materialRegistry,
        std::vector<NodePtr> *_cameras,
        sg::FrameBuffer *_fb,
        NodePtr _currentImporter,
        InstanceConfiguration _ic)
        : fileName(fileName),
          rootNode(rootNode),
          materialRegistry(_materialRegistry),
          cameras(_cameras),
          fb(_fb),
          currentImporter(_currentImporter),
          ic(_ic)
    {}

   public:
    const FileName &fileName;

    bool parseAsset();
    void createMaterials();
    void createSkins();
    void finalizeSkins();
    void createGeometries();
    void createCameras();
    void createLights();
    void buildScene();
    void loadNodeInfo(const int nid, NodePtr sgNode);
    // load animations AFTER loading scene nodes and their transforms
    void createAnimations(std::vector<Animation> &);
    void applySceneBackground(NodePtr bgXfm);
    std::vector<NodePtr> lights;

   private:
    InstanceConfiguration ic;
    NodePtr currentImporter;
    NodePtr rootNode;
    sg::FrameBuffer *fb{nullptr};
    std::vector<NodePtr> *cameras{nullptr};
    std::vector<SkinPtr> skins;
    std::vector<std::vector<NodePtr>> ospMeshes;
    std::shared_ptr<sg::MaterialRegistry> materialRegistry;
    std::vector<NodePtr> sceneNodes; // lookup table glTF:nodeID -> NodePtr

    tinygltf::Model model;

    std::vector<NodePtr> ospMaterials;

    size_t baseMaterialOffset = 0; // set in createMaterials()

    void loadKeyframeInput(int accessorID, std::vector<float> &kfInput);

    void loadKeyframeOutput(std::vector<vec3f> &ts, std::vector<affine3f> &kfOutput, std::string &propertyName);

    void visitNode(NodePtr sgNode,
        const int nid,
        const int level); // XXX level is just for debug

    void applyNodeTransform(NodePtr, const tinygltf::Node &node);

    NodePtr createOSPMesh(
        const std::string &primBaseName, tinygltf::Primitive &primitive);

    NodePtr createOSPMaterial(const tinygltf::Material &material);

    NodePtr createOSPTexture(const std::string &texParam,
        const tinygltf::Texture &texture,
        const int colorChannel,
        const bool preferLinear);

    void setOSPTexture(NodePtr ospMaterial,
        const std::string &texParam,
        int texIndex,
        bool preferLinear = true)
    {
      setOSPTexture(ospMaterial,
          texParam,
          texIndex,
          4, // default, texture uses all channels
          preferLinear);
    }
    void setOSPTexture(NodePtr ospMaterial,
        const std::string &texParam,
        int texIndex,
        int colorChannel, // sampled channel R(0), G(1), B(2), A(3)
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

    return ret;
  }

  void GLTFData::applySceneBackground(NodePtr bgXfm)
  {
    auto background = model.extensions.find("BIT_scene_background")->second;
    auto &bgFileName = background.Get("background-uri").Get<std::string>();
    rkcommon::FileName bgTexture = fileName.path() + bgFileName;
    auto bgNode = createNode("background", "hdri");
    auto &map = bgNode->createChild("map", "texture_2d");
    map.nodeAs<sg::Texture2D>()->load(bgTexture, false, false);

    if (background.Has("rotation")) {
      const auto &r = background.Get("rotation").Get<tinygltf::Value::Array>();
      auto &rot = bgXfm->nodeAs<sg::Transform>()->child("rotation");
      rot = quaternionf(r[3].Get<double>(),
          r[0].Get<double>(),
          r[1].Get<double>(),
          r[2].Get<double>());
    }

    lights.push_back(bgNode);
    bgXfm->add(bgNode);
  }

  void GLTFData::loadNodeInfo(const int nid, NodePtr sgNode) {
    const tinygltf::Node &n = model.nodes[nid];

    auto assetObj = n.extensions.find("BIT_asset_info")->second;
    auto &asset = assetObj.Get("extensions").Get("BIT_asset_info");
    // auto &assetId = asset.Get("id").Get<std::string>();
    auto assetTitle = asset.Get("title").Get<std::string>();

    auto refLink = n.extensions.find("BIT_reference_link")->second;
    auto &refId = refLink.Get("id").Get<std::string>();
    auto refTitle = refLink.Get("title").Get<std::string>();
    sgNode->createChild("geomId", "string", refId);

    auto node = n.extensions.find("BIT_node_info")->second;
    auto &nodeId = node.Get("id").Get<std::string>();
    sgNode->createChild("instanceId", "string", nodeId);

    if (refTitle.empty())
      return;

    // node has asset Info and asset title should match refernce link title
    if (!assetTitle.empty() && assetTitle != refTitle) {
      std::cout << "mistmatch in asset information and reference link "
                << std::endl;
      return;
    }
    std::string refLinkFileName = refTitle + ".gltf";
    std::string refLinkFullPath = fileName.path() + refLinkFileName;
    rkcommon::FileName file(refLinkFullPath);
    std::cout << "Importing: " << file << std::endl;

    auto importer =
        std::static_pointer_cast<sg::Importer>(sg::getImporter(sgNode, file));
    if (importer) {
      importer->setMaterialRegistry(materialRegistry);
      auto parentImporter = currentImporter->nodeAs<sg::Importer>();

      auto cameraList = parentImporter->getCameraList();
      if (cameraList)
        importer->setCameraList(*cameraList);

      auto animationList = parentImporter->getAnimationList();
      if (animationList)
        importer->setAnimationList(*animationList);

      auto lightsManager = parentImporter->getLightsManager();
      importer->setLightsManager(lightsManager);

      auto fb = parentImporter->getFb();
      if (fb)
        importer->setFb(*fb);

      auto &pointSize = parentImporter->pointSize;
      importer->pointSize = pointSize;

      auto instanceConfig = parentImporter->getInstanceConfiguration();
      importer->setInstanceConfiguration(instanceConfig);

      importer->importScene();
    }
  }

  void GLTFData::createLights()
  {
    for (auto &l : model.lights) {
      static auto nLight = 0;
      auto lightName =
          l.name != "" ? l.name : "light_" + std::to_string(nLight++);
      auto lightType = l.type;
      NodePtr newLight;

      if (l.type == "directional")
        newLight = createNode(lightName, "distant");
      else if (l.type == "point")
        newLight = createNode(lightName, "sphere");
      else if (l.type == "spot") {
        newLight = createNode(lightName, "spot");
        auto outerConeAngle = (float)l.spot.outerConeAngle;
        auto innerConeAngle = (float)l.spot.innerConeAngle;
        newLight->createChild("openingAngle", "float", outerConeAngle);
        newLight->createChild(
            "penumbraAngle", "float", outerConeAngle - innerConeAngle);
      } else if (l.type == "hdri") {
        newLight = createNode(lightName, l.type);
        auto &hdriTex = newLight->createChild("map", "texture_2d");
        static rkcommon::FileName texFileName("");
        auto hdrFileName = l.extras.Get("map").Get<std::string>();
        texFileName = fileName.path() + hdrFileName;

        auto ast2d = hdriTex.nodeAs<sg::Texture2D>();
        ast2d->load(texFileName, false, false);
      } else
        newLight = createNode(lightName, l.type);

      auto lightColor =
          vec3f{(float)l.color[0], (float)l.color[1], (float)l.color[2]};
      newLight->createChild("color", "vec3f", lightColor);

      if (l.intensity)
        newLight->createChild("intensity", "float", (float)l.intensity);
      if (l.range)
        std::cout << "Range value for light is not supported yet" << std::endl;

      // TODO:: Address extras property on lights

      lights.push_back(newLight);
    }
  }

  void GLTFData::createMaterials()
  {
    // DEBUG << "Create Materials\n";

    // Create materials and textures
    // (adding 1 for default material)
    ospMaterials.reserve(model.materials.size() + 1);

    // "default" material for glTF '-1' index (no material)
    ospMaterials.emplace_back(createNode(fileName.name() + ":default", "obj"));

    // Create materials (also sets textures to material params)
    for (const auto &material : model.materials) {
      ospMaterials.push_back(createOSPMaterial(material));
    }

    baseMaterialOffset = materialRegistry->baseMaterialOffSet();
    for (auto m : ospMaterials)
      materialRegistry->add(m);
  }

  void GLTFData::createCameras()
  {
    cameras->reserve(model.cameras.size());
    NodePtr sgCamera;

    for (auto &m : model.cameras) {
      static auto nCamera = 0;
      auto cameraName = m.name;
      if(cameraName == "")
       cameraName = "camera_" + std::to_string(nCamera);
      if (m.type == "perspective") {
        sgCamera = createNode(cameraName, "camera_perspective");

        // convert radians to degrees for vertical FOV
        float fovy = (float)m.perspective.yfov * (180.f / (float)pi);

        sgCamera->createChild("fovy", "float", fovy);
        sgCamera->createChild("nearClip", "float", (float)m.perspective.znear);
        if (m.perspective.aspectRatio > 0)
          sgCamera->createChild("aspect", "float", (float)m.perspective.aspectRatio);

        if (m.perspective.extras.Has("focusDistance"))
          sgCamera->createChild("focusDistance",
                                "float",
                                (float)m.perspective.extras.Get("focusDistance").GetNumberAsDouble());

        if (m.perspective.extras.Has("apertureRadius"))
          sgCamera->createChild("apertureRadius",
                                "float",
                                (float)m.perspective.extras.Get("apertureRadius").GetNumberAsDouble());
      } else {
        sgCamera = createNode(cameraName, "camera_orthographic");
        sgCamera->createChild("height", "float", (float)m.orthographic.ymag);

        // calculate orthographic aspect with horizontal and vertical maginifications
        float aspect = (float)m.orthographic.xmag / m.orthographic.ymag;
        sgCamera->createChild("aspect", "float", aspect);
        sgCamera->createChild("nearClip", "float", (float)m.orthographic.znear);
      }

      // check if camera has EXT_cameras_sensor
      if (m.extensions.find("EXT_cameras_sensor") != m.extensions.end()
          && sgCamera->subType() == "camera_perspective") {
        auto ext = m.extensions["EXT_cameras_sensor"];
        float len_x, len_y{0.f};

        if (ext.Has("imageSensor")) {
          auto imageSensor = ext.Get("imageSensor");
          auto pixels = imageSensor.Get("pixels").Get<tinygltf::Value::Array>();
          auto pixelSize =
              imageSensor.Get("pixelSize").Get<tinygltf::Value::Array>();
          int x = pixels[0].Get<int>();
          int y = pixels[1].Get<int>();
          fb->child("size").setValue(vec2i(x, y));
          len_x = x * (float)pixelSize[0].Get<double>();
          len_y = y * (float)pixelSize[1].Get<double>();
          sgCamera->child("aspect").setValue(len_x / len_y);
        }

        if (ext.Has("lens")) {
          auto lens = ext.Get("lens");
          float chamberConstant = (float)lens.Get("focalLength").Get<double>();
          float fovy = atan(len_y / (2 * chamberConstant)) * 360.f / (float)pi;
          sgCamera->child("fovy").setValue(fovy);

          float apertureRadius =
              (float)lens.Get("apertureRadius").Get<double>();
          sgCamera->child("apertureRadius").setValue(apertureRadius);

          float focusDistance = (float)lens.Get("focusDistance").Get<double>();
          sgCamera->child("focusDistance").setValue(focusDistance);
          
          auto centerPointShift =
              lens.Get("centerPointShift").Get<tinygltf::Value::Array>();
          float x_shift = (float)centerPointShift[0].Get<double>();
          float y_shift = (float)centerPointShift[1].Get<double>();
          vec2f imageStart = vec2f(x_shift, y_shift)
              + sgCamera->child("imageStart").valueAs<vec2f>();
          vec2f imageEnd = vec2f(x_shift, y_shift)
              + sgCamera->child("imageEnd").valueAs<vec2f>();
          sgCamera->child("imageStart").setValue(imageStart);
          sgCamera->child("imageEnd").setValue(imageEnd);
        }
      }
      sgCamera->createChild("cameraId", "int", ++nCamera);
      sgCamera->child("cameraId").setSGOnly();
      cameras->push_back(sgCamera);
    }
  }

  void GLTFData::createSkins()
  {
    skins.reserve(model.skins.size());
    for (auto &skin : model.skins) {
      auto &ibm = model.accessors[skin.inverseBindMatrices];
      if (ibm.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT
          && ibm.type == TINYGLTF_TYPE_MAT4) {
        skins.emplace_back(new Skin);
        Accessor<float[16]> m(ibm, model);
        auto &xfm = skins.back()->inverseBindMatrices;
        xfm.reserve(m.size());
        for (size_t i = 0; i < m.size(); i++)
          xfm.emplace_back(vec3f(m[i][0 * 4], m[i][0 * 4 + 1], m[i][0 * 4 + 2]),
              vec3f(m[i][1 * 4], m[i][1 * 4 + 1], m[i][1 * 4 + 2]),
              vec3f(m[i][2 * 4], m[i][2 * 4 + 1], m[i][2 * 4 + 2]),
              vec3f(m[i][3 * 4], m[i][3 * 4 + 1], m[i][3 * 4 + 2]));
      } else {
        skins.push_back(nullptr);
        WARN << "unsupported inverseBindMatrices\n";
      }
    }
  }

  void GLTFData::finalizeSkins()
  {
    for (size_t i = 0; i < model.skins.size(); i++)
      if (skins[i]) {
        auto &joints = skins[i]->joints;
        auto &inJoints = model.skins[i].joints;
        joints.reserve(inJoints.size());
        for (auto i : inJoints)
          joints.push_back(sceneNodes[i]);
      }
  }

  void GLTFData::createGeometries()
  {
    // DEBUG << "Create Geometries\n";

    ospMeshes.reserve(model.meshes.size());
    for (auto &m : model.meshes) {
      static auto nModel = 0;
      auto modelName = m.name + "_" + pad(std::to_string(nModel++));
      std::vector<NodePtr> mesh_subsets;

      for (auto &prim : m.primitives) {
        static auto nSubset = 0;
        modelName = modelName + "_" + pad(std::to_string(nSubset++));
        auto mesh = createOSPMesh(modelName, prim);
        mesh_subsets.push_back(mesh);
      }
      ospMeshes.push_back(mesh_subsets);
    }
  }

  // create animation channels and load sampler information
  // link nodes with the channels that animate them in animatedNodes map
  void GLTFData::createAnimations(std::vector<Animation> &animations)
  {
    for (auto &a : model.animations) {
      animations.emplace_back(a.name);
      auto &animation = animations.back();
      // tracks
      for (auto &c : a.channels) {
        if (c.target_node < 0)
          continue;
        auto &s = a.samplers[c.sampler];
        auto &inputAcc = model.accessors[s.input];
        if (inputAcc.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
          continue;

        AnimationTrackBase *track{nullptr};
        auto &valueAcc = model.accessors[s.output];
        if (c.target_path == "translation" || c.target_path == "scale") {
          // translation and scaling will always have complete dataType of vec3f
          Accessor<vec3f> value(valueAcc, model);
          auto *t = new AnimationTrack<vec3f>();
          track = t;
          t->values.reserve(value.size());
          for (size_t i = 0; i < value.size(); ++i)
            t->values.push_back(value[i]);
        } else if (c.target_path == "rotation") {
          if (valueAcc.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
            WARN
                << "animation rotation is not in 'float' format; conversion not implemented yet"
                << std::endl;
            continue;
          }
          Accessor<vec4f> value(valueAcc, model);
          auto *t = new AnimationTrack<quaternionf>();
          track = t;
          t->values.reserve(value.size());
          for (size_t i = 0; i < value.size(); ++i) {
            const auto &v = value[i];
            t->values.push_back(quaternionf(v.w, v.x, v.y, v.z));
          }
        } else if (c.target_path == "weights") {
          WARN << "animating weights of morph targets not implemented yet"
               << std::endl;
          continue;
        }
        if (!track)
          continue;

        track->interpolation = InterpolationMode::STEP;
        if (s.interpolation == "LINEAR")
          track->interpolation = InterpolationMode::LINEAR;
        if (s.interpolation == "CUBICSPLINE")
          track->interpolation = InterpolationMode::CUBIC;

        track->target =
            sceneNodes[c.target_node]->child(c.target_path).shared_from_this();
        
        if(sceneNodes[c.target_node]->hasChild("animateCamera"))
          sceneNodes[c.target_node]->child("animateCamera").setValue(true);

        Accessor<float> time(inputAcc, model);
        track->times.reserve(time.size());
        for (size_t i = 0; i < time.size(); ++i)
          track->times.push_back(time[i]);

        animation.addTrack(track);
      }
    }
  }

  void GLTFData::buildScene()
  {
    if (model.scenes.empty())
      return;

    // background with custom BIT_scene_background extension is applied on scene-level
    if(model.extensions.find("BIT_scene_background") != model.extensions.end()) {
      auto bgXfm =
        createNode("bgXfm", "transform");
      applySceneBackground(bgXfm);
      rootNode->add(bgXfm);
    }

    const auto defaultScene = std::max(0, model.defaultScene);

    if (model.scenes[defaultScene].nodes.empty())
      return;

    // DEBUG << "Build Scene\n";
    sceneNodes.resize(model.nodes.size());

    // Process all nodes in default scene
    auto &scene = model.scenes[defaultScene];
    for (const auto &nid : scene.nodes) {
      // DEBUG << "... Top Node (#" << nid << ")\n";
      // recursively process node hierarchy
      visitNode(rootNode, nid, 1);
    }

    // attach (skinned) meshes to correct transform node
    size_t nIdx = 0;
    for (auto n : model.nodes) {
      if (n.mesh != -1 && sceneNodes[nIdx]) {
        auto &ospMesh = ospMeshes[n.mesh];
        auto &targetNode = sceneNodes[nIdx];
        if (n.skin != -1) {
          if (model.skins[n.skin].skeleton != -1)
            targetNode = sceneNodes[model.skins[n.skin].skeleton];
          for (auto &m : ospMesh) {
            const auto &geom = m->nodeAs<Geometry>();
            geom->skin = skins[n.skin];
            geom->skeletonRoot = targetNode;
          }
        }
        for (auto &m : ospMesh)
          targetNode->add(m);
      }
      nIdx++;
    }
  }

  void GLTFData::visitNode(NodePtr sgNode,
      const int nid,
      const int level) // XXX just for debug
  {
    const tinygltf::Node &n = model.nodes[nid];
    // DEBUG << pad("", '.', 3 * level) << nid << ":" << n.name
    //     << " (children:" << n.children.size() << ")\n";

    static auto nNode = 0;
    auto nodeName = n.name + "_" + pad(std::to_string(nNode++));
    // DEBUG << pad("", '.', 3 * level) << "..node." + nodeName << "\n";
    // DEBUG << pad("", '.', 3 * level) << "....xfm\n";

    auto newXfm =
        createNode(nodeName + "_xfm_" + std::to_string(level), "transform");
    if (ic == 1)
      newXfm->child("dynamicScene").setValue(true);
    else if (ic == 2)
      newXfm->child("compactMode").setValue(true);
    else if (ic ==3)
      newXfm->child("robustMode").setValue(true);

    sceneNodes[nid] = newXfm;
    sgNode->add(newXfm);
    applyNodeTransform(newXfm, n);

    // create child animate camera for all camera nodes added to scene hierarchy,
    // bool value is set during createAnimation when appropriate target xfm is found
    if(n.camera != -1 && cameras != nullptr) {
      auto &listCameras = *cameras;
      auto &camera = listCameras[n.camera];
      newXfm->add(camera);
      newXfm->createChild("animateCamera", "bool", false);
    }

    sgNode = newXfm;

    // while parsing assets from BIT-TS look for BIT_asset_info to add to
    // assetCatalogue
    // followed by BIT_node_info for adding that particular instance
    // followed by BIT_reference_link to load that reference
    if (n.extensions.find("BIT_asset_info") != n.extensions.end() &&
        n.extensions.find("BIT_node_info") != n.extensions.end() &&
        n.extensions.find("BIT_reference_link") != n.extensions.end())
      loadNodeInfo(nid, sgNode);

    // KHR_lights_punctual extension info on nodes
    if(n.extensions.find("KHR_lights_punctual") != n.extensions.end()) {
      // defines light orientation
      auto lightIndex = n.extensions.find("KHR_lights_punctual")->second.Get("light").GetNumberAsInt();
      auto lightNode = lights[lightIndex];
      sgNode->add(lightNode);
    }

    // recursively process children nodes
    for (const auto &cid : n.children) {
      if (sgNode->type() != NodeType::LIGHT)
        visitNode(sgNode, cid, level + 1);
    }
  }

  void GLTFData::applyNodeTransform(NodePtr xfmNode, const tinygltf::Node &n)
  {
    if (!n.matrix.empty()) {
      // Matrix must decompose to T/R/S, no skew/shear
      const auto &m = n.matrix;
      xfmNode->setValue(
          affine3f(vec3f(m[0 * 4 + 0], m[0 * 4 + 1], m[0 * 4 + 2]),
              vec3f(m[1 * 4 + 0], m[1 * 4 + 1], m[1 * 4 + 2]),
              vec3f(m[2 * 4 + 0], m[2 * 4 + 1], m[2 * 4 + 2]),
              vec3f(m[3 * 4 + 0], m[3 * 4 + 1], m[3 * 4 + 2])));
    } else {
      if (!n.scale.empty()) {
        const auto &s = n.scale;
        xfmNode->child("scale") = vec3f(s[0], s[1], s[2]);
      }
      if (!n.rotation.empty()) {
        const auto &r = n.rotation;
        xfmNode->child("rotation") = quaternionf(r[3], r[0], r[1], r[2]);
      }
      if (!n.translation.empty()) {
        const auto &t = n.translation;
        xfmNode->child("translation") = vec3f(t[0], t[1], t[2]);
      }
    }
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

    std::shared_ptr<Geometry> ospGeom = nullptr;

    // create appropriate Geometry node
    if (prim.mode == TINYGLTF_MODE_TRIANGLES) 
      ospGeom =
          createNodeAs<Geometry>(primName + "_object", "geometry_triangles");
    else if (prim.mode == TINYGLTF_MODE_POINTS) {
      // points as spheres
      ospGeom =
          createNodeAs<Geometry>(primName + "_object", "geometry_spheres");
    } else {
      ERROR << "Unsupported primitive mode! File must contain only "
        "triangles or points\n";
      throw std::runtime_error(
          "Unsupported primitive mode! Only triangles are supported");
    }


#if 1  // XXX: Generalize these with full component support!!!
       // In : 1,2,3,4 ubyte, ubyte(N), ushort, ushort(N), uint, float
       // Out:   2,3,4 int, float

    if (prim.indices >  -1) {
      auto &vi = ospGeom->vi;
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
    }

    // Colors: vec3/vec4 float/ubyte(N)/ushort(N) RGB/RGBA
    // accessor->normalized
    auto fnd = prim.attributes.find("COLOR_0");
    if (fnd != prim.attributes.end()) {
      auto &vc = ospGeom->vc;
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
        // DEBUG << pad("", '.', 6) << "prim. Correcting Alpha\n";
        std::transform(vc.begin(), vc.end(), vc.begin(), [](vec4f c) {
          return vec4f(c.x, c.y, c.z, 1.f);
        });
      }
    }

    // TexCoords: vec2 float/ubyte(N)/ushort(N)
    // accessor->normalized
    // Note: GLTF can have texture coordinates [0,1] used by different
    // textures.  Only supporting TEXCOORD_0
    fnd = prim.attributes.find("TEXCOORD_0");
    if (fnd != prim.attributes.end()) {
      auto &vt = ospGeom->vt;
      Accessor<vec2f> uv_accessor(model.accessors[fnd->second], model);
      vt.reserve(uv_accessor.size());
      for (size_t i = 0; i < uv_accessor.size(); ++i)
        vt.emplace_back(uv_accessor[i]);
    }
#if 0 // OSPRay does not support a second uv texcoord set
      // But, specifying it here isn't a problem.  Only on attempted usage.
    fnd = prim.attributes.find("TEXCOORD_1");
    if (fnd != prim.attributes.end()) {
      WARN << "gltf found TEXCOORD_1 attribute.  Not supported...\n"
           << std::endl;
    }
#endif
#endif

    // Add attribute arrays to mesh
    if (ospGeom->subType() == "geometry_triangles") {
      // Positions: vec3f
      Accessor<vec3f> pos_accessor(
          model.accessors[prim.attributes["POSITION"]], model);
      const auto vertices = pos_accessor.size();
      ospGeom->skinnedPositions.reserve(vertices);
      for (size_t i = 0; i < vertices; ++i)
        ospGeom->skinnedPositions.emplace_back(pos_accessor[i]);
      ospGeom->createChildData(
          "vertex.position", ospGeom->skinnedPositions, true);

      // Normals: vec3f
      fnd = prim.attributes.find("NORMAL");
      if (fnd != prim.attributes.end()) {
        Accessor<vec3f> normal_accessor(model.accessors[fnd->second], model);
        if (vertices == normal_accessor.size()) {
          ospGeom->skinnedNormals.reserve(vertices);
          for (size_t i = 0; i < vertices; ++i)
            ospGeom->skinnedNormals.emplace_back(normal_accessor[i]);
          ospGeom->createChildData(
              "vertex.normal", ospGeom->skinnedNormals, true);
        } else
          WARN << "mismatching NORMAL size\n";
      }

      ospGeom->createChildData("index", ospGeom->vi, true);
      if (vertices == ospGeom->vc.size())
        ospGeom->createChildData("vertex.color", ospGeom->vc, true);
      else if (!ospGeom->vc.empty())
        WARN << "mismatching COLOR_0 size\n";
      if (vertices == ospGeom->vt.size())
        ospGeom->createChildData("vertex.texcoord", ospGeom->vt, true);
      else if (!ospGeom->vt.empty())
        WARN << "mismatching TEXCOORD_0 size\n";

      // skinning, XXX for now only for triangles
      ospGeom->weightsPerVertex = 0;
      int stride = 0;
      for (int set = 2; set >= 0; set--) {
        auto fndj = prim.attributes.find("JOINTS_" + std::to_string(set));
        auto fndw = prim.attributes.find("WEIGHTS_" + std::to_string(set));
        if (fndj == prim.attributes.end() || fndw == prim.attributes.end())
          continue;
        if (ospGeom->weightsPerVertex == 0) { // init only for largest found set
          stride = set + 1;
          ospGeom->weightsPerVertex = stride * 4;
          ospGeom->joints.resize(vertices * ospGeom->weightsPerVertex);
          ospGeom->weights.resize(vertices * ospGeom->weightsPerVertex);
          ospGeom->positions = ospGeom->skinnedPositions;
          ospGeom->normals = ospGeom->skinnedNormals;
        }
        auto &joints = model.accessors[fndj->second];
        if (vertices != joints.count) {
          WARN << "mismatching JOINTS_" << set << " size\n";
          continue;
        }
        vec4us *geomJoints = (vec4us *)ospGeom->joints.data() + set;
        bool isVec4 = joints.type == TINYGLTF_TYPE_VEC4;
        if (isVec4
            && joints.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
          Accessor<vec4uc> joint(joints, model);
          for (size_t i = 0; i < joint.size(); ++i)
            geomJoints[i * stride] = joint[i];
        } else if (isVec4
            && joints.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
          Accessor<vec4us> joint(joints, model);
          for (size_t i = 0; i < joint.size(); ++i)
            geomJoints[i * stride] = joint[i];
        } else
          WARN << "invalid JOINTS_" << set << std::endl;

        auto &weights = model.accessors[fndw->second];
        if (vertices != weights.count) {
          WARN << "mismatching WEIGHTS_" << set << " size\n";
          continue;
        }
        vec4f *geomWeights = (vec4f *)ospGeom->weights.data() + set;
        isVec4 = weights.type == TINYGLTF_TYPE_VEC4;
        if (isVec4
            && weights.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
          Accessor<vec4uc> weight(weights, model);
          for (size_t i = 0; i < weight.size(); ++i)
            geomWeights[i * stride] = weight[i] / 255.0f;
        } else if (isVec4
            && weights.componentType
                == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
          Accessor<vec4us> weight(weights, model);
          for (size_t i = 0; i < weight.size(); ++i)
            geomWeights[i * stride] = weight[i] / 65535.0f;
        } else if (isVec4
            && weights.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
          Accessor<vec4f> weight(weights, model);
          for (size_t i = 0; i < weight.size(); ++i)
            geomWeights[i * stride] = weight[i];
        } else
          WARN << "invalid WEIGHTS_" << set << std::endl;
      }
    } else if (ospGeom->subType() == "geometry_spheres") {

      // Positions: vec3f
      Accessor<vec3f> pos_accessor(
          model.accessors[prim.attributes["POSITION"]], model);
      ospGeom->skinnedPositions.reserve(pos_accessor.size());
      for (size_t i = 0; i < pos_accessor.size(); ++i)
        ospGeom->skinnedPositions.emplace_back(pos_accessor[i]);
      ospGeom->createChildData("sphere.position", ospGeom->skinnedPositions, true);

      // glTF doesn't specify point radius.
      ospGeom->createChild("radius", "float", 0.005f);

      if (!ospGeom->vc.empty()) {
        ospGeom->createChildData("color", ospGeom->vc, true);
        // color will be added to the geometric model, it is not directly part
        // of the spheres primitive
        ospGeom->child("color").setSGOnly();
      }
      if (!ospGeom->vt.empty())
        ospGeom->createChildData("sphere.texcoord", ospGeom->vt, true);
    }

    if (ospGeom) {
      // add one for default, "no material" material
      auto materialID = prim.material + 1 + baseMaterialOffset;
      ospGeom->mIDs.resize(ospGeom->skinnedPositions.size(), materialID);
      ospGeom->createChildData("material", ospGeom->mIDs, true);
      ospGeom->child("material").setSGOnly();
    }

    return ospGeom;
  }

  NodePtr GLTFData::createOSPMaterial(const tinygltf::Material &mat)
  {
    static auto nMat = 0;
    auto matName     = mat.name + "_" + pad(std::to_string(nMat++));
    // DEBUG << pad("", '.', 3) << "material." + matName << "\n";
    // DEBUG << pad("", '.', 3) << "        .alphaMode:" << mat.alphaMode
    //       << "\n";
    // DEBUG << pad("", '.', 3)
    //       << "        .alphaCutoff:" << (float)mat.alphaCutoff << "\n";

    auto &pbr = mat.pbrMetallicRoughness;

    // DEBUG << pad("", '.', 3)
    //     << "        .baseColorFactor.alpha:" << (float)pbr.baseColorFactor[4]
    //     << "\n";

    auto emissiveColor = vec3f(
        mat.emissiveFactor[0], mat.emissiveFactor[1], mat.emissiveFactor[2]);

    // We can emulate a constant colored emissive texture
    // XXX this is a workaround
    auto constColor = false;
    if (emissiveColor != vec3f(0.f) && mat.emissiveTexture.index != -1) {
      const auto &tex = model.textures[mat.emissiveTexture.index];
      const auto &img = model.images[tex.source];
      if (img.image.size() > 0) {
        constColor = true;
        const auto *data = img.image.data();

        const vec3f color0 = vec3f(data[0], data[1], data[2]);
        auto i = 1;
        WARN << "Material emissiveTexture #" << mat.emissiveTexture.index;
        WARN << std::endl;
        WARN << "   color0 : " << color0 << std::endl;
        while (constColor && (i < img.width * img.height)) {
          const vec3f color =
            vec3f(data[4 * i + 0], data[4 * i + 1], data[4 * i + 2]);
          if (color0 != color) {
            WARN << "   color @ " << i << " : " << color << std::endl;
            WARN << "   !!! non constant color, skipping emissive" << std::endl;
            constColor = false;
            break;
          }
          i++;
        }
      }
    }

    if ((emissiveColor == vec3f(0.f)) || (constColor == false)) {
        auto ospMat = createNode(matName, "principled");
        ospMat->createChild("baseColor", "rgb") = vec3f(pbr.baseColorFactor[0],
            pbr.baseColorFactor[1],
            pbr.baseColorFactor[2]);
        ospMat->createChild("metallic", "float")  = (float)pbr.metallicFactor;
        ospMat->createChild("roughness", "float") = (float)pbr.roughnessFactor;

        // Unspec'd defaults
        ospMat->createChild("diffuse", "float") = 1.0f;
        ospMat->createChild("ior", "float") = 1.5f;

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
        if (pbr.baseColorTexture.texCoord != 0
            || pbr.metallicRoughnessTexture.texCoord != 0
            || mat.normalTexture.texCoord != 0) {
          WARN << "gltf found TEXCOOR_1 attribute.  Not supported...\n";
          WARN << std::endl;
        }

        if (pbr.baseColorTexture.index != -1
            && pbr.baseColorTexture.texCoord == 0) {
          // Used as a color texture, must be sRGB space, not linear
          setOSPTexture(ospMat, "baseColor", pbr.baseColorTexture.index, false);
        }

      if (pbr.metallicRoughnessTexture.index != -1 &&
          pbr.metallicRoughnessTexture.texCoord == 0) {
        // metallic in Blue(2) channel, roughness in Green(1)
        setOSPTexture(
            ospMat, "metallic", pbr.metallicRoughnessTexture.index, 2);
        setOSPTexture(
            ospMat, "roughness", pbr.metallicRoughnessTexture.index, 1);
      }

      if (mat.normalTexture.index != -1 && mat.normalTexture.texCoord == 0) {
        // NormalTextureInfo() : index(-1), texCoord(0), scale(1.0) {}
        setOSPTexture(ospMat, "normal", mat.normalTexture.index);
        ospMat->createChild("normal", "float", (float)mat.normalTexture.scale);
      }

      // Material Extensions
      const auto &exts = mat.extensions;
      if (exts.find("KHR_materials_pbrSpecularGlossiness") != exts.end()) {
        // https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_materials_pbrSpecularGlossiness
        auto params = exts.find("KHR_materials_pbrSpecularGlossiness")->second;

        // diffuseFactor: The reflected diffuse factor of the material.
        // default:[1.0,1.0,1.0,1.0]
        vec3f diffuse = vec3f(1.f);
        float opacity = 1.f;
        if (params.Has("diffuseFactor")) {
          std::vector<tinygltf::Value> dv =
              params.Get("diffuseFactor").Get<tinygltf::Value::Array>();
          diffuse = vec3f(
              dv[0].Get<double>(), dv[1].Get<double>(), dv[2].Get<double>());
          opacity = (float)dv[3].Get<double>();
        }
        ospMat->createChild("baseColor", "rgb") = diffuse;
        ospMat->createChild("opacity", "float") = opacity;

        // diffuseTexture: The diffuse texture.
        if (params.Has("diffuseTexture")) {
          // RGB or RGBA
          setOSPTexture(ospMat,
              "baseColor",
              params.Get("diffuseTexture").Get("index").Get<int>(), false);
        }

        // specularFactor: The specular RGB color of the material.
        // default:[1.0,1.0,1.0]
        vec3f specular = vec3f(1.f);
        if (params.Has("specularFactor")) {
          std::vector<tinygltf::Value> sv =
              params.Get("specularFactor").Get<tinygltf::Value::Array>();
          specular = vec3f(
              sv[0].Get<double>(), sv[1].Get<double>(), sv[2].Get<double>());
        }
        // XXX this can't simply overwrite baseColor
        ospMat->createChild("baseColor", "rgb") = specular;

        // glossinessFactor: The glossiness or smoothness of the material.
        // default:1.0
        float gloss = 1.f;
        if (params.Has("glossinessFactor")) {
          gloss = (float)params.Get("glossinessFactor").Get<double>();
        }
        ospMat->createChild("roughness", "float") = 1.f - gloss;

        // specularGlossinessTexture: The specular-glossiness texture.
#if 0
        // XXX texture isn't simply RGBA!!!
        // texture containing the sRGB encoded specular color and the linear
        // glossiness value (A).
        if (params.Has("specularGlossinessTexture")) {
          // XXX this can't simply overwrite baseColor
          setOSPTexture(ospMat,
              "baseColor",
              params.Get("specularGlossinessTexture").Get("index").Get<int>(),
              false);
        }
#endif
      }

      if (exts.find("KHR_materials_clearcoat") != exts.end()) {
        // https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_materials_clearcoat
        auto params = exts.find("KHR_materials_clearcoat")->second;

        // clearcoatFactor: The clearcoat layer intensity. default:0.0
        float coat = 0.f;
        if (params.Has("clearcoatFactor"))
          coat = (float)params.Get("clearcoatFactor").Get<double>();
        ospMat->createChild("coat", "float") = coat;
        ospMat->createChild("coatThickness", "float") = 1.f; // (not in spec)

        // clearcoatRoughnessFactor: The clearcoat layer roughness.
        // default:0.0
        float coatRoughness = 0.f;
        if (params.Has("clearcoatRoughnessFactor"))
          coatRoughness =
              (float)params.Get("clearcoatRoughnessFactor").Get<double>();
        ospMat->createChild("coatRoughness", "float") = coatRoughness;

        // clearcoatTexture: The clearcoat layer intensity texture.
        // clearcoatRoughnessTexture: The clearcoat layer roughness texture.
#if 1
        // XXX textures aren't simply RGB!!!
        // clearcoat = clearcoatFactor * clearcoatTexture.r
        // clearcoatRoughness = clearcoatRoughnessFactor *
        //                      clearcoatRoughnessTexture.g
        if (params.Has("clearcoatTexture")) {
          // Red channel
          setOSPTexture(ospMat,
              "coat",
              params.Get("clearcoatTexture").Get("index").Get<int>(), 0);
        }
        if (params.Has("clearcoatRoughnessTexture")) {
          // Green channel
          setOSPTexture(ospMat,
              "coatRoughness",
              params.Get("clearcoatRoughnessTexture").Get("index").Get<int>(),
              1);
        }
#endif

        // clearcoatNormalTexture: The clearcoat normal map texture.
        if (params.Has("clearcoatNormalTexture")) {
          setOSPTexture(ospMat,
              "coatNormal",
              params.Get("clearcoatNormalTexture").Get("index").Get<int>());
        }
      }

      if (exts.find("KHR_materials_transmission") != exts.end()) {
        // https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_materials_transmission
        auto params = exts.find("KHR_materials_transmission")->second;

        // (this extension deals exclusively with infinitely thin surfaces)
        ospMat->createChild("thin", "bool") = true;
        ospMat->createChild("transmissionDepth", "float") = 1.f; // OSPRay xtra

        // transmissionFactor: The base percentage of light that is transmitted
        // through the surface. Default:0.0
        float transmission = 0.f;
        if (params.Has("transmissionFactor"))
          transmission = (float)params.Get("transmissionFactor").Get<double>();
        ospMat->createChild("transmission", "float") = transmission;

        vec3f tinting = vec3f(pbr.baseColorFactor[0],
            pbr.baseColorFactor[1],
            pbr.baseColorFactor[2]);
        ospMat->createChild("transmissionColor", "rgb") = tinting;

        // Use the baseColorTexture to also tint the transmissionColor
        // otherwise, any texture is just a surface color.
        // (allows for experiments with thickness (thin = false).
        if (pbr.baseColorTexture.index != -1
            && pbr.baseColorTexture.texCoord == 0) {
          // Used as a color texture, must be sRGB space, not linear
          setOSPTexture(ospMat,
              "transmissionColor", pbr.baseColorTexture.index, false);
        }

        // transmissionTexture: A texture that defines the transmission
        // percentage of the surface, stored in the R channel. This will be
        // multiplied by transmissionFactor.
        if (params.Has("transmissionTexture")) {
          setOSPTexture(ospMat,
              "transmission",
              params.Get("transmissionTexture").Get("index").Get<int>(), 0);
        }
      }

      if (exts.find("KHR_materials_sheen") != exts.end()) {
        // https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_materials_sheen/
        auto params = exts.find("KHR_materials_sheen")->second;

        // sheen weight (not in spec)
        ospMat->createChild("sheen", "float") = 1.f;

        // sheenColorFactor: The sheen color in linear space
        // default:[0.0, 0.0, 0.0]
        vec3f sheen = vec3f(1.f);
        if (params.Has("sheenColorFactor")) {
          std::vector<tinygltf::Value> sv =
              params.Get("sheenColorFactor").Get<tinygltf::Value::Array>();
          sheen = vec3f(
              sv[0].Get<double>(), sv[1].Get<double>(), sv[2].Get<double>());
        }
        ospMat->createChild("sheenColor", "rgb") = sheen;

        // sheenColorTexture: The sheen color (sRGB).
        if (params.Has("sheenColorTexture")) {
          setOSPTexture(ospMat,
              "sheen",
              params.Get("sheenColorTexture").Get("index").Get<int>());
        }

        // sheenRoughnessFactor: The sheen roughness. default:0.0
        float sheenRoughness = 0.f;
        if (params.Has("sheenRoughnessFactor")) {
          sheenRoughness =
              (float)params.Get("sheenRoughnessFactor").Get<double>();
        }
        ospMat->createChild("sheenRoughness", "float") = sheenRoughness;

        // sheenRoughnessTexture: The sheen roughness (Alpha) texture.
        if (params.Has("sheenRoughnessTexture")) {
          setOSPTexture(ospMat,
              "sheenRoughness",
              params.Get("sheenRoughnessTexture").Get("index").Get<int>(), 3);
        }
      }

      if (exts.find("KHR_materials_ior") != exts.end()) {
        // Experimental https://github.com/KhronosGroup/glTF/pull/1718
        auto params = exts.find("KHR_materials_ior")->second;

        // ior: The index of refraction. default:1.5
        float ior = 1.5f;
        if (params.Has("ior")) {
          ior = (float)params.Get("ior").Get<double>();
        }
        ospMat->createChild("ior", "float") = ior;

        // iorTexture: A greyscale texture that defines the index of refraction
        // as 1/ior.
        // XXX the experimental spec says that texture holds 1/ior values!!!
        // Nothing to test this with, thus far
      }

      return ospMat;

    } else {
      // XXX TODO Principled material doesn't have emissive params yet
      // So, use a luminous instead
      auto ospMat = createNode(matName, "luminous");

      if (emissiveColor != vec3f(0.f)) {
        ospMat->createChild("color", "rgb") = emissiveColor;
        ospMat->createChild("intensity", "float") =
            20.f; // XXX what's good default intensity?

        // Already checked for constant color above.
        if (mat.emissiveTexture.index != -1) {
          const auto &tex = model.textures[mat.emissiveTexture.index];
          const auto &img = model.images[tex.source];
          const auto *data = img.image.data();
          const vec3f color0 = vec3f(data[0], data[1], data[2]);
          WARN << "   name: " << img.name << std::endl;
          WARN << "   img: (" << img.width << ", " << img.height << ")";
          WARN << std::endl;
          WARN << "   emulating with solid color : " << color0 << std::endl;
          ospMat->child("color") = emissiveColor * (color0 / 255.f);
        }
      }

#if 0 // OSPRay Luminous doesn't support textured params yet.
      if (mat.emissiveTexture.texCoord != 0) {
        WARN << "gltf found TEXCOOR_1 attribute.  Not supported...\n"
          << std::endl;
      }

      if (mat.emissiveTexture.index != -1 &&
          mat.emissiveTexture.texCoord == 0) {
        // EmissiveTextureInfo() : index(-1), texCoord(0) {}
        setOSPTexture(ospMat, "color", mat.emissiveTexture.index, false);
        WARN << "Material has emissiveTexture #" << mat.emissiveTexture.index
             << "\n";
      }
#endif

      return ospMat;
    }
  }

  NodePtr GLTFData::createOSPTexture(const std::string &texParam,
                                     const tinygltf::Texture &tex,
                                     const int colorChannel,
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

    // DEBUG << pad("", '.', 6) << "texture." + texName << "\n";
    // DEBUG << pad("", '.', 9) << "image name: " << img.name << "\n";
    // DEBUG << pad("", '.', 9) << "image width: " << img.width << "\n";
    // DEBUG << pad("", '.', 9) << "image height: " << img.height << "\n";
    // DEBUG << pad("", '.', 9) << "image component: " << img.component << "\n";
    // DEBUG << pad("", '.', 9) << "image bits: " << img.bits << "\n";
    // if (img.uri.length() < 256)  // The uri can be a stream of data!
    //  DEBUG << pad("", '.', 9) << "image uri: " << img.uri << "\n";
    // DEBUG << pad("", '.', 9) << "image bufferView: " << img.bufferView <<
    // "\n"; DEBUG << pad("", '.', 9) << "image data: " << img.image.size() <<
    // "\n";

    // XXX Only handle pre-loaded images for now
    if (img.image.empty()) {
      ERROR << "!!! no texture data!  Need to load it !!!\n";
      return nullptr;
    }

    auto ospTexNode = createNode(texParam, "texture_2d");
    auto &ospTex    = *ospTexNode->nodeAs<Texture2D>();

    ospTex.size.x = img.width;
    ospTex.size.y = img.height;
    ospTex.components = img.component;
    ospTex.depth =
        img.bits == 8 ? 1 : img.bits == 16 ? 2 : img.bits == 32 ? 4 : 0;

    // XXX handle different number of components!!!!
    if (!ospTex.depth)
      ERROR << "Not handled texel depth: " << img.bits << std::endl;
    if (ospTex.components != 4)
      ERROR << "Not yet handled number of components: " << ospTex.components
            << std::endl;

    auto texFormat =
        osprayTextureFormat(ospTex.depth, ospTex.components, preferLinear);

    // If texture doesn't use all channels(4), setup a strided-data access
    if (colorChannel < 4) {
      // Only 1 channel, colorChannel(0-3) determines which component is used
      texFormat = osprayTextureFormat(ospTex.depth, 1, preferLinear);
      if (ospTex.depth == 4) {
        ospTex.createChildData("data",
            vec2ul(img.width, img.height), // numItems
            sizeof(vec4f) * vec2ul(1, img.width), // byteStride
            (float *)img.image.data() + colorChannel);
      } else if (ospTex.depth == 2) {
        ospTex.createChildData("data",
            vec2ul(img.width, img.height), // numItems
            sizeof(vec4us) * vec2ul(1, img.width), // byteStride
            (uint16_t *)img.image.data() + colorChannel);
      } else {
        ospTex.createChildData("data",
            vec2ul(img.width, img.height), // numItems
            sizeof(vec4uc) * vec2ul(1, img.width), // byteStride
            (uint8_t *)img.image.data() + colorChannel);
      }
    } else {
      // RGBA
      if (ospTex.depth == 4) {
        ospTex.createChildData("data",
            vec2ul(img.width, img.height), // numItems
            vec2ul(0, 0), // byteStride
            (vec4f *)img.image.data());
      } else if (ospTex.depth == 2) {
        ospTex.createChildData("data",
            vec2ul(img.width, img.height), // numItems
            vec2ul(0, 0), // byteStride
            (vec4us *)img.image.data());
      } else {
        ospTex.createChildData("data",
            vec2ul(img.width, img.height), // numItems
            vec2ul(0, 0), // byteStride
            (vec4uc *)img.image.data());
      }
    }

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
                               int colorChannel,
                               bool preferLinear)
  {
    // A disabled texture will have index = -1
    if (texIndex < 0)
      return;

    auto texParam = "map_" + texParamBase;
    auto ospTexNode = createOSPTexture(
        texParam, model.textures[texIndex], colorChannel, preferLinear);
    if (ospTexNode) {
      //auto &ospTex = *ospTexNode->nodeAs<Texture2D>();
      //DEBUG << pad("", '.', 3) << "        .setChild: " << texParam << "= "
      //     << ospTex.name() << "\n";
      // DEBUG << pad("", '.', 3) << "            "
      //     << "depth: " << ospTex.depth << " components: " << ospTex.components
      //     << " preferLinear: " << preferLinear << "\n";

      // Texture extensions
      const auto &exts = model.textures[texIndex].extensions;
      if (exts.find("KHR_texture_transform") != exts.end()) {
        // https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_texture_transform
        auto params = exts.find("KHR_texture_transform")->second;

        // offset: The offset of the UV coordinate origin as a factor of the
        // texture dimensions. default:[0.0, 0.0]
        vec2f offset = vec2f(0.f);
        if (params.Has("offset")) {
          std::vector<tinygltf::Value> ov =
              params.Get("offset").Get<tinygltf::Value::Array>();
          offset = vec2f(ov[0].Get<double>(), ov[1].Get<double>());
        }
        ospTexNode->createChild("translation", "vec2f") = offset;

        // rotation: Rotate the UVs by this many radians counter-clockwise
        // around the origin. This is equivalent to a similar rotation of the
        // image clockwise. default:0.0
        float rotation = 0.0f;
        if (params.Has("rotation")) {
          rotation = (float)params.Get("rotation").Get<double>();
        }
        ospMat->createChild("rotation", "float") = rotation;

        // scale: The scale factor applied to the components of the UV
        // coordinates. default:[1.0, 1.0]
        vec2f scale = vec2f(1.f);
        if (params.Has("scale")) {
          std::vector<tinygltf::Value> sv =
              params.Get("scale").Get<tinygltf::Value::Array>();
          offset = vec2f(sv[0].Get<double>(), sv[1].Get<double>());
        }
        ospTexNode->createChild("scale", "vec2f") = scale;

        // texCoord: Overrides the textureInfo texCoord value if supplied, and
        // if this extension is supported.
        // XXX not sure what, if anything, to do about this one.
      }

      ospMat->add(ospTexNode);
    }
  }

  // GLTFmporter definitions //////////////////////////////////////////////////

  void glTFImporter::importScene()
  {
    // Create a root Transform/Instance off the Importer, under which to build
    // the import hierarchy
    std::string baseName = fileName.name() + "_rootXfm";
    auto rootNode = createNode(baseName, "transform");

    GLTFData gltf(rootNode, fileName, materialRegistry, cameras, fb, shared_from_this(), ic);

    if (!gltf.parseAsset())
      return;

    gltf.createMaterials();
    gltf.createLights();

    if (importCameras)
      gltf.createCameras();

    gltf.createSkins();
    gltf.createGeometries(); // needs skins
    gltf.buildScene();
    gltf.finalizeSkins(); // needs nodes / buildScene
    if (animations)
      gltf.createAnimations(*animations);
    if (gltf.lights.size() != 0) {
      auto lightsMan = std::static_pointer_cast<sg::LightsManager>(lightsManager);
      lightsMan->addLights(gltf.lights);
    }

    // Finally, add node hierarchy to importer parent
    add(rootNode);

    INFO << "finished import!\n";
  }

  }  // namespace sg
} // namespace ospray
