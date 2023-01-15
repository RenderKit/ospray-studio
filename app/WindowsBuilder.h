// Copyright 2018 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

/* ----------------------------------------------------------
** WindowsBuilder class holds all functions and member 
** varibles for creating imgui windows and editors on top of 
** the GLFW window
** ---------------------------------------------------------*/

#include "GUIContext.h"

// widgets
#include "widgets/AdvancedMaterialEditor.h"
#include "widgets/FileBrowserWidget.h"
#include "widgets/Guizmo.h"
#include "widgets/ListBoxWidget.h"
#include "widgets/PieMenu.h"
#include "widgets/SearchWidget.h"
#include "widgets/TransferFunctionWidget.h"

// imgui
#include "imgui.h"

#include "sg/visitors/CollectTransferFunctions.h"
#include "sg/visitors/SetParamByNode.h"

using namespace ospray_studio;

// Option windows /////////////////////////////////////////////////////////////
struct WindowsBuilder
{
 public:
  WindowsBuilder(std::shared_ptr<GUIContext> _ctx, std::shared_ptr<CameraStack<CameraState>> cameraStack);
  ~WindowsBuilder() = default;

  void start();
  void buildWindows();
  void buildWindowRendererEditor();
  void buildWindowFrameBufferEditor();
  void buildWindowKeyframes();
  void buildWindowSnapshots();
  void buildWindowLightEditor();
  void buildWindowCameraEditor();
  void buildWindowMaterialEditor();
  void buildWindowTransferFunctionEditor();
  void buildWindowIsosurfaceEditor();
  void buildWindowTransformEditor();
  void buildWindowRenderingStats();

  // imgui window visibility toggles
  bool showRendererEditor{false};
  bool showFrameBufferEditor{false};
  bool showKeyframes{false};
  bool showSnapshots{false};
  bool showLightEditor{false};
  bool showCameraEditor{false};
  bool showMaterialEditor{false};
  bool showTransferFunctionEditor{false};
  bool showIsosurfaceEditor{false};
  bool showTransformEditor{false};
  bool showRenderingStats{false};

  // static member variables
  static ImGuiWindowFlags g_imguiWindowFlags;
  static std::vector<std::string> g_lightTypes;
  static std::vector<std::string> g_debugRendererTypes;
  static std::vector<std::string> g_renderers;
  std::string rendererTypeStr;
  OSPRayRendererType rendererType{OSPRayRendererType::SCIVIS};

  // static callback functions
  bool static rendererUI_callback(void *, int index, const char **out_text);
  bool static debugTypeUI_callback(void *, int index, const char **out_text);
  bool static lightTypeUI_callback(void *, int index, const char **out_text);

  // utility functions
  void viewCameraPath(bool showCameraPath);

  // main Util instance
  std::shared_ptr<GUIContext> ctx = nullptr;

  int whichLightType{-1};

  std::string lightTypeStr{"ambient"};

  std::shared_ptr<sg::LightsManager> lightsManager;
  std::shared_ptr<sg::MaterialRegistry> baseMaterialRegistry;
  std::shared_ptr<CameraStack<CameraState>> cameraStack = nullptr;
};

  // initialize all static member variables
ImGuiWindowFlags WindowsBuilder::g_imguiWindowFlags =
    ImGuiWindowFlags_AlwaysAutoResize;
std::vector<std::string> WindowsBuilder::g_lightTypes = {"ambient",
    "cylinder",
    "distant",
    "hdri",
    "sphere",
    "spot",
    "sunSky",
    "quad"};
std::vector<std::string> WindowsBuilder::g_debugRendererTypes = {"eyeLight",
    "primID",
    "geomID",
    "instID",
    "Ng",
    "Ns",
    "backfacing_Ng",
    "backfacing_Ns",
    "dPds",
    "dPdt",
    "volume"};

#ifdef USE_MPI
std::vector<std::string> WindowsBuilder::g_renderers = {
    "scivis", "pathtracer", "ao", "debug", "mpiRaycast"};
#else
std::vector<std::string> WindowsBuilder::g_renderers = {
    "scivis", "pathtracer", "ao", "debug"};
#endif

WindowsBuilder::WindowsBuilder(
    std::shared_ptr<GUIContext> _ctx, std::shared_ptr<CameraStack<CameraState>> _cameraStack)
    : ctx(_ctx), cameraStack(_cameraStack)
{
  lightsManager = ctx->lightsManager;
  baseMaterialRegistry = ctx->baseMaterialRegistry;
  rendererTypeStr = ctx->optRendererTypeStr;
}

