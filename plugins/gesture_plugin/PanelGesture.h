#pragma once

#include "app/widgets/Panel.h"
#include "app/ospStudio.h"

#include "tracker/TrackingManager.h"

namespace ospray {
namespace gesture_plugin {

struct PanelGesture : public Panel
{
  PanelGesture(std::shared_ptr<StudioContext> _context, std::string _panelName, std::string _configFilePath);

  void buildUI(void *ImGuiCtx) override;

  void processRequests();

private:
  std::string panelName;
  std::string configFilePath;
  std::unique_ptr<TrackingManager> trackingManager;
};

}  // namespace gesture_plugin
}  // namespace ospray
