#pragma once

#include "app/widgets/Panel.h"
#include "app/ospStudio.h"

#include "request/RequestManager.h"

namespace ospray {
namespace storyboard_plugin {

struct PanelStoryboard : public Panel
{
  PanelStoryboard(std::shared_ptr<StudioContext> _context, std::string _panelName, std::string _configFilePath);

  void buildUI(void *ImGuiCtx) override;

  void processRequests();

private:
  std::string panelName;
  std::string configFilePath;
  std::unique_ptr<RequestManager> requestManager;
};

}  // namespace storyboard_plugin
}  // namespace ospray
