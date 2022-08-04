
// imgui
#include "imgui.h"

#include "GUIContext.h"
#include "WindowsBuilder.h"

// rkcommon
#include "rkcommon/math/rkmath.h"
#include "rkcommon/os/FileName.h"
#include "rkcommon/utility/DataView.h"
#include "rkcommon/utility/SaveImage.h"

#include "sg/exporter/Exporter.h"
#include "sg/importer/Importer.h"

using namespace ospray_studio;

// Main menu //////////////////////////////////////////////////////////////////

struct MainMenuBuilder
{
 public:
  MainMenuBuilder(std::shared_ptr<GUIContext> _ctx, std::shared_ptr<WindowsBuilder> windowsBuilder);
  ~MainMenuBuilder() = default;
  void start();
  void buildMainMenu();
  void buildMainMenuFile();
  void buildMainMenuEdit();
  void buildMainMenuView();
  void buildMainMenuPlugins();

  //static member variables
  static std::vector<std::string> g_scenes;
  static bool g_clearSceneConfirm;
  static bool stringVec_callback(void *data, int index, const char **out_text);

  // main Util instance
  std::shared_ptr<GUIContext> ctx = nullptr;
  std::shared_ptr<WindowsBuilder> windowsBuilder = nullptr;
};

bool MainMenuBuilder::stringVec_callback(void *data, int index, const char **out_text)
{
  *out_text = ((std::string *)data)[index].c_str();
  return true;
};

// initialize all static variables
std::vector<std::string> MainMenuBuilder::g_scenes = {"tutorial_scene",
    "sphere",
    "particle_volume",
    "random_spheres",
    "wavelet",
    "wavelet_slices",
    "torus_volume",
    "unstructured_volume",
    "multilevel_hierarchy"};
bool MainMenuBuilder::g_clearSceneConfirm = false;

MainMenuBuilder::MainMenuBuilder(std::shared_ptr<GUIContext> _ctx,
    std::shared_ptr<WindowsBuilder> _windowsBuilder)
    : ctx(_ctx), windowsBuilder(_windowsBuilder)
{}

void MainMenuBuilder::start()
{
  // build main menu bar and options
  ImGui::BeginMainMenuBar();
  buildMainMenuFile();
  buildMainMenuEdit();
  buildMainMenuView();
  buildMainMenuPlugins();
  ImGui::EndMainMenuBar();
}

