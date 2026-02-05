#include <ctime>

#include "PanelGesture.h"

#include "app/widgets/GenerateImGuiWidgets.h"
#include "app/MainWindow.h"

#include "imgui.h"


namespace ospray {
namespace gesture_plugin {

PanelGesture::PanelGesture(std::shared_ptr<StudioContext> _context, std::string _panelName, std::string _configFilePath)
    : Panel(_panelName.c_str(), _context)
    , panelName(_panelName)
    , configFilePath(_configFilePath)
{
  trackingManager.reset(new TrackingManager(configFilePath));
}

void PanelGesture::processRequests() {
  if (!trackingManager->isRunning())
    return;
  
  TrackingState state = trackingManager->pollState();
  if (state.mode == INTERACTION_FLYING) {
    MainWindow* pMW = reinterpret_cast<MainWindow*>(context->getMainWindow());
    
    vec3f pos = pMW->arcballCamera->center();
    vec3f center = state.leaningDir + pos;
    pMW->arcballCamera->setCenter(center);
    context->updateCamera();
  }
}

void PanelGesture::buildUI(void *ImGuiCtx)
{
  processRequests();

  // Allows plugin to still do other work if the UI isn't shown.
  if (!isShown())
    return;

  // Need to set ImGuiContext in *this* address space
  ImGui::SetCurrentContext((ImGuiContext *)ImGuiCtx);
  ImGui::OpenPopup(panelName.c_str());

  if (!ImGui::BeginPopupModal(panelName.c_str(), nullptr, ImGuiWindowFlags_None)) return;

  if (trackingManager->isRunning()) {
    ImGui::Text("%s", "Currently connected to the server...");

    if (ImGui::Button("Disconnect")) {
      trackingManager->close();
    }
  }
  else {
    ImGui::Text("%s", "Currently NOT connected to the server...");
    std::string str = "- Connect to " + trackingManager->ipAddress + ":" + std::to_string(trackingManager->portNumber);
    ImGui::Text("%s", str.c_str());

    if (ImGui::Button("Connect")) {
      trackingManager->start();
    }
  }
  ImGui::Separator();

  // Close button
  if (ImGui::Button("Close")) {
    setShown(false);
    ImGui::CloseCurrentPopup();
  }
  ImGui::Separator();

  if (ImGui::CollapsingHeader("Configuration", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Text("%s", "Offset(s)");
    ImGui::DragFloat3("Scale", trackingManager->scaleOffset, 0.001, -100, 100, "%.3f");
    ImGui::DragFloat3("Translate", trackingManager->translationOffset, 0, -100, 100, "%.1f");

    ImGui::Text("%s", "Gestures(s)");
    ImGui::DragInt("Confidence Level Threshold", &trackingManager->confidenceLevelThreshold, K4ABT_JOINT_CONFIDENCE_LOW, K4ABT_JOINT_CONFIDENCE_NONE, K4ABT_JOINT_CONFIDENCE_LEVELS_COUNT);
    ImGui::DragFloat("Leaning Angle Threshold", &trackingManager->leaningAngleThreshold, 1, 0, 180);
    ImGui::DragFloat3("Leaning Dir Scale", trackingManager->leaningDirScaleFactor, 0, -100, 100, "%.1f");

    ImGui::Separator();
    if (ImGui::Button("Save")) {
      trackingManager->saveConfig(this->configFilePath);
    }
  }
  ImGui::Separator();

  // Display statuses in a scrolling region
  if (ImGui::CollapsingHeader("Status", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::BeginChild("Scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
    for (std::string status : trackingManager->statuses) {
      ImGui::Text("%s", status.c_str());
    }
    ImGui::EndChild();
  }
  ImGui::Separator();

  ImGui::EndPopup();
}

}  // namespace gesture_plugin
}  // namespace ospray
