// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Importer.h"
// tiny_gltf
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"
// rkcommon
#include "rkcommon/os/FileName.h"

#include "glTF/buffer_view.h"
#include "glTF/gltf_types.h"

#include "sg/Util.h"
#include "sg/scene/Transform.h"
#include "sg/scene/geometry/Geometry.h"
#include "sg/texture/Texture2D.h"
#include "sg/visitors/PrintNodes.h"
// Note: may want to disable warnings/errors from TinyGLTF
#define REPORT_TINYGLTF_WARNINGS

#define DEBUG if (verboseImport) std::cout << prefix << "(D): "
#define INFO if (verboseImport) std::cout << prefix << "(I): "
#define WARN std::cout << prefix << "(W): "
#define ERR std::cerr << prefix << "(E): "

namespace ospray {
namespace sg {

struct glTFImporter : public Importer
{
  glTFImporter() = default;
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
      std::shared_ptr<CameraMap> _cameras,
      sg::FrameBuffer *_fb,
      NodePtr _currentImporter,
      InstanceConfiguration _ic,
      bool verboseImport
      )
      : fileName(fileName),
        rootNode(rootNode),
        materialRegistry(_materialRegistry),
        cameras(_cameras),
        fb(_fb),
        currentImporter(_currentImporter),
        ic(_ic),
        verboseImport(verboseImport)
  {}

 public:
  const FileName &fileName;

  bool parseAsset();
  void createMaterials();
  void createSkins();
  void finalizeSkins();
  void createGeometries();
  void createCameraTemplates();
  void createLightTemplates();
  void buildScene();
  void loadNodeInfo(const int nid, NodePtr sgNode);
  // load animations AFTER loading scene nodes and their transforms
  void createAnimations(std::vector<Animation> &);
  void applySceneBackground(NodePtr bgXfm);
  std::vector<NodePtr> lights;
  std::vector<NodePtr> lightTemplates;
  std::string geomId{""};
  bool importCameras{false};
  void setScheduler(SchedulerPtr _scheduler) {
    scheduler = _scheduler;
  }
  void setVolumeParams(NodePtr _volumeParams) {
    volumeParams = _volumeParams;
  }

 private:
  bool verboseImport{false}; // Enable/disable import logging
  InstanceConfiguration ic;
  NodePtr currentImporter;
  NodePtr rootNode;
  sg::FrameBuffer *fb{nullptr};
  std::shared_ptr<CameraMap> cameras{nullptr};
  std::vector<NodePtr> cameraTemplates;
  std::vector<SkinPtr> skins;
  std::vector<std::vector<NodePtr>> ospMeshes;
  std::shared_ptr<sg::MaterialRegistry> materialRegistry;
  SchedulerPtr scheduler{nullptr};
  NodePtr volumeParams;
  std::vector<NodePtr> sceneNodes; // lookup table glTF:nodeID -> NodePtr

  tinygltf::Model model;

  std::vector<NodePtr> ospMaterials;

  size_t baseMaterialOffset = 0; // set in createMaterials()
  int numIntelLights{0};

  void loadKeyframeInput(int accessorID, std::vector<float> &kfInput);

  void loadKeyframeOutput(std::vector<vec3f> &ts,
      std::vector<affine3f> &kfOutput,
      std::string &propertyName);

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
      const tinygltf::ExtensionMap &extensions,
      bool preferLinear = true)
  {
    setOSPTexture(ospMaterial,
        texParam,
        texIndex,
        extensions,
        4, // default, texture uses all channels
        preferLinear);
  }
  void setOSPTexture(NodePtr ospMaterial,
      const std::string &texParam,
      int texIndex,
      const tinygltf::ExtensionMap &extensions,
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
    ERR << "gltf parsing errors(s)...\n" << err << std::endl;
  }
#endif

  if (!ret) {
    ERR << "FATAL error parsing gltf file,"
          << " no geometry added to the scene!" << std::endl;
    return false;
  }

  auto asset = model.asset;