// WindowsBuilder static member function definitions ////////////////
bool WindowsBuilder::rendererUI_callback(
    void *, int index, const char **out_text)
{
  *out_text = g_renderers[index].c_str();
  return true;
};

bool WindowsBuilder::debugTypeUI_callback(
    void *, int index, const char **out_text)
{
  *out_text = g_debugRendererTypes[index].c_str();
  return true;
};

bool WindowsBuilder::lightTypeUI_callback(
    void *, int index, const char **out_text)
{
  *out_text = g_lightTypes[index].c_str();
  return true;
};

void WindowsBuilder::start()
{
  if (showRendererEditor)
    buildWindowRendererEditor();
  if (showFrameBufferEditor)
    buildWindowFrameBufferEditor();
  if (showKeyframes)
    buildWindowKeyframes();
  if (showSnapshots)
    buildWindowSnapshots();
  if (showCameraEditor)
    buildWindowCameraEditor();
  if (showLightEditor)
    buildWindowLightEditor();
  if (showMaterialEditor)
    buildWindowMaterialEditor();
  if (showTransferFunctionEditor)
    buildWindowTransferFunctionEditor();
  if (showIsosurfaceEditor)
    buildWindowIsosurfaceEditor();
  if (showTransformEditor)
    buildWindowTransformEditor();
  if (showRenderingStats)
    buildWindowRenderingStats();
}

void WindowsBuilder::buildWindowRendererEditor()
{
  if (!ImGui::Begin(
          "Renderer editor", &showRendererEditor, g_imguiWindowFlags)) {
    ImGui::End();
    return;
  }

  int whichRenderer =
      find(g_renderers.begin(), g_renderers.end(), rendererTypeStr)
      - g_renderers.begin();

  static int whichDebuggerType = 0;
  ImGui::PushItemWidth(10.f * ImGui::GetFontSize());
  if (ImGui::Combo("renderer##whichRenderer",
          &whichRenderer,
          rendererUI_callback,
          nullptr,
          g_renderers.size())) {
    rendererTypeStr = g_renderers[whichRenderer];

    if (rendererType == OSPRayRendererType::DEBUGGER)
      whichDebuggerType = 0; // reset UI if switching away
                             // from debug renderer

    if (rendererTypeStr == "scivis")
      rendererType = OSPRayRendererType::SCIVIS;
    else if (rendererTypeStr == "pathtracer")
      rendererType = OSPRayRendererType::PATHTRACER;
    else if (rendererTypeStr == "ao")
      rendererType = OSPRayRendererType::AO;
    else if (rendererTypeStr == "debug")
      rendererType = OSPRayRendererType::DEBUGGER;
#ifdef USE_MPI
    if (rendererTypeStr == "mpiRaycast")
      rendererType = OSPRayRendererType::MPIRAYCAST;
#endif
    else
      rendererType = OSPRayRendererType::OTHER;

    // Change the renderer type, if the new renderer is different.
    auto &renderer = ctx->frame->childAs<sg::Renderer>("renderer");
    auto newType = "renderer_" + rendererTypeStr;

    if (renderer["type"].valueAs<std::string>() != newType) {
      // Save properties of current renderer then create new renderer
      ctx->saveRendererParams();
      ctx->optRendererTypeStr = rendererTypeStr;
      ctx->refreshRenderer();
    }
  }

  auto &renderer = ctx->frame->childAs<sg::Renderer>("renderer");

  if (rendererType == OSPRayRendererType::DEBUGGER) {
    if (ImGui::Combo("debug type##whichDebugType",
            &whichDebuggerType,
            debugTypeUI_callback,
            nullptr,
            g_debugRendererTypes.size())) {
      renderer["method"] = g_debugRendererTypes[whichDebuggerType];
    }
  }

  if (GenerateWidget(renderer))
    ctx->saveRendererParams();

  ImGui::End();
}