void MainMenuBuilder::buildMainMenuFile()
{
  static bool showImportFileBrowser = false;

  if (ImGui::BeginMenu("File")) {
    if (ImGui::MenuItem("Import ...", nullptr))

      showImportFileBrowser = true;
    if (ImGui::BeginMenu("Demo Scene")) {
      for (size_t i = 0; i < g_scenes.size(); ++i) {
        if (ImGui::MenuItem(g_scenes[i].c_str(), nullptr)) {
          ctx->scene = g_scenes[i];
          ctx->refreshScene(true);
        }
      }
      ImGui::EndMenu();
    }
    ImGui::Separator();
    if (ImGui::BeginMenu("Save")) {
      if (ImGui::MenuItem("Scene (entire)"))
        ctx->saveSGScene();

      ImGui::Separator();
      if (ImGui::MenuItem("Materials (only)"))
        ctx->saveNodesJson("baseMaterialRegistry");
      if (ImGui::MenuItem("Lights (only)"))
        ctx->saveNodesJson("lightsManager");
      if (ImGui::MenuItem("Camera (only)"))
        ctx->saveNodesJson("camera");

      ImGui::Separator();

      if (ImGui::MenuItem("Screenshot", "Ctrl+S", nullptr))
        ctx->saveCurrentFrame();

      static const std::vector<std::string> screenshotFiletypes =
          sg::getExporterTypes();

      static int screenshotFiletypeChoice =
          std::distance(screenshotFiletypes.begin(),
              std::find(screenshotFiletypes.begin(),
                  screenshotFiletypes.end(),
                  ctx->optImageFormat));

      ImGui::SetNextItemWidth(5.f * ImGui::GetFontSize());
      if (ImGui::Combo("##screenshot_filetype",
              (int *)&screenshotFiletypeChoice,
              stringVec_callback,
              (void *)screenshotFiletypes.data(),
              screenshotFiletypes.size())) {
        ctx->optImageFormat = screenshotFiletypes[screenshotFiletypeChoice];
      }
      sg::showTooltip("Image filetype for saving screenshots");

      if (ctx->optImageFormat == "exr") {
        // the following options should be available only when FB format is
        // float.
        auto &fb = ctx->frame->childAs<sg::FrameBuffer>("framebuffer");
        auto fbFloatFormat = fb["floatFormat"].valueAs<bool>();
        if (ImGui::Checkbox("FB float format ", &fbFloatFormat))
          fb["floatFormat"] = fbFloatFormat;
        if (fbFloatFormat) {
          ImGui::Checkbox("albedo##saveAlbedo", &ctx->optSaveAlbedo);
          ImGui::SameLine();
          ImGui::Checkbox("layers as separate files", &ctx->optSaveLayersSeparately);
          ImGui::Checkbox("depth##saveDepth", &ctx->optSaveDepth);
          ImGui::Checkbox("normal##saveNormal", &ctx->optSaveNormal);
        }
      }

      ImGui::EndMenu();
    }

    ImGui::Separator();
    // Remove Quit option if in "show mode" to make it more difficult to exit
    // by mistake.
    auto showMode = rkcommon::utility::getEnvVar<int>("OSPSTUDIO_SHOW_MODE");
    if (showMode) {
      ImGui::TextColored(
          ImVec4(.8f, .2f, .2f, 1.f), "ShowMode, use ctrl-Q to exit");
    } else {
      if (ImGui::MenuItem("Quit", "Alt+F4"))
        ctx->mainWindow->quitNextFrame();
    }

    ImGui::EndMenu();
  }

  // Leave the fileBrowser open until files are selected
  if (showImportFileBrowser) {
    if (fileBrowser(ctx->filesToImport, "Select Import File(s) - ", true)) {
      showImportFileBrowser = false;
      // do not reset camera when loading a scene file
      bool resetCam = true;
      for (auto &fn : ctx->filesToImport)
        if (rkcommon::FileName(fn).ext() == "sg")
          resetCam = false;
      ctx->refreshScene(resetCam);
    }
  }
}