  if (std::stof(asset.version) < 2.0) {
    ERR << "FATAL: incompatible glTF file, version " << asset.version
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
  INFO << "... " << model.cameras.size() << " cameraTemplates\n";
  INFO << "... " << model.scenes.size() << " scenes\n";
  INFO << "... " << model.lights.size() << " lights\n";

  // geomtericId/assetId
  if (asset.extensions.find("BIT_asset_info") != asset.extensions.end()) {
    auto BIT_assetInfo = asset.extensions.find("BIT_asset_info")->second;
    geomId = BIT_assetInfo.Get("id").Get<std::string>();
  }

  return ret;
}

void GLTFData::applySceneBackground(NodePtr bgXfm)
{
  auto background = model.extensions.find("BIT_scene_background")->second;
  auto &bgFileName = background.Get("background-uri").Get<std::string>();
  rkcommon::FileName bgTexture = fileName.path() + bgFileName;
  auto bgNode = createNode("background", "hdri");
  bgNode->child("filename") = bgTexture.str();

  if (background.Has("rotation")) {
    const auto &r = background.Get("rotation").Get<tinygltf::Value::Array>();
    auto &rot = bgXfm->nodeAs<sg::Transform>()->child("rotation");
    rot = normalize(quaternionf(r[3].Get<double>(),
        r[0].Get<double>(),
        r[1].Get<double>(),
        r[2].Get<double>()));
  }

  if (background.Has("visible"))
    bgNode->child("visible") = background.Get("visible").Get<bool>();

  if (background.Has("intensity"))
    bgNode->child("intensity") =
        (float)background.Get("intensity").Get<double>();

  lights.push_back(bgNode);
  bgXfm->add(bgNode);
}

void GLTFData::loadNodeInfo(const int nid, NodePtr sgNode)
{
  const tinygltf::Node &n = model.nodes[nid];

  // load referenced asset if reference-link found
  std::string refTitle{""};
  bool isVolume{false};
  if (n.extensions.find("BIT_reference_link") != n.extensions.end()) {
    auto refLink = n.extensions.find("BIT_reference_link")->second;
    refTitle = refLink.Get("title").Get<std::string>();
    isVolume = refLink.Get("type").Get<std::string>() == "volume";
  }

  auto node = n.extensions.find("BIT_node_info")->second;
  if (node.Has("id")) {
    auto &nodeId = node.Get("id").Get<std::string>();
    if (!isValidUUIDv4(nodeId))
      std::cerr << nodeId << " is not a valid version 4 UUID\n";
    sgNode->createChild("instanceId", "string", nodeId);
  }

  // nothing to import
  if (refTitle.empty())
    return;

  if (!isVolume)
    refTitle += ".gltf";

  std::string refLinkFullPath = fileName.path() + refTitle;
  rkcommon::FileName file(refLinkFullPath);
  INFO << "Importing: " << file << std::endl;

  auto importer =
      std::static_pointer_cast<sg::Importer>(sg::getImporter(sgNode, file));
  if (importer) {
    if (scheduler)
      importer->setScheduler(scheduler);

    if (volumeParams)
      importer->setVolumeParams(volumeParams);

    importer->setMaterialRegistry(materialRegistry);
    auto parentImporter = currentImporter->nodeAs<sg::Importer>();

    auto cameraList = parentImporter->getCameraList();
    if (cameraList)
      importer->setCameraList(cameraList);

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

void GLTFData::createLightTemplates()
{
  // INTEL_lights_sunsky extension
  if (model.extensions.find("INTEL_lights_sunsky") != model.extensions.end()) {
    auto lightsSunSky =
        model.extensions.find("INTEL_lights_sunsky")->second.Get("lights");
    int arrayLen = 0;
    if (lightsSunSky.IsArray())
      arrayLen = lightsSunSky.ArrayLen();

    for (int i = 0; i < arrayLen; i++) {
      auto lightName = "sunSkyLight_" + std::to_string(i);
      auto sunSky = lightsSunSky.Get(i);
      float intensity = sunSky.Get("intensity").Get<double>();
      float elevation = (float)sunSky.Get("elevation").Get<double>()
          * (180.f / (float)pi); // degrees
      float azimuth = (float)sunSky.Get("azimuth").Get<double>()
          * (180.f / (float)pi); // degrees
      float turbidity = sunSky.Get("turbidity").Get<double>();
      float albedo = sunSky.Get("albedo").Get<double>();
      float horizonExtension = sunSky.Get("horizonExtension").Get<double>();
      auto sunSkyLight = createNode(lightName, "sunSky");
      sunSkyLight->child("intensity") = intensity;
      sunSkyLight->child("elevation") = elevation;
      sunSkyLight->child("azimuth") = azimuth;
      sunSkyLight->child("turbidity") = turbidity;
      sunSkyLight->child("albedo") = albedo;
      sunSkyLight->child("horizonExtension") = horizonExtension;

      lightTemplates.push_back(sunSkyLight);
    }
    numIntelLights = lightTemplates.size();
  }
  // KHR_lights_punctual
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
      newLight->child("openingAngle") = outerConeAngle * (360.f / (float)pi);
      newLight->child("penumbraAngle") =
          (outerConeAngle - innerConeAngle) * (180.f / (float)pi);
    } else if (l.type == "hdri") {
      newLight = createNode(lightName, l.type);
      auto hdrFileName = l.extras.Get("map").Get<std::string>();
      newLight->child("filename") = fileName.path() + hdrFileName;
    } else
      newLight = createNode(lightName, l.type);

    if (newLight->hasChild("position"))
      newLight->child("position").setValue(vec3f(0));
    if (newLight->hasChild("direction"))
      newLight->child("direction").setValue(vec3f(0.f, 0.f, -1.f));

    // Color is optional, default:[1.0,1.0,1.0]
    auto lightColor = rgb(1.f);
    if (!l.color.empty())
      lightColor = rgb{(float)l.color[0], (float)l.color[1], (float)l.color[2]};
    newLight->child("color") = lightColor;

    if (l.intensity)
      newLight->child("intensity") = (float)l.intensity;
    if (l.range)
      WARN << "Range value for light is not supported yet" << std::endl;

    // TODO:: Address extras property on lights

    lightTemplates.push_back(newLight);
  }
}

void GLTFData::createMaterials()
{
  // DEBUG << "Create Materials\n";

  // Create materials and textures
  // (adding 1 for default material)
  ospMaterials.reserve(model.materials.size() + 1);

  // "default" material for glTF '-1' index (no material)
  static int defMatNum = 0;
  ospMaterials.emplace_back(createNode(
      fileName.name() + std::to_string(defMatNum++) + ":default", "obj"));

  // Create materials (also sets textures to material params)
  for (const auto &material : model.materials) {
    ospMaterials.push_back(createOSPMaterial(material));
  }

  baseMaterialOffset = materialRegistry->baseMaterialOffSet();
  for (auto m : ospMaterials)
    materialRegistry->add(m);
}

void GLTFData::createCameraTemplates()
{
  cameraTemplates.reserve(model.cameras.size());
  NodePtr sgCamera;

  for (auto &m : model.cameras) {
    static auto nCamera = 0;
    auto cameraName = m.name;
    if (cameraName == "")
      cameraName = "camera_" + std::to_string(nCamera);
    if (m.type == "perspective") {
      sgCamera = createNode("camera", "camera_perspective");
      // convert radians to degrees for vertical FOV
      float fovy = (float)m.perspective.yfov * (180.f / (float)pi);

      sgCamera->child("fovy") = fovy;
      sgCamera->child("nearClip") = (float)m.perspective.znear;
      sgCamera->child("aspect") = (float)m.perspective.aspectRatio;

    } else if (m.type == "panoramic") { // custom extension
      sgCamera = createNode("camera", "camera_panoramic");
    } else {
      sgCamera = createNode("camera", "camera_orthographic");
      sgCamera->child("height") = (float)m.orthographic.ymag;

      // calculate orthographic aspect with horizontal and vertical
      // maginifications
      float aspect = (float)m.orthographic.xmag / m.orthographic.ymag;
      sgCamera->child("aspect") = aspect;
      sgCamera->child("nearClip") = (float)m.orthographic.znear;
    }
    sgCamera->child("position") = vec3f(0.f);
    sgCamera->child("direction") = vec3f(0.0f, 0.0f, -1.f);
    sgCamera->child("up") = vec3f(0.0f, 1.0f, 0.f);
    sgCamera->child("uniqueCameraName") = cameraName;

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
        len_x = x * (float)pixelSize[0].Get<double>();
        len_y = y * (float)pixelSize[1].Get<double>();
        sgCamera->child("aspect").setValue(len_x / len_y);
      }

      if (ext.Has("lens")) {
        auto lens = ext.Get("lens");
        float focalLength = (float)lens.Get("focalLength").Get<double>();
        float fovy = 2 * atan(len_y / (2 * focalLength)) * (180.f / (float)pi);
        sgCamera->child("fovy").setValue(fovy);

        float apertureRadius = (float)lens.Get("apertureRadius").Get<double>();
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

      if (ext.Has("temporal")) {
        auto temp = ext.Get("temporal");
        auto copyFloatParam = [&](const char *name) -> float {
          float ret = 0.0f;
          if (temp.Has(name)) {
            ret = (float)temp.Get(name).Get<double>();
            sgCamera->createChild(name, "float", "", ret);
            sgCamera->child(name).setSGOnly();
          }
          return ret;
        };
        copyFloatParam("startTime");
        float measureTime = copyFloatParam("measureTime");
        if (measureTime > 0.0f) {
          sgCamera->child("motion blur").setValue(1.0f);
          uint8_t shutterType = 0; // default global
          if (temp.Get("type").Get<std::string>() == "rollingShutter") {
            shutterType = 1; // default rolling right
            if (temp.Has("rollingShutter")) {
              auto rol = temp.Get("rollingShutter");
              if (rol.Has("direction")) {
                auto str = rol.Get("direction").Get<std::string>();
                if (str == "left")
                  shutterType = 2;
                if (str == "down")
                  shutterType = 3;
                if (str == "up")
                  shutterType = 4;
              }
              float duration = 0.0f;
              if (rol.Has("measureTime"))
                duration = (float)rol.Get("measureTime").Get<double>();
              sgCamera->child("rollingShutterDuration") =
                  duration / measureTime;
            }
          }
          sgCamera->child("shutterType") = shutterType;
        }
      }
    }

    cameraTemplates.push_back(sgCamera);
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
          t->values.push_back(normalize(quaternionf(v.w, v.x, v.y, v.z)));
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

  // background with custom BIT_scene_background extension is applied on
  // scene-level
  if (model.extensions.find("BIT_scene_background") != model.extensions.end()) {
    auto bgXfm = createNode("bgXfm", "transform");
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
        else
          targetNode = rootNode;
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
  if (n.name != "")
    newXfm->setOrigName(n.name);
  if (ic == 1)
    newXfm->child("dynamicScene").setValue(true);
  else if (ic == 2)
    newXfm->child("compactMode").setValue(true);
  else if (ic == 3)
    newXfm->child("robustMode").setValue(true);

  sceneNodes[nid] = newXfm;
  sgNode->add(newXfm);
  applyNodeTransform(newXfm, n);

  if (level == 1 && !geomId.empty()) {
    if (!isValidUUIDv4(geomId))
      std::cerr << geomId << " is not a valid version 4 UUID\n";
    newXfm->createChild("geomId", "string", geomId);
    geomId = "";
  }

  // create child animate camera for all camera nodes added to scene hierarchy,
  // bool value is set during createAnimation when appropriate target xfm is
  // found
  if (n.camera != -1 && !importCameras && cameras) {
    // add camera from existing .sg cameras
    auto camera = cameras->at(n.name);
    newXfm->add(camera);
  } else if (n.camera != -1 && n.camera < cameraTemplates.size() && cameras) {
    auto cameraTemplate = cameraTemplates[n.camera];
    static auto nCamera = 0;
    auto camera = createNode("camera", cameraTemplate->subType());
    for (auto &c : cameraTemplate->children()) {
      if (camera->hasChild(c.first))
        camera->child(c.first) = c.second->value();
      else {
        camera->createChild(c.first, c.second->subType(), c.second->value());
        if (cameraTemplate->child(c.first).sgOnly())
          camera->child(c.first).setSGOnly();
      }
    }
    auto uniqueCamName =
        n.name != "" ? n.name : "camera_" + std::to_string(nCamera);
    camera->child("uniqueCameraName") = uniqueCamName;
    camera->child("cameraId").setValue(++nCamera);

    cameras->operator[](uniqueCamName) = camera;
    newXfm->add(camera);
  }

  sgNode = newXfm;

  if (n.extensions.find("BIT_node_info") != n.extensions.end()
      || n.extensions.find("BIT_reference_link") != n.extensions.end())
    loadNodeInfo(nid, sgNode);

  std::vector<int> lightIdxs;

  // instantiate lights for extensions: INTEL_lights_sunsky and
  // KHR_lights_punctual
  if (n.extensions.find("INTEL_lights_sunsky") != n.extensions.end()) {
    auto lightJson =
        n.extensions.find("INTEL_lights_sunsky")->second.Get("light");
    lightIdxs.push_back(lightJson.GetNumberAsInt());
  }

  if (n.extensions.find("KHR_lights_punctual") != n.extensions.end()) {
    auto lightJson =
        n.extensions.find("KHR_lights_punctual")->second.Get("light");
    lightIdxs.push_back(lightJson.GetNumberAsInt() + numIntelLights);
  }

  for (auto &lightIdx : lightIdxs) {
    auto lightTemplate = lightTemplates[lightIdx];
    static int lightCounter = 0;
    // instantiate SG light nodes
    auto uniqueLightName = n.name != ""
        ? n.name + std::to_string(lightCounter++)
        : lightTemplate->name() + std::to_string(lightCounter++);
    auto light = createNode(uniqueLightName, lightTemplate->subType());
    for (auto &c : lightTemplate->children()) {
      if (light->hasChild(c.first))
        light->child(c.first) = c.second->value();
      else {
        light->createChild(c.first, c.second->subType(), c.second->value());
        if (lightTemplate->child(c.first).sgOnly())
          light->child(c.first).setSGOnly();
      }
    }
    lights.push_back(light);
    sgNode->add(light);
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
    xfmNode->setValue(affine3f(vec3f(m[0 * 4 + 0], m[0 * 4 + 1], m[0 * 4 + 2]),
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
      xfmNode->child("rotation") =
          normalize(quaternionf(r[3], r[0], r[1], r[2]));
    }
    if (!n.translation.empty()) {
      const auto &t = n.translation;
      xfmNode->child("translation") = vec3f(t[0], t[1], t[2]);
    }
  }
}

NodePtr GLTFData::createOSPMesh(
    const std::string &primBaseName, tinygltf::Primitive &prim)
{
  static auto nPrim = 0;
  auto primName = primBaseName + "_" + pad(std::to_string(nPrim++));
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
    ospGeom = createNodeAs<Geometry>(primName + "_object", "geometry_spheres");
  } else {
    ERR << "Unsupported primitive mode! File must contain only "
             "triangles or points\n";
    throw std::runtime_error(
        "Unsupported primitive mode! Only triangles are supported");
  }

#if 1 // XXX: Generalize these with full component support!!!
      // In : 1,2,3,4 ubyte, ubyte(N), ushort, ushort(N), uint, float
      // Out:   2,3,4 int, float

  if (prim.indices > -1) {
    auto &vi = ospGeom->vi;
    // Indices: scalar ubyte/ushort/uint
    if (model.accessors[prim.indices].componentType
        == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
      Accessor<uint8_t> index_accessor(model.accessors[prim.indices], model);
      vi.reserve(index_accessor.size() / 3);
      for (size_t i = 0; i < index_accessor.size() / 3; ++i)
        vi.emplace_back(vec3ui(index_accessor[i * 3],
            index_accessor[i * 3 + 1],
            index_accessor[i * 3 + 2]));
    } else if (model.accessors[prim.indices].componentType
        == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
      Accessor<uint16_t> index_accessor(model.accessors[prim.indices], model);
      vi.reserve(index_accessor.size() / 3);
      for (size_t i = 0; i < index_accessor.size() / 3; ++i)
        vi.emplace_back(vec3ui(index_accessor[i * 3],
            index_accessor[i * 3 + 1],
            index_accessor[i * 3 + 2]));
    } else if (model.accessors[prim.indices].componentType
        == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
      Accessor<uint32_t> index_accessor(model.accessors[prim.indices], model);
      vi.reserve(index_accessor.size() / 3);
      for (size_t i = 0; i < index_accessor.size() / 3; ++i)
        vi.emplace_back(vec3ui(index_accessor[i * 3],
            index_accessor[i * 3 + 1],
            index_accessor[i * 3 + 2]));
    } else {
      ERR << "Unsupported index type: "
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
    if (model.accessors[col_attrib].componentType
        == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
      Accessor<uint16_t> col_accessor(model.accessors[col_attrib], model);
      vc.reserve(col_accessor.size() / 3);
      for (size_t i = 0; i < col_accessor.size() / 3; ++i)
        vc.emplace_back(vec4f(col_accessor[i * 3],
            col_accessor[i * 3 + 1],
            col_accessor[i * 3 + 2],
            1.f));
    } else if (model.accessors[col_attrib].componentType
        == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
      Accessor<uint8_t> col_accessor(model.accessors[col_attrib], model);
      vc.reserve(col_accessor.size() / 3);
      for (size_t i = 0; i < col_accessor.size() / 3; ++i)
        vc.emplace_back(vec4f(col_accessor[i * 3],
            col_accessor[i * 3 + 1],
            col_accessor[i * 3 + 2],
            1.f));
    } else if (model.accessors[col_attrib].componentType
        == TINYGLTF_COMPONENT_TYPE_FLOAT) {
      Accessor<vec4f> col_accessor(model.accessors[col_attrib], model);
      vc.reserve(col_accessor.size());
      for (size_t i = 0; i < col_accessor.size(); ++i)
        vc.emplace_back(col_accessor[i]);
    } else {
      ERR << "Unsupported color type: "
            << model.accessors[col_attrib].componentType << "\n";
      throw std::runtime_error("Unsupported color component type");
    }

    // If alphaMode is OPAQUE (or default material), leave colors unchanged,
    // but set all alpha to 1.f
    if (prim.material == -1
        || model.materials[prim.material].alphaMode == "OPAQUE") {
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
          && weights.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
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

    if (ospGeom->checkAndNormalizeWeights())
      WARN << "non-normalized weights\n";

  } else if (ospGeom->subType() == "geometry_spheres") {
    // Positions: vec3f
    Accessor<vec3f> pos_accessor(
        model.accessors[prim.attributes["POSITION"]], model);
    ospGeom->skinnedPositions.reserve(pos_accessor.size());
    for (size_t i = 0; i < pos_accessor.size(); ++i)
      ospGeom->skinnedPositions.emplace_back(pos_accessor[i]);
    ospGeom->createChildData(
        "sphere.position", ospGeom->skinnedPositions, true);

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
  auto matName = mat.name + "_" + pad(std::to_string(nMat++));
  // DEBUG << pad("", '.', 3) << "material." + matName << "\n";
  // DEBUG << pad("", '.', 3) << "        .alphaMode:" << mat.alphaMode
  //       << "\n";
  // DEBUG << pad("", '.', 3)
  //       << "        .alphaCutoff:" << (float)mat.alphaCutoff << "\n";

  auto &pbr = mat.pbrMetallicRoughness;

  // DEBUG << pad("", '.', 3)
  //     << "        .baseColorFactor.alpha:" << (float)pbr.baseColorFactor[4]
  //     << "\n";

  auto emissiveColor =
      rgb(mat.emissiveFactor[0], mat.emissiveFactor[1], mat.emissiveFactor[2]);

  // We can emulate a constant colored emissive texture
  // XXX this is a workaround
  auto constColor = true;
  if (emissiveColor != rgb(0.f) && mat.emissiveTexture.index != -1) {
    const auto &tex = model.textures[mat.emissiveTexture.index];
    const auto &img = model.images[tex.source];
    if (img.image.size() > 0) {
      const auto *data = img.image.data();

      const rgb color0 = rgb(data[0], data[1], data[2]);
      auto i = 1;
      WARN << "Material emissiveTexture #" << mat.emissiveTexture.index
           << std::endl;
      WARN << "   color0 : " << color0 << std::endl;
      while (constColor && (i < img.width * img.height)) {
        const rgb color =
            rgb(data[4 * i + 0], data[4 * i + 1], data[4 * i + 2]);
        if (color0 != color) {
          WARN << "   color @ " << i << " : " << color << std::endl;
          WARN << "   !!! non constant color, skipping emissive" << std::endl;
          constColor = false;
          break;
        }
        i++;
      }
      // Module the emissiveColor with the texture value.
      emissiveColor *= color0;
    }
  }

  if ((emissiveColor == rgb(0.f)) || (constColor == false)) {
    auto ospMat = createNode(matName, "principled");
    ospMat->createChild("baseColor", "rgb") = rgb(
        pbr.baseColorFactor[0], pbr.baseColorFactor[1], pbr.baseColorFactor[2]);
    auto alpha = (float)pbr.baseColorFactor[3];
    ospMat->createChild("metallic", "float") = (float)pbr.metallicFactor;
    ospMat->createChild("roughness", "float") = (float)pbr.roughnessFactor;

    // Unspec'd defaults
    ospMat->createChild("diffuse", "float") = 1.0f;
    ospMat->createChild("ior", "float") = 1.5f;

    // XXX will require texture tweaks to get closer to glTF spec, if needed
    // BLEND is always used, so OPAQUE can be achieved by setting all texture
    // texels alpha to 1.0.  OSPRay doesn't support alphaCutoff for MASK mode.
    // Masking turned off atm due to issues with unwanted opacity values,
    // specially for Valerie scenes(some large city buildings)
    if (mat.alphaMode == "OPAQUE")
      ospMat->createChild("opacity", "float") = 1.f;
    else if (mat.alphaMode == "BLEND")
      ospMat->createChild("opacity", "float") = alpha;
    // else if (mat.alphaMode == "MASK")
    //   ospMat->createChild("opacity", "float") = 1.f - (float)mat.alphaCutoff;

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
      setOSPTexture(ospMat,
          "baseColor",
          pbr.baseColorTexture.index,
          pbr.baseColorTexture.extensions,
          false);
    }

    if (pbr.metallicRoughnessTexture.index != -1
        && pbr.metallicRoughnessTexture.texCoord == 0) {
      // metallic in Blue(2) channel, roughness in Green(1)
      setOSPTexture(ospMat,
          "metallic",
          pbr.metallicRoughnessTexture.index,
          pbr.metallicRoughnessTexture.extensions,
          2);
      setOSPTexture(ospMat,
          "roughness",
          pbr.metallicRoughnessTexture.index,
          pbr.metallicRoughnessTexture.extensions,
          1);
    }

    if (mat.normalTexture.index != -1 && mat.normalTexture.texCoord == 0) {
      // NormalTextureInfo() : index(-1), texCoord(0), scale(1.0) {}
      setOSPTexture(ospMat,
          "normal",
          mat.normalTexture.index,
          mat.normalTexture.extensions);
      ospMat->createChild("normal", "float", (float)mat.normalTexture.scale);
    }

    // Material Extensions
    const auto &exts = mat.extensions;

    // KHR_materials_specular
    if (exts.find("KHR_materials_specular") != exts.end()) {
      // https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_specular
      auto params = exts.find("KHR_materials_specular")->second;

      // specularFactor: The strength of the specular reflection.
      // default: 1.0
      float specular = 1.f;
      if (params.Has("specularFactor"))
        specular = (float)params.Get("specularFactor").Get<double>();
      ospMat->createChild("specular", "float") = specular;

      // specularTexture: A texture that defines the strength of the specular
      // reflection, stored in the alpha (A) channel. This will be multiplied
      // by specularFactor.
      if (params.Has("specularTexture")) {
        setOSPTexture(ospMat,
            "specular",
            params.Get("specularTexture").Get("index").Get<int>(),
            params.Get("specularTexture")
                .Get("extensions")
                .Get<tinygltf::ExtensionMap>(),
            3);
      }

#if 0 // XXX OSPRay is missing the F0 color, always assumes white?
      // specularColorFactor The F0 color of the specular reflection
      // (linear RGB).
      // default: [1.0, 1.0, 1.0]
        rgb specularColorFactor  = rgb(1.f);
        if (params.Has("specularColorFactor")) {
          std::vector<tinygltf::Value> sv =
              params.Get("specularColorFactor").Get<tinygltf::Value::Array>();
          specularColorFactor  = rgb(
              sv[0].Get<double>(), sv[1].Get<double>(), sv[2].Get<double>());
        }
        ospMat->createChild("specularColor", "rgb") = specularColorFactor;

        // specularColorTexture: A texture that defines the F0 color of the
        // specular reflection, stored in the RGB channels and encoded in sRGB.
        // This texture will be multiplied by specularColorFactor.
        if (params.Has("specularColorTexture")) {
          setOSPTexture(ospMat,
              "specularColor",
              params.Get("specularColorTexture").Get("index").Get<int>(),
              false);
        }
#endif
    }

    // KHR_materials_pbrSpecularGlossiness
    if (exts.find("KHR_materials_pbrSpecularGlossiness") != exts.end()) {
      // https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_pbrSpecularGlossiness
      auto params = exts.find("KHR_materials_pbrSpecularGlossiness")->second;

      // diffuseFactor: The reflected diffuse factor of the material.
      // default:[1.0,1.0,1.0,1.0]
      rgb diffuse = rgb(1.f);
      float opacity = 1.f;
      if (params.Has("diffuseFactor")) {
        std::vector<tinygltf::Value> dv =
            params.Get("diffuseFactor").Get<tinygltf::Value::Array>();
        diffuse =
            rgb(dv[0].Get<double>(), dv[1].Get<double>(), dv[2].Get<double>());
        opacity = (float)dv[3].Get<double>();
      }
      ospMat->createChild("baseColor", "rgb") = diffuse;
      ospMat->createChild("opacity", "float") = opacity;

      // diffuseTexture: The diffuse texture.
      if (params.Has("diffuseTexture")) {
        // RGB or RGBA
        setOSPTexture(ospMat,
            "baseColor",
            params.Get("diffuseTexture").Get("index").Get<int>(),
            params.Get("diffuseTexture")
                .Get("extensions")
                .Get<tinygltf::ExtensionMap>(),
            false);
      }

      // specularFactor: The specular RGB color of the material.
      // default:[1.0,1.0,1.0]
      rgb specular = rgb(1.f);
      if (params.Has("specularFactor")) {
        std::vector<tinygltf::Value> sv =
            params.Get("specularFactor").Get<tinygltf::Value::Array>();
        specular =
            rgb(sv[0].Get<double>(), sv[1].Get<double>(), sv[2].Get<double>());
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
              params.Get("specularGlossinessTexture")
                  .Get("extensions")
                  .Get<tinygltf::ExtensionMap>(),
              false);
        }
#endif
    }

    // KHR_materials_clearcoat
    if (exts.find("KHR_materials_clearcoat") != exts.end()) {
      // https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_clearcoat
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
            params.Get("clearcoatTexture").Get("index").Get<int>(),
            params.Get("clearcoatTexture")
                .Get("extensions")
                .Get<tinygltf::ExtensionMap>(),
            0);
      }
      if (params.Has("clearcoatRoughnessTexture")) {
        // Green channel
        setOSPTexture(ospMat,
            "coatRoughness",
            params.Get("clearcoatRoughnessTexture").Get("index").Get<int>(),
            params.Get("clearcoatRoughnessTexture")
                .Get("extensions")
                .Get<tinygltf::ExtensionMap>(),
            1);
      }
#endif

      // clearcoatNormalTexture: The clearcoat normal map texture.
      if (params.Has("clearcoatNormalTexture")) {
        setOSPTexture(ospMat,
            "coatNormal",
            params.Get("clearcoatNormalTexture").Get("index").Get<int>(),
            params.Get("clearcoatNormalTexture")
                .Get("extensions")
                .Get<tinygltf::ExtensionMap>());
      }
    }

    // KHR_materials_transmission
    if (exts.find("KHR_materials_transmission") != exts.end()) {
      // https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_transmission
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

      rgb tinting = rgb(pbr.baseColorFactor[0],
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
            "transmissionColor",
            pbr.baseColorTexture.index,
            pbr.baseColorTexture.extensions,
            false);
      }

      // transmissionTexture: A texture that defines the transmission
      // percentage of the surface, stored in the R channel. This will be
      // multiplied by transmissionFactor.
      if (params.Has("transmissionTexture")) {
        setOSPTexture(ospMat,
            "transmission",
            params.Get("transmissionTexture").Get("index").Get<int>(),
            params.Get("transmissionTexture")
                .Get("extensions")
                .Get<tinygltf::ExtensionMap>(),
            0);
      }
    }

    // KHR_materials_sheen
    if (exts.find("KHR_materials_sheen") != exts.end()) {
      // https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_sheen/
      auto params = exts.find("KHR_materials_sheen")->second;

      // sheen weight (not in spec)
      ospMat->createChild("sheen", "float") = 1.f;

      // sheenColorFactor: The sheen color in linear space
      // default:[0.0, 0.0, 0.0]
      rgb sheen = rgb(0.f);
      if (params.Has("sheenColorFactor")) {
        std::vector<tinygltf::Value> sv =
            params.Get("sheenColorFactor").Get<tinygltf::Value::Array>();
        sheen =
            rgb(sv[0].Get<double>(), sv[1].Get<double>(), sv[2].Get<double>());
      }
      ospMat->createChild("sheenColor", "rgb") = sheen;

      // sheenColorTexture: The sheen color (sRGB).
      if (params.Has("sheenColorTexture")) {
        setOSPTexture(ospMat,
            "sheen",
            params.Get("sheenColorTexture").Get("index").Get<int>(),
            params.Get("sheenColorTexture")
                .Get("extensions")
                .Get<tinygltf::ExtensionMap>());
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
            params.Get("sheenRoughnessTexture").Get("index").Get<int>(),
            params.Get("sheenRoughnessTexture")
                .Get("extensions")
                .Get<tinygltf::ExtensionMap>(),
            3);
      }
    }