void WindowsBuilder::buildWindowFrameBufferEditor()
{
  if (!ImGui::Begin(
          "Framebuffer editor", &showFrameBufferEditor, g_imguiWindowFlags)) {
    ImGui::End();
    return;
  }

  auto &fb = ctx->frame->childAs<sg::FrameBuffer>("framebuffer");
  GenerateWidget(fb, sg::TreeState::ALLOPEN);

  ImGui::Separator();

  static int whichBuffer = 0;
  ImGui::Text("Display Buffer");
  ImGui::RadioButton("color##displayColor", &whichBuffer, 0);

  if (!fb.hasAlbedoChannel() && !fb.hasDepthChannel()) {
    ImGui::TextColored(
        ImVec4(.5f, .5f, .5f, 1.f), "Enable float format for more buffers");
  }

  if (fb.hasAlbedoChannel()) {
    ImGui::SameLine();
    ImGui::RadioButton("albedo##displayAlbedo", &whichBuffer, 1);
  }
  if (fb.hasDepthChannel()) {
    ImGui::SameLine();
    ImGui::RadioButton("depth##displayDepth", &whichBuffer, 2);
    ImGui::SameLine();
    ImGui::RadioButton("invert depth##displayDepthInv", &whichBuffer, 3);
  }

  ctx->selectBuffer(whichBuffer);

  ImGui::Separator();

  ImGui::Text("Post-processing");
  if (fb.isFloatFormat()) {
    ImGui::Checkbox("Tonemap", &ctx->frame->toneMapFB);
    ImGui::SameLine();
    ImGui::Checkbox("Tonemap nav", &ctx->frame->toneMapNavFB);

    if (ctx->studioCommon.denoiserAvailable) {
      ImGui::Checkbox("Denoise", &ctx->frame->denoiseFB);
      ImGui::SameLine();
      ImGui::Checkbox("Denoise nav", &ctx->frame->denoiseNavFB);
    }
    if (ctx->frame->denoiseFB || ctx->frame->denoiseNavFB) {
      ImGui::Checkbox("Denoise only PathTracer", &ctx->frame->denoiseOnlyPathTracer);
      ImGui::Checkbox("Denoise on final frame", &ctx->frame->denoiseFBFinalFrame);
      ImGui::SameLine();
      // Add accum here for convenience with final-frame denoising
      ImGui::SetNextItemWidth(5 * ImGui::GetFontSize());
      ImGui::DragInt(
          "Limit accumulation", &ctx->frame->accumLimit, 1, 0, INT_MAX, "%d frames");
    }
  } else {
    ImGui::TextColored(
        ImVec4(.5f, .5f, .5f, 1.f), "Enable float format for post-processing");
  }

  ImGui::Separator();

  ImGui::Text("Scaling");
  {
    static const float scaleValues[9] = {
        0.25f, 0.5f, 0.75f, 1.f, 1.25f, 1.5f, 2.f, 4.f, 8.f};

    auto size = ctx->frame->child("windowSize").valueAs<vec2i>();
    char _label[56];
    auto createLabel = [&_label, size](std::string uniqueId, float v) {
      const vec2i _sz = v * size;
      snprintf(_label,
          sizeof(_label),
          "%1.2fx (%d,%d)##%s",
          v,
          _sz.x,
          _sz.y,
          uniqueId.c_str());
      return _label;
    };

    auto selectNewScale = [&](std::string id, const float _scale) {
      auto scale = _scale;
      auto custom = true;
      for (auto v : scaleValues) {
        if (ImGui::Selectable(createLabel(id, v), v == scale))
          scale = v;
        custom &= (v != scale);
      }

      ImGui::Separator();
      char cLabel[64];
      snprintf(cLabel, sizeof(cLabel), "custom %s", createLabel(id, scale));
      if (ImGui::BeginMenu(cLabel)) {
        ImGui::SetNextItemWidth(5 * ImGui::GetFontSize());
        ImGui::InputFloat("x##fb_scaling", &scale);
        ImGui::EndMenu();
      }

      return scale;
    };

    auto scale = ctx->frame->child("scale").valueAs<float>();
    ImGui::SetNextItemWidth(12 * ImGui::GetFontSize());
    if (ImGui::BeginCombo("Scale resolution", createLabel("still", scale))) {
      auto newScale = selectNewScale("still", scale);
      if (scale != newScale)
        ctx->frame->child("scale") = newScale;
      ImGui::EndCombo();
    }

    scale = ctx->frame->child("scaleNav").valueAs<float>();
    ImGui::SetNextItemWidth(12 * ImGui::GetFontSize());
    if (ImGui::BeginCombo("Scale Nav resolution", createLabel("nav", scale))) {
      auto newScale = selectNewScale("nav", scale);
      if (scale != newScale)
        ctx->frame->child("scaleNav") = newScale;
      ImGui::EndCombo();
    }
  }

  ImGui::Separator();

  ImGui::Text("Aspect Ratio");
  const float origAspect = ctx->lockAspectRatio;
  if (ctx->lockAspectRatio != 0.f) {
    ImGui::SameLine();
    ImGui::Text("locked at %f", ctx->lockAspectRatio);
    if (ImGui::Button("Unlock")) {
      ctx->lockAspectRatio = 0.f;
    }
  } else {
    if (ImGui::Button("Lock")) {
      ctx->lockAspectRatio =
          static_cast<float>(ctx->mainWindow->windowSize.x) / static_cast<float>(ctx->mainWindow->windowSize.y);
    }
    sg::showTooltip("Lock to current aspect ratio");
  }

  ImGui::SetNextItemWidth(5 * ImGui::GetFontSize());
  ImGui::InputFloat("Set", &ctx->lockAspectRatio);
  sg::showTooltip("Lock to custom aspect ratio");
  ctx->lockAspectRatio = std::max(ctx->lockAspectRatio, 0.f);

  if (origAspect != ctx->lockAspectRatio)
    ctx->mainWindow->reshape();

  ImGui::End();
}