void MainMenuBuilder::buildMainMenuEdit()
{
  if (ImGui::BeginMenu("Edit")) {
    // Scene stuff /////////////////////////////////////////////////////

    if (ImGui::MenuItem("Lights...", "", nullptr))
      windowsBuilder->showLightEditor = true;
    if (ImGui::MenuItem("Transforms...", "", nullptr))
      windowsBuilder->showTransformEditor = true;
    if (ImGui::MenuItem("Materials...", "", nullptr))
      windowsBuilder->showMaterialEditor = true;
    if (ImGui::MenuItem("Transfer Functions...", "", nullptr))
      windowsBuilder->showTransferFunctionEditor = true;
    if (ImGui::MenuItem("Isosurfaces...", "", nullptr))
      windowsBuilder->showIsosurfaceEditor = true;
    ImGui::Separator();

    if (ImGui::MenuItem("Clear scene"))
      g_clearSceneConfirm = true;

    ImGui::EndMenu();
  }

  if (g_clearSceneConfirm) {
    g_clearSceneConfirm = false;
    ImGui::OpenPopup("Clear scene");
  }

  if (ImGui::BeginPopupModal("Clear scene")) {
    ImGui::Text("Are you sure you want to clear the scene?");
    ImGui::Text("This will delete all objects, materials and lights.");

    if (ImGui::Button("No!"))
      ImGui::CloseCurrentPopup();
    ImGui::SameLine(ImGui::GetWindowWidth() - (8 * ImGui::GetFontSize()));

    if (ImGui::Button("Yes, clear it")) {
      ctx->clearScene();
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void MainMenuBuilder::buildMainMenuView()
{
  static bool showFileBrowser = false;
  static bool guizmoOn = false;
  if (ImGui::BeginMenu("View")) {
    // Camera stuff ////////////////////////////////////////////////////

    if (ImGui::MenuItem("Camera...", "", nullptr))
      windowsBuilder->showCameraEditor = true;
    if (ImGui::MenuItem("Center camera", "", nullptr)) {
      ctx->mainWindow->resetArcball();
      ctx->updateCamera();
    }

    static bool lockUpDir = false;
    if (ImGui::Checkbox("Lock UpDir", &lockUpDir))
      ctx->mainWindow->setLockUpDir(lockUpDir);

    if (lockUpDir) {
      ImGui::SameLine();
      static int dir = 1;
      static int _dir = -1;
      ImGui::RadioButton("X##setUpDir", &dir, 0);
      ImGui::SameLine();
      ImGui::RadioButton("Y##setUpDir", &dir, 1);
      ImGui::SameLine();
      ImGui::RadioButton("Z##setUpDir", &dir, 2);
      if (dir != _dir) {
        ctx->mainWindow->setUpDir(vec3f(dir == 0, dir == 1, dir == 2));
        _dir = dir;
      }
    }

    ImGui::Text("Camera Movement Speed:");
    ImGui::SetNextItemWidth(5 * ImGui::GetFontSize());
    ImGui::SliderFloat("Speed##camMov", &ctx->mainWindow->maxMoveSpeed, 0.1f, 5.0f);
    ImGui::SetNextItemWidth(5 * ImGui::GetFontSize());
    ImGui::SliderFloat(
        "fineControl##camMov", &ctx->mainWindow->fineControl, 0.1f, 1.0f, "%0.2fx");
    sg::showTooltip("hold <left-Ctrl> for more sensitive camera movement.");

    ImGui::Separator();

    if (ImGui::MenuItem("Animation Controls...", "", nullptr))
      ctx->mainWindow->animationSetShowUI();

    if (ImGui::MenuItem("Keyframes...", "", nullptr))
      windowsBuilder->showKeyframes = true;
    if (ImGui::MenuItem("Snapshots...", "", nullptr))
      windowsBuilder->showSnapshots = true;

    ImGui::Checkbox("Auto rotate", &ctx->optAutorotate);
    if (ctx->optAutorotate) {
      ImGui::SameLine();
      ImGui::SetNextItemWidth(5 * ImGui::GetFontSize());
      ImGui::SliderInt(" speed", &ctx->mainWindow->autorotateSpeed, 1, 100);
    }
    ImGui::Separator();

    // Renderer stuff //////////////////////////////////////////////////

    if (ImGui::MenuItem("Renderer..."))
      windowsBuilder->showRendererEditor = true;

    auto &renderer = ctx->frame->childAs<sg::Renderer>("renderer");

    ImGui::Checkbox("Rendering stats", &windowsBuilder->showRenderingStats);
    ImGui::Checkbox("Pause rendering", &ctx->frame->pauseRendering);
    ImGui::SetNextItemWidth(5 * ImGui::GetFontSize());
    ImGui::DragInt(
        "Limit accumulation", &ctx->frame->accumLimit, 1, 0, INT_MAX, "%d frames");

    // Although varianceThreshold and backgroundColor are found in the
    // renderer UI, also add them here to make them easier to find.
    ImGui::SetNextItemWidth(5 * ImGui::GetFontSize());
    GenerateWidget(renderer["varianceThreshold"]);
    GenerateWidget(renderer["backgroundColor"]);

    if (ImGui::MenuItem("Set background texture..."))
      showFileBrowser = true;
    if (!ctx->backPlateTexture.str().empty()) {
      ImGui::TextColored(ImVec4(.5f, .5f, .5f, 1.f),
          "current: %s",
          ctx->backPlateTexture.base().c_str());
      if (ImGui::MenuItem("Clear background texture")) {
        ctx->backPlateTexture = "";
        ctx->refreshRenderer();
      }
    }

    // Framebuffer and window stuff ////////////////////////////////////
    ImGui::Separator();
    if (ImGui::MenuItem("Framebuffer..."))
      windowsBuilder->showFrameBufferEditor = true;
    if (ImGui::BeginMenu("Quick window size")) {
      const std::vector<vec2i> options = {{480, 270},
          {960, 540},
          {1280, 720},
          {1920, 1080},
          {2560, 1440},
          {3840, 2160}};
      for (auto &sizeChoice : options) {
        char label[64];
        snprintf(label,
            sizeof(label),
            "%s%d x %d",
            ctx->mainWindow->windowSize == sizeChoice ? "*" : " ",
            sizeChoice.x,
            sizeChoice.y);
        if (ImGui::MenuItem(label))
          ctx->mainWindow->reshape(sizeChoice, true);

      }
      ImGui::EndMenu();
    }
    ImGui::Checkbox("Display as sRGB", &ctx->mainWindow->uiDisplays_sRGB);
    sg::showTooltip(
        "Display linear framebuffers as sRGB,\n"
        "maintains consistent display across all formats.");
    // Allows the user to cancel long frame renders, such as too-many spp or
    // very large resolution.  Don't wait on the frame-cancel completion as
    // this locks up the UI.  Note: Next frame-start following frame
    // cancellation isn't immediate.
    if (ImGui::MenuItem("Cancel frame"))
      ctx->frame->cancelFrame();

    ImGui::Separator();

    // UI options //////////////////////////////////////////////////////
    ImGui::Text("ui options");

    ImGui::Checkbox("Show tooltips", &g_ShowTooltips);
    if (g_ShowTooltips) {
      ImGui::SameLine();
      ImGui::SetNextItemWidth(5 * ImGui::GetFontSize());
      ImGui::DragInt("delay", &g_TooltipDelay, 50, 0, 1000, "%d ms");
    }

#if 1 // XXX example new features that need to be integrated
    ImGui::Separator();

    ImGuiIO &io = ImGui::GetIO();
    ImGui::CheckboxFlags(
        "DockingEnable", &io.ConfigFlags, ImGuiConfigFlags_DockingEnable);
    sg::showTooltip("[experimental] Menu docking");
    ImGui::CheckboxFlags(
        "ViewportsEnable", &io.ConfigFlags, ImGuiConfigFlags_ViewportsEnable);
    sg::showTooltip("[experimental] Mind blowing multi-viewports support");

    ImGui::Checkbox("Guizmo", &guizmoOn);

    ImGui::Text("...right-click to open pie menu...");
    pieMenu();
#endif

    ImGui::EndMenu();
  }

#if 1 // Guizmo shows outsize menu window
  if (guizmoOn) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
        | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav
        | ImGuiWindowFlags_NoMove;
    ImGui::SetNextWindowBgAlpha(0.75f);

    // Bottom right corner
    ImVec2 window_pos(
        ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
    ImVec2 window_pos_pivot(1.f, 1.f);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);

    if (ImGui::Begin("###guizmo", &guizmoOn, flags)) {
      guizmo();
      ImGui::End();
    }
  }
#endif

  // Leave the fileBrowser open until file is selected
  if (showFileBrowser) {
    FileList fileList = {};
    if (fileBrowser(fileList, "Select Background Texture")) {
      showFileBrowser = false;

      if (!fileList.empty()) {
        ctx->backPlateTexture = fileList[0];
        ctx->refreshRenderer();
      }
    }
  }
}

void MainMenuBuilder::buildMainMenuPlugins()
{
  auto &pPanels = ctx->pluginPanels;
  if (!pPanels.empty() && ImGui::BeginMenu("Plugins")) {
    for (auto &p : pPanels)
      if (ImGui::MenuItem(p->name().c_str()))
        p->setShown(true);

    ImGui::EndMenu();
  }
}