    // KHR_materials_ior
    if (exts.find("KHR_materials_ior") != exts.end()) {
      // https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_ior
      auto params = exts.find("KHR_materials_ior")->second;

      // ior: The index of refraction. default:1.5
      float ior = 1.5f;
      if (params.Has("ior")) {
        ior = (float)params.Get("ior").Get<double>();
      }
      ospMat->createChild("ior", "float") = ior;
    }

    // KHR_materials_volume
    if (exts.find("KHR_materials_volume") != exts.end()) {
      // https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_volume
      auto params = exts.find("KHR_materials_volume")->second;

      // thicknessFactor: The thickness of the volume beneath the surface.
      // default: 0.
      float thickness = 0.f;
      if (params.Has("thicknessFactor")) {
        thickness = (float)params.Get("thicknessFactor").Get<double>();
      }
      ospMat->createChild("thin", "bool") = thickness > 0 ? false : true;
      ospMat->createChild("thickness", "float") = thickness;

      // thicknessTexture <textureInfo> A texture that defines the thickness,
      // stored in the G channel. This will be multiplied by thicknessFactor.
      // Default: No
      if (params.Has("thicknessTexture")) {
        setOSPTexture(ospMat,
            "thickness",
            params.Get("thicknessTexture").Get("index").Get<int>(),
            params.Get("thicknessTexture")
                .Get("extensions")
                .Get<tinygltf::ExtensionMap>(),
            1);
      }

      // attenuationDistance <float> Density of the medium given as the
      // average distance that light travels in the medium before interacting
      // with a particle. The value is given in world space.
      // No, default: +Infinity (inf doesn't behave well with UI sliders)
      float attenuationDistance = std::numeric_limits<float>::max();
      if (params.Has("attenuationDistance")) {
        attenuationDistance =
            (float)params.Get("attenuationDistance").Get<double>();
      }
      ospMat->createChild("transmissionDepth", "float") = attenuationDistance;

      // attenuationColor <vec3f> The color that white light turns into due
      // to absorption when reaching the attenuation distance.
      // No, default: [1, 1, 1]
      rgb attenuationColor = rgb(1.f);
      if (params.Has("attenuationColor")) {
        std::vector<tinygltf::Value> ac =
            params.Get("attenuationColor").Get<tinygltf::Value::Array>();
        attenuationColor =
            rgb(ac[0].Get<double>(), ac[1].Get<double>(), ac[2].Get<double>());
        // XXX Setting transmissionColor to default attenuationColor would
        // result in overwriting the tinting specified by
        // KHR_materials_transmission. Only set transmissionColor if
        // attenuationColor is present.
        ospMat->createChild("transmissionColor", "rgb") = attenuationColor;
      }
    }