void WindowsBuilder::buildWindowKeyframes()
{
  if (!ImGui::Begin("Keyframe editor", &showKeyframes, g_imguiWindowFlags)) {
    ImGui::End();
    return;
  }

  ImGui::SetNextItemWidth(25 * ImGui::GetFontSize());

  if (ImGui::Button("add")) // add current camera state after the selected one
    cameraStack->addCameraState();

  sg::showTooltip(
      "insert a new keyframe after the selected keyframe, based\n"
      "on the current camera state");

  ImGui::SameLine();
  if (ImGui::Button("remove")) // remove the selected camera state
    cameraStack->removeCameraState();

  sg::showTooltip("remove the currently selected keyframe");

  if (cameraStack->size() >= 2) {
    ImGui::SameLine();
    if (ImGui::Button(ctx->g_animatingPath ? "stop" : "play")) {
      ctx->g_animatingPath = !ctx->g_animatingPath;
      cameraStack->g_camCurrentPathIndex = 0;
      // following moved to mainWindowNew display()
      // if (ctx->g_animatingPath)
      //   g_camPath = buildPath(cameraStack, g_camPathSpeed * 0.01);
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(10 * ImGui::GetFontSize());
    ImGui::SliderFloat("speed##path", &cameraStack->g_camPathSpeed, 0.f, 10.0);
    sg::showTooltip(
        "Animation speed for computed path.\n"
        "Slow speeds may cause jitter for small objects");

    static bool showCameraPath = false;
    if (ImGui::Checkbox("show camera path", &showCameraPath)) {
      viewCameraPath(showCameraPath);
    }
  }

  if (ImGui::ListBoxHeader("##")) {
    for (int i = 0; i < cameraStack->size(); i++) {
      if (ImGui::Selectable(
              (std::to_string(i) + ": " + to_string(cameraStack->at(i))).c_str(),
              (cameraStack->g_camSelectedStackIndex == (int)i))) {
                auto valid = cameraStack->selectCamStackIndex(i);
                if (valid)
                ctx->updateCamera();
              }
    }
    ImGui::ListBoxFooter();
  }

  ImGui::End();
}

void WindowsBuilder::buildWindowSnapshots()
{
  if (!ImGui::Begin("Camera snap shots", &showSnapshots, g_imguiWindowFlags)) {
    ImGui::End();
    return;
  }
  ImGui::Text("+ key to add new snapshots");
  for (size_t s = 0; s < cameraStack->size(); s++) {
    if (ImGui::Button(std::to_string(s).c_str())) {
      auto valid = cameraStack->setCameraSnapshot(s);
      if (valid)
        ctx->updateCamera();
    }
  }
  if (cameraStack->size()) {
    if (ImGui::Button("save to cams.json")) {
      std::ofstream cams("cams.json");
      if (cams) {
        JSON j = *cameraStack;
        cams << j;
      }
    }
  }
  ImGui::End();
}

void WindowsBuilder::buildWindowLightEditor()
{
  if (!ImGui::Begin("Light editor", &showLightEditor, g_imguiWindowFlags)) {
    ImGui::End();
    return;
  }

  auto &lights = lightsManager->children();
  static int whichLight = -1;

  // Validate that selected light is still a valid light.  Clear scene will
  // change the lights list, elsewhere.
  if (whichLight >= lights.size())
    whichLight = -1;

  ImGui::Text("lights");
  if (ImGui::ListBoxHeader("", 3)) {
    int i = 0;
    for (auto &light : lights) {
      if (ImGui::Selectable(light.first.c_str(), (whichLight == i))) {
        whichLight = i;
      }
      i++;
    }
    ImGui::ListBoxFooter();

    if (whichLight != -1) {
      ImGui::Text("edit");
      GenerateWidget(lightsManager->child(lights.at_index(whichLight).first));
    }
  }

  if (lights.size() > 1 && ImGui::Button("remove"))
  ctx->removeLight(whichLight);

  ImGui::Separator();

  ImGui::Text("new light");

  static std::string lightType = "";
  if (ImGui::Combo("type##whichLightType",
          &whichLightType,
          lightTypeUI_callback,
          nullptr,
          g_lightTypes.size())) {
    if (whichLightType > -1 && whichLightType < (int)g_lightTypes.size())
      lightType = g_lightTypes[whichLightType];
  }

  static bool lightNameWarning = false;
  static bool lightTexWarning = false;

  static char lightName[64] = "";
  if (ImGui::InputText("name", lightName, sizeof(lightName)))
    lightNameWarning = !(*lightName) || lightsManager->lightExists(lightName);

  // HDRI lights need a texture
  static bool showHDRIFileBrowser = false;
  static rkcommon::FileName texFileName("");
  if (lightType == "hdri") {
    // This field won't be typed into.
    ImGui::InputTextWithHint(
        "texture", "select...", (char *)texFileName.base().c_str(), 0);
    if (ImGui::IsItemClicked())
      showHDRIFileBrowser = true;
  } else
    lightTexWarning = false;

  // Leave the fileBrowser open until file is selected
  if (showHDRIFileBrowser) {
    FileList fileList = {};
    if (fileBrowser(fileList, "Select HDRI Texture")) {
      showHDRIFileBrowser = false;

      lightTexWarning = fileList.empty();
      if (!fileList.empty()) {
        texFileName = fileList[0];
      }
    }
  }

  if ((!lightNameWarning && !lightTexWarning)) {
    if (ImGui::Button("add")) {
      if (lightsManager->addLight(lightName, lightType)) {
        if (lightType == "hdri") {
          auto &hdri = lightsManager->child(lightName);
          hdri["filename"] = texFileName.str();
        }
        // Select newly added light
        whichLight = lights.size() - 1;
      } else {
        lightNameWarning = true;
      }
    }
    if (lightsManager->hasDefaultLight()) {
      auto &rmDefaultLight = lightsManager->rmDefaultLight;
      ImGui::SameLine();
      ImGui::Checkbox("remove default", &rmDefaultLight);
    }
  }

  if (lightNameWarning)
    ImGui::TextColored(
        ImVec4(1.f, 0.f, 0.f, 1.f), "Light must have unique non-empty name");
  if (lightTexWarning)
    ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "No texture provided");

  ImGui::End();
}

void WindowsBuilder::buildWindowCameraEditor()
{
  if (!ImGui::Begin("Camera editor", &showCameraEditor)) {
    ImGui::End();
    return;
  }

  auto frameCameraId = ctx->frame->child("camera").child("cameraId").valueAs<int>();
  ctx->whichCamera = frameCameraId;

  const char* preview_value = NULL;
  auto &items = ctx->g_sceneCameras;

  // Only present selector UI if more than one camera
  if (ImGui::BeginCombo("sceneCameras##whichCamera",
          items.at_index(ctx->whichCamera).first.c_str())) {
    for (int i = 0; i < items.size(); ++i) {
      const bool isSelected = (ctx->whichCamera == i);
      if (ImGui::Selectable(items.at_index(i).first.c_str(), isSelected)) {
        ctx->whichCamera = i;
        ctx->selectCamera();
      }
      if (isSelected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  // Change camera type
  ImGui::Text("Type:");
  static const std::vector<std::string> cameraTypes = {
      "perspective", "orthographic", "panoramic"};
  auto currentType = ctx->frame->childAs<sg::Camera>("camera").subType();
  for (auto &type : cameraTypes) {
    auto newType = "camera_" + type;
    ImGui::SameLine();
    if (ImGui::RadioButton(type.c_str(), currentType == newType)) {
      ctx->createNewCamera(newType);
      break;
    }
  }

  // UI Widget
  GenerateWidget(ctx->frame->childAs<sg::Camera>("camera"));

  ImGui::End();
}

void WindowsBuilder::buildWindowMaterialEditor()
{
  if (!ImGui::Begin("Material editor", &showMaterialEditor)) {
    ImGui::End();
    return;
  }

  static std::vector<sg::NodeType> types{sg::NodeType::MATERIAL};
  static SearchWidget searchWidget(types, types, sg::TreeState::ALLCLOSED);
  static AdvancedMaterialEditor advMaterialEditor;

  searchWidget.addSearchBarUI(*baseMaterialRegistry);
  searchWidget.addSearchResultsUI(*baseMaterialRegistry);
  auto selected = searchWidget.getSelected();
  if (selected) {
    GenerateWidget(*selected);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(245, 200, 66, 255));
    if (ImGui::TreeNodeEx(
            "Advanced options##materials", ImGuiTreeNodeFlags_None)) {
      ImGui::PopStyleColor();
      advMaterialEditor.buildUI(baseMaterialRegistry, selected);
      ImGui::TreePop();
    } else {
      ImGui::PopStyleColor();
    }
  }

  if (baseMaterialRegistry->isModified()) {
    ctx->frame->cancelFrame();
    ctx->frame->waitOnFrame();
  }

  ImGui::End();
}

void WindowsBuilder::buildWindowTransferFunctionEditor()
{
  if (!ImGui::Begin("Transfer Function editor", &showTransferFunctionEditor)) {
    ImGui::End();
    return;
  }

  // Gather all transfer functions in the scene
  sg::CollectTransferFunctions visitor;
  ctx->frame->traverse(visitor);
  auto &transferFunctions = visitor.transferFunctions;

  if (transferFunctions.empty()) {
    ImGui::Text("== empty == ");

  } else {
    ImGui::Text("TransferFunctions");
    static int whichTFn = -1;
    static std::string selected = "";

    // Called by TransferFunctionWidget to update selected TFn
    auto transferFunctionUpdatedCallback =
        [&](const range1f &valueRange,
            const std::vector<vec4f> &colorsAndOpacities) {
          if (whichTFn != -1) {
            auto &tfn =
                *(transferFunctions[selected]->nodeAs<sg::TransferFunction>());
            auto &colors = tfn.colors;
            auto &opacities = tfn.opacities;

            colors.resize(colorsAndOpacities.size());
            opacities.resize(colorsAndOpacities.size());

            // Separate out colors
            std::transform(colorsAndOpacities.begin(),
                colorsAndOpacities.end(),
                colors.begin(),
                [](vec4f c4) { return vec3f(c4[0], c4[1], c4[2]); });

            // Separate out opacities
            std::transform(colorsAndOpacities.begin(),
                colorsAndOpacities.end(),
                opacities.begin(),
                [](vec4f c4) { return c4[3]; });

            tfn.createChildData("color", colors);
            tfn.createChildData("opacity", opacities);
            tfn["value"] = valueRange;
          }
        };

    static TransferFunctionWidget transferFunctionWidget(
        transferFunctionUpdatedCallback,
        range1f(0.f, 1.f),
        "TransferFunctionEditor");

    if (ImGui::ListBoxHeader("", transferFunctions.size())) {
      int i = 0;
      for (auto t : transferFunctions) {
        if (ImGui::Selectable(t.first.c_str(), (whichTFn == i))) {
          whichTFn = i;
          selected = t.first;

          auto &tfn = *(t.second->nodeAs<sg::TransferFunction>());
          const auto numSamples = tfn.colors.size();

          if (numSamples > 1) {
            auto vRange = tfn["value"].valueAs<range1f>();

            // Create a c4 from c3 + opacity
            std::vector<vec4f> c4;

            if (tfn.opacities.size() != numSamples)
              tfn.opacities.resize(numSamples, tfn.opacities.back());

            for (int n = 0; n < numSamples; n++) {
              c4.emplace_back(vec4f(tfn.colors.at(n), tfn.opacities.at(n)));
            }

            transferFunctionWidget.setValueRange(vRange);
            transferFunctionWidget.setColorsAndOpacities(c4);
          }
        }
        i++;
      }
      ImGui::ListBoxFooter();

      ImGui::Separator();
      if (whichTFn != -1) {
        transferFunctionWidget.updateUI();
      }
    }
  }

  ImGui::End();
}

void WindowsBuilder::buildWindowIsosurfaceEditor()
{
  if (!ImGui::Begin("Isosurface editor", &showIsosurfaceEditor)) {
    ImGui::End();
    return;
  }

  // Specialized node vector list box
  using vNodePtr = std::vector<sg::NodePtr>;
  auto ListBox = [](const char *label, int *selected, vNodePtr &nodes) {
    auto getter = [](void *vec, int index, const char **name) {
      auto nodes = static_cast<vNodePtr *>(vec);
      if (0 > index || index >= (int)nodes->size())
        return false;
      // Need longer lifetime than this lambda?
      static std::string copy = "";
      copy = nodes->at(index)->name();
      *name = copy.data();
      return true;
    };

    if (nodes.empty())
      return false;
    return ImGui::ListBox(
        label, selected, getter, static_cast<void *>(&nodes), nodes.size());
  };

  // Gather all volumes in the scene
  vNodePtr volumes = {};
  for (auto &node : ctx->frame->child("world").children())
    if (node.second->type() == sg::NodeType::GENERATOR
        || node.second->type() == sg::NodeType::IMPORTER
        || node.second->type() == sg::NodeType::VOLUME) {
      auto volume = findFirstNodeOfType(node.second, sg::NodeType::VOLUME);
      if (volume)
        volumes.push_back(volume);
    }

  ImGui::Text("Volumes");
  ImGui::Text("(select to create an isosurface)");
  if (volumes.empty()) {
    ImGui::Text("== empty == ");

  } else {
    static int currentVolume = 0;
    if (ListBox("##Volumes", &currentVolume, volumes))
    ctx->createIsoSurface(currentVolume, volumes);
  }

  // Gather all isosurfaces in the scene
  vNodePtr surfaces = {};
  for (auto &node : ctx->frame->child("world").children())
    if (node.second->type() == sg::NodeType::GENERATOR
        || node.second->type() == sg::NodeType::IMPORTER
        || node.second->type() == sg::NodeType::TRANSFORM) {
      auto surface = findFirstNodeOfType(node.second, sg::NodeType::GEOMETRY);
      if (surface && (surface->subType() == "geometry_isosurfaces"))
        surfaces.push_back(surface);
    }

  ImGui::Separator();
  ImGui::Text("Isosurfaces");
  if (surfaces.empty()) {
    ImGui::Text("== empty == ");
  } else {
    for (auto &surface : surfaces) {
      GenerateWidget(*surface);
      if (surface->isModified())
        break;
    }
  }

  ImGui::End();
}

void WindowsBuilder::buildWindowTransformEditor()
{
  if (!ImGui::Begin("Transform Editor", &showTransformEditor)) {
    ImGui::End();
    return;
  }

  typedef sg::NodeType NT;

  auto toggleSearch = [&](sg::SearchResults &results, bool visible) {
    for (auto result : results) {
      auto resultNode = result.lock();
      if (resultNode->hasChild("visible"))
        resultNode->child("visible").setValue(visible);
    }
  };
  auto showSearch = [&](sg::SearchResults &r) { toggleSearch(r, true); };
  auto hideSearch = [&](sg::SearchResults &r) { toggleSearch(r, false); };

  auto &warudo = ctx->frame->child("world");
  auto toggleDisplay = [&](bool visible) {
    warudo.traverse<sg::SetParamByNode>(NT::GEOMETRY, "visible", visible);
    warudo.traverse<sg::SetParamByNode>(NT::VOLUME, "visible", visible);
  };
  auto showDisplay = [&]() { toggleDisplay(true); };
  auto hideDisplay = [&]() { toggleDisplay(false); };

  static std::vector<NT> searchTypes{
      NT::IMPORTER, NT::TRANSFORM, NT::GENERATOR, NT::GEOMETRY, NT::VOLUME};
  static std::vector<NT> displayTypes{
      NT::IMPORTER, NT::TRANSFORM, NT::GENERATOR};
  static SearchWidget searchWidget(searchTypes, displayTypes);

  searchWidget.addSearchBarUI(warudo);
  searchWidget.addCustomAction("show all", showSearch, showDisplay);
  searchWidget.addCustomAction("hide all", hideSearch, hideDisplay, true);
  searchWidget.addSearchResultsUI(warudo);

  auto selected = searchWidget.getSelected();
  if (selected) {
    auto toggleSelected = [&](bool visible) {
      selected->traverse<sg::SetParamByNode>(NT::GEOMETRY, "visible", visible);
      selected->traverse<sg::SetParamByNode>(NT::VOLUME, "visible", visible);
    };

    ImGui::Text("Selected ");
    ImGui::SameLine();
    if (ImGui::Button("show"))
      toggleSelected(true);
    ImGui::SameLine();
    if (ImGui::Button("hide"))
      toggleSelected(false);

    GenerateWidget(*selected);
  }

  ImGui::End();
}

void WindowsBuilder::buildWindowRenderingStats()
{
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
      | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings
      | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav
      | ImGuiWindowFlags_NoMove;
  ImGui::SetNextWindowBgAlpha(0.75f);

  // Bottom left corner
  static const float FROM_EDGE = 10.f;
  ImVec2 window_pos(FROM_EDGE, ImGui::GetIO().DisplaySize.y - FROM_EDGE);
  ImVec2 window_pos_pivot(0.f, 1.f);
  ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);

  if (!ImGui::Begin("Rendering stats", &showRenderingStats, flags)) {
    ImGui::End();
    return;
  }

  auto &fb = ctx->frame->childAs<sg::FrameBuffer>("framebuffer");
  auto variance = fb.variance();
  auto &v = ctx->frame->childAs<sg::Renderer>("renderer")["varianceThreshold"];
  auto varianceThreshold = v.valueAs<float>();

  std::string mode = ctx->frame->child("navMode").valueAs<bool>() ? "Nav" : "";
  float scale = ctx->frame->child("scale" + mode).valueAs<float>();

  ImGui::Text("renderer: %s", rendererTypeStr.c_str());
  ImGui::Text("frame size: (%d,%d)", ctx->mainWindow->windowSize.x, ctx->mainWindow->windowSize.y);
  ImGui::SameLine();
  ImGui::Text("x%1.2f", scale);
  ImGui::Text("framerate: %-4.1f fps", ctx->mainWindow->latestFPS);
  ImGui::Text("ui framerate: %-4.1f fps", ImGui::GetIO().Framerate);

  if (varianceThreshold == 0) {
    ImGui::Text("variance    : %-5.2f    ", variance);
  } else {
    ImGui::Text("variance    :");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(8 * ImGui::GetFontSize());
    float progress = varianceThreshold / variance;
    char message[64];
    snprintf(
        message, sizeof(message), "%.2f/%.2f", variance, varianceThreshold);
    ImGui::ProgressBar(progress, ImVec2(0.f, 0.f), message);
  }

  if (ctx->frame->accumLimit == 0) {
    ImGui::Text("accumulation: %d", ctx->frame->currentAccum);
  } else {
    ImGui::Text("accumulation:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(8 * ImGui::GetFontSize());
    float progress = float(ctx->frame->currentAccum) / ctx->frame->accumLimit;
    char message[64];
    snprintf(message,
        sizeof(message),
        "%d/%d",
        ctx->frame->currentAccum,
        ctx->frame->accumLimit);
    ImGui::ProgressBar(progress, ImVec2(0.f, 0.f), message);
    auto remaining = ctx->frame->accumLimit - ctx->frame->currentAccum;
    if (remaining > 0) {
      auto secondsPerFrame = 1.f / (ctx->mainWindow->latestFPS + 1e-6);
      ImGui::SameLine();
      ImGui::Text(
          "ETA: %4.2f s", int(remaining * secondsPerFrame * 100.f) / 100.f);
    }
  }

  ImGui::End();
}

void WindowsBuilder::viewCameraPath(bool showCameraPath)
{
  if (!showCameraPath) {
    ctx->frame->child("world").remove("cameraPath_xfm");
    ctx->refreshScene(false);
  } else {
    auto pathXfm = sg::createNode("cameraPath_xfm", "transform");

    const auto &worldBounds = ctx->frame->child("world").bounds();
    float pathRad = 0.0075f * reduce_min(worldBounds.size());
    auto cameraPath = cameraStack->buildPath();
    std::vector<vec4f> pathVertices; // position and radius
    for (const auto &state : cameraPath)
      pathVertices.emplace_back(state.position(), pathRad);
    pathVertices.emplace_back(cameraStack->back().position(), pathRad);

    std::vector<uint32_t> indexes(pathVertices.size());
    std::iota(indexes.begin(), indexes.end(), 0);

    std::vector<vec4f> colors(pathVertices.size());
    std::fill(colors.begin(), colors.end(), vec4f(0.8f, 0.4f, 0.4f, 1.f));

    const std::vector<uint32_t> mID = {
        static_cast<uint32_t>(baseMaterialRegistry->baseMaterialOffSet())};
    auto mat = sg::createNode("pathGlass", "thinGlass");
    baseMaterialRegistry->add(mat);

    auto path = sg::createNode("cameraPath", "geometry_curves");
    path->createChildData("vertex.position_radius", pathVertices);
    path->createChildData("vertex.color", colors);
    path->createChildData("index", indexes);
    path->createChild("type", "uchar", (unsigned char)OSP_ROUND);
    path->createChild("basis", "uchar", (unsigned char)OSP_CATMULL_ROM);
    path->createChildData("material", mID);
    path->child("material").setSGOnly();

    std::vector<vec3f> capVertexes;
    std::vector<vec4f> capColors;
    for (int i = 0; i < cameraStack->size(); i++) {
      capVertexes.push_back(cameraStack->at(i).position());
      if (i == 0)
        capColors.push_back(vec4f(.047f, .482f, .863f, 1.f));
      else
        capColors.push_back(vec4f(vec3f(0.8f), 1.f));
    }

    auto caps = sg::createNode("cameraPathCaps", "geometry_spheres");
    caps->createChildData("sphere.position", capVertexes);
    caps->createChildData("color", capColors);
    caps->child("color").setSGOnly();
    caps->child("radius") = pathRad * 1.5f;
    caps->createChildData("material", mID);
    caps->child("material").setSGOnly();

    pathXfm->add(path);
    pathXfm->add(caps);

    ctx->frame->child("world").add(pathXfm);
  }
}