    return ospMat;

  } else {
    // XXX TODO Principled material doesn't have emissive params yet
    // So, use a luminous instead
    auto ospMat = createNode(matName, "luminous");

    if (emissiveColor != rgb(0.f)) {
      // Material Extensions
      const auto &exts = mat.extensions;

      float intensity = 1.f;

      // KHR_materials_emissive_strength
      if (exts.find("KHR_materials_emissive_strength") != exts.end()) {
        // (Not yet ratified)
        // https://github.com/KhronosGroup/glTF/tree/KHR_materials_emissive_strength/extensions/2.0/Khronos/KHR_materials_emissive_strength
        auto params = exts.find("KHR_materials_emissive_strength")->second;

        // default: 1.0
        intensity = 1.f;
        if (params.Has("emissiveStrength")) {
          intensity = (float)params.Get("emissiveStrength").Get<double>();
        }
      }

      ospMat->createChild("color", "rgb") = emissiveColor;
      ospMat->createChild("intensity", "float") = intensity;

      // Already checked for constant color above.
      if (mat.emissiveTexture.index != -1) {
        const auto &tex = model.textures[mat.emissiveTexture.index];
        const auto &img = model.images[tex.source];
        if (img.image.size() > 0) {
          const auto *data = img.image.data();
          const rgb color0 = rgb(data[0], data[1], data[2]);
          WARN << "   name: " << img.name << std::endl;
          WARN << "   img: (" << img.width << ", " << img.height << ")";
          WARN << std::endl;
          WARN << "   emulating with solid color : " << color0 << std::endl;
          ospMat->child("color") = emissiveColor * (color0 / 255.f);
        }
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
        setOSPTexture(ospMat, "color", mat.emissiveTexture, false);
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
  if (tex.source == -1)
    return nullptr;

  tinygltf::Image &img = model.images[tex.source];

  // The uri can be a stream of base64-encoded data!  If it is not, it
  // contains the base filename, whereas sometimes the img.name is less
  // descriptive.
  if (img.uri.length() > 0 && img.uri.length() < 256) {
    // XXX should decode the uri before using it as a filename.  Otherwise
    // FileName::canonical() returns "" for names containing '%20', etc.
    std::string fileName = FileName(img.uri).canonical();
    img.name = fileName != "" ? fileName : img.uri;
  }

  // Last try, if the texture comes from a bufferView and we have no other name
  // give it the <filename>_texture_<bufferview#>
  if (img.name.empty() && img.bufferView >= 0)
    img.name =
        fileName.name() + "_texture_" + pad(std::to_string(img.bufferView));

#if 0
    DEBUG << pad("", '.', 9) << "image name: |" << img.name << "|\n";
    DEBUG << pad("", '.', 9) << "image width: " << img.width << "\n";
    DEBUG << pad("", '.', 9) << "image height: " << img.height << "\n";
    DEBUG << pad("", '.', 9) << "image component: " << img.component << "\n";
    DEBUG << pad("", '.', 9) << "image bits: " << img.bits << "\n";
    if (img.uri.length() < 256)  // The uri can be a stream of data!
     DEBUG << pad("", '.', 9) << "image uri: " << img.uri << "\n";
    DEBUG << pad("", '.', 9) << "image bufferView: " << img.bufferView << "\n";
    DEBUG << pad("", '.', 9) << "image data: " << img.image.size() << "\n";
    DEBUG << pad("", '.', 9) << "texParam: " << texParam << "\n";
    DEBUG << pad("", '.', 9) << "colorChannel: " << colorChannel << "\n";
    DEBUG << pad("", '.', 9) << "\n";
#endif

  NodePtr ospTexNode = createNode(texParam, "texture_2d");
  auto &ospTex = *ospTexNode->nodeAs<Texture2D>();

  // XXX Check sampler wrap/clamp modes!!!
  bool nearestFilter = (tex.sampler != -1
      && model.samplers[tex.sampler].magFilter
          == TINYGLTF_TEXTURE_FILTER_NEAREST);

  // If texture name (uri) is a UDIM set, ignore tiny_gltf loaded image and
  // reload from file as udim tiles
  if (ospTex.checkForUDIM(img.name))
    img.image.clear();

  // Pre-loaded texture image (loaded by tinygltf)
  if (!img.image.empty()) {
    ospTex.params.size = vec2ul(img.width, img.height);
    ospTex.params.components = img.component;
    ospTex.params.depth =
        img.bits == 8 ? 1 : img.bits == 16 ? 2 : img.bits == 32 ? 4 : 0;
    if (!ospTex.load(img.name,
            preferLinear,
            nearestFilter,
            colorChannel,
            img.image.data()))
      ospTexNode = nullptr;

  } else {
    // Load texture from file (external uri or udim tiles)
    ospTex.params.flip = false; // glTF textures are not vertically flipped

    // use same path as gltf scene file
    // If load fails, remove the texture node
    if (!ospTex.load(img.name,
            preferLinear,
            nearestFilter,
            colorChannel))
      ospTexNode = nullptr;
  }

  return ospTexNode;
}

void GLTFData::setOSPTexture(NodePtr ospMat,
    const std::string &texParamBase,
    int texIndex,
    const tinygltf::ExtensionMap &exts,
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
    auto &ospTex = *ospTexNode->nodeAs<Texture2D>();
    // DEBUG << pad("", '.', 3) << "        .setChild: " << texParam << "= "
    //     << ospTex.name() << "\n";
    // DEBUG << pad("", '.', 3) << "            "
    //     << "depth: " << ospTex.depth << " components: " << ospTex.components
    //     << " preferLinear: " << preferLinear << "\n";

    // Texture extensions
    if (exts.find("KHR_texture_transform") != exts.end()) {
      // https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_texture_transform
      auto params = exts.find("KHR_texture_transform")->second;

      // offset: The offset of the UV coordinate origin as a factor of the
      // texture dimensions. default:[0.0, 0.0]
      if (params.Has("offset")) {
        std::vector<tinygltf::Value> ov =
            params.Get("offset").Get<tinygltf::Value::Array>();
        vec2f offset = vec2f(ov[0].Get<double>(), ov[1].Get<double>());
        ospMat->createChild(texParam + ".translation", "vec2f") = -offset;
      }

      // Create linear transformation for rotation/scale rather than using
      // OSPRay rotation/scale parameters. Those work around the texture center
      linear2f xfm(one);

      // rotation: Rotate the UVs by this many radians counter-clockwise
      // around the origin. This is equivalent to a similar rotation of the
      // image clockwise. default:0.0
      if (params.Has("rotation")) {
        float rotation = (float)params.Get("rotation").Get<double>();
        xfm *= linear2f::rotate(-rotation);
      }

      // scale: The scale factor applied to the components of the UV
      // coordinates. default:[1.0, 1.0]
      if (params.Has("scale")) {
        std::vector<tinygltf::Value> sv =
            params.Get("scale").Get<tinygltf::Value::Array>();
        vec2f scale = vec2f(sv[0].Get<double>(), sv[1].Get<double>());
        xfm *= linear2f::scale(scale);
      }

      if (xfm != linear2f(one))
        ospMat->createChild(texParam + ".transform", "linear2f") = xfm;

      // texCoord: Overrides the textureInfo texCoord value if supplied, and
      // if this extension is supported.
      // XXX not sure what, if anything, to do about this one.
    }

    // If texture is UDIM, set appropriate scale/translation
    if (ospTex.hasUDIM()) {
      auto scale = ospTex.getUDIM_scale();
      auto translation = ospTex.getUDIM_translation();
      auto paramName = texParam + ".scale";
      if (!ospMat->hasChild(paramName))
        ospMat->createChild(paramName, "vec2f", scale);
      else
        ospMat->child(paramName) =
            ospMat->child(paramName).valueAs<vec2f>() * scale;

      paramName = texParam + ".translation";
      if (!ospMat->hasChild(paramName))
        ospMat->createChild(paramName, "vec2f", translation);
      else
        ospMat->child(paramName) =
            ospMat->child(paramName).valueAs<vec2f>() + translation;
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

  GLTFData gltf(rootNode,
      fileName,
      materialRegistry,
      cameras,
      fb,
      shared_from_this(),
      ic,
      verboseImport);

  if (!gltf.parseAsset())
    return;
  gltf.setScheduler(scheduler);
  gltf.setVolumeParams(volumeParams);
  gltf.createMaterials();
  gltf.createLightTemplates();

  if (importCameras) {
    gltf.importCameras = true;
    gltf.createCameraTemplates();
  }

  gltf.createSkins();
  gltf.createGeometries(); // needs skins
  gltf.buildScene();
  gltf.finalizeSkins(); // needs nodes / buildScene
  if (animations)
    gltf.createAnimations(*animations);
  if (gltf.lights.size() != 0) {
    auto lightsMan = std::static_pointer_cast<sg::LightsManager>(lightsManager);
    // Add lights as "Group" lights, they are part of the scene hierarchy and
    // will respond to scene transforms.
    lightsMan->addGroupLights(gltf.lights);
  }

  // Finally, add node hierarchy to importer parent
  add(rootNode);

  INFO << "finished import!\n";
}

} // namespace sg
} // namespace ospray
