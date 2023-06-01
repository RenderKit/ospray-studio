#include <ctime>

#include "PanelStoryboard.h"

#include "app/widgets/GenerateImGuiWidgets.h"

#include "rkcommon/math/vec.h"

#include "sg/JSONDefs.h"
#include "sg/Math.h"
#include "sg/JSONDefs.h"

#include "imgui.h"

namespace ospray {
namespace storyboard_plugin {

PanelStoryboard::PanelStoryboard(std::shared_ptr<StudioContext> _context, std::string _panelName, std::string _configFilePath)
    : Panel(_panelName.c_str(), _context)
    , panelName(_panelName)
    , configFilePath(_configFilePath)
{
  requestManager.reset(new RequestManager("localhost", 8888));

  requestManager->start();
}

void PanelStoryboard::buildUI(void *ImGuiCtx)
{
  // Need to set ImGuiContext in *this* address space
  ImGui::SetCurrentContext((ImGuiContext *)ImGuiCtx);
  ImGui::OpenPopup(panelName.c_str());

  if (!ImGui::BeginPopupModal(panelName.c_str(), nullptr, ImGuiWindowFlags_None)) return;

  if (requestManager->isRunning()) {
    ImGui::Text("%s", "Currently connected to the storyboard server...");

    if (ImGui::Button("Disconnect")) {
      requestManager->close();
    }
  }
  else {
    ImGui::Text("%s", "Currently NOT connected to the storyboard server...");
    std::string str = "- Connect to " + requestManager->ipAddress + ":" + std::to_string(requestManager->portNumber);
    ImGui::Text("%s", str.c_str());

    if (ImGui::Button("Connect")) {
      requestManager->start();
    }
  }
  ImGui::Separator();

  // Close button
  if (ImGui::Button("Close")) {
    setShown(false);
    ImGui::CloseCurrentPopup();
  }
  ImGui::Separator();

  // if (ImGui::CollapsingHeader("Configuration", ImGuiTreeNodeFlags_DefaultOpen)) {
  //   ImGui::Text("%s", "Offset(s)");
  //   ImGui::DragFloat3("Scale", storyboardManager->scaleOffset, 0.001, -100, 100, "%.3f");
  //   ImGui::DragFloat3("Translate", storyboardManager->translationOffset, 0, -100, 100, "%.1f");
    
  //   ImGui::Separator();
  //   if (ImGui::Button("Save")) {
  //     storyboardManager->saveConfig(this->configFilePath);
  //   }
  // }
  ImGui::Separator();

  // Display statuses in a scrolling region
  if (ImGui::CollapsingHeader("Status", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::BeginChild("Scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
    for (std::string status : requestManager->statuses) {
      ImGui::Text("%s", status.c_str());
    }
    ImGui::EndChild();
  }
  ImGui::Separator();

  ImGui::EndPopup();
}

void PanelStoryboard::process(std::string key) {
  if (key == "update") {
    std::queue<nlohmann::ordered_json> requests;
    requestManager->pollRequests(requests);

    std::cout << "Requests... " << requests.size() << std::flush << std::endl;

    while (!requests.empty()) {
      nlohmann::ordered_json req = requests.front();

      if (req.contains("type") && req["type"] == "request" && 
          req.contains("action") && req["action"] == "capture.view") {
        
        nlohmann::ordered_json j = 
        {
          {"type", "response"},
          {"action", "capture.view"},
          {"snapshotIdx", req["snapshotIdx"].get<int>()},
          {"camera", context->arcballCamera->getState()}
        };
        requestManager->send(j.dump());
      }
      else if (req.contains("type") && req["type"] == "request" && 
          req.contains("action") && req["action"] == "capture.image") {
        
        // 1) capture the image
        vec2i size{16, 16};
        std::vector<uint8_t> values
        {
          48,50,50,50,231,48,170,127,50,50,50,50,249,64,188,127,3,3,3,3,246,48,2,5,3,3,3,3,244,48,3,6,50,50,50,50,247,64,170,127,50,242,2,168,231,48,255,255,3,3,3,255,230,64,0,15,0,255,0,170,233,64,159,255,91,3,3,3,202,106,15,48,3,3,3,255,202,104,15,48,170,148,144,64,186,91,175,104,64,0,0,255,202,88,15,32,0,0,0,255,230,64,1,44,0,255,0,170,219,65,255,255,0,0,0,255,232,64,1,28,0,255,0,170,187,64,255,255
        };

        // auto binary = nlohmann::ordered_json::binary_t(
        // {
        //     0x30, 0x32, 0x32, 0x32, 0xe7, 0x30, 0xaa, 0x7f, 0x32, 0x32, 0x32, 0x32, 0xf9, 0x40, 0xbc, 0x7f,
        //     0x03, 0x03, 0x03, 0x03, 0xf6, 0x30, 0x02, 0x05, 0x03, 0x03, 0x03, 0x03, 0xf4, 0x30, 0x03, 0x06,
        //     0x32, 0x32, 0x32, 0x32, 0xf7, 0x40, 0xaa, 0x7f, 0x32, 0xf2, 0x02, 0xa8, 0xe7, 0x30, 0xff, 0xff,
        //     0x03, 0x03, 0x03, 0xff, 0xe6, 0x40, 0x00, 0x0f, 0x00, 0xff, 0x00, 0xaa,  0xe9, 0x40, 0x9f, 0xff,
        //     0x5b, 0x03, 0x03, 0x03, 0xca, 0x6a, 0x0f, 0x30, 0x03, 0x03, 0x03, 0xff, 0xca, 0x68, 0x0f, 0x30,
        //     0xaa, 0x94, 0x90, 0x40, 0xba, 0x5b, 0xaf, 0x68, 0x40, 0x00, 0x00, 0xff, 0xca, 0x58, 0x0f, 0x20,
        //     0x00, 0x00, 0x00, 0xff, 0xe6, 0x40, 0x01, 0x2c, 0x00, 0xff, 0x00, 0xaa, 0xdb, 0x41, 0xff, 0xff,
        //     0x00, 0x00, 0x00, 0xff, 0xe8, 0x40, 0x01, 0x1c, 0x00, 0xff, 0x00, 0xaa, 0xbb, 0x40, 0xff, 0xff,
        // });

        // auto binary = nlohmann::ordered_json::binary_t(
        // {
        //   48,50,50,50,231,48,170,127,50,50,50,50,249,64,188,127,3,3,3,3,246,48,2,5,3,3,3,3,244,48,3,6,50,50,50,50,247,64,170,127,50,242,2,168,231,48,255,255,3,3,3,255,230,64,0,15,0,255,0,170,233,64,159,255,91,3,3,3,202,106,15,48,3,3,3,255,202,104,15,48,170,148,144,64,186,91,175,104,64,0,0,255,202,88,15,32,0,0,0,255,230,64,1,44,0,255,0,170,219,65,255,255,0,0,0,255,232,64,1,28,0,255,0,170,187,64,255,255
        // });

        // 2) send the setup information
        nlohmann::ordered_json jSetUp = 
        {
          {"type", "response"},
          {"action", "capture.image.setup"},
          {"snapshotIdx", req["snapshotIdx"].get<int>()},
          {"width", size.x},
          {"height", size.y}
        };
        requestManager->send(jSetUp.dump());

        // 3) send the image data in chunks
        int start = 0;
        int chunckLen = 16 * 1;
        while (start < size.x * size.y) {
          int len = (start + chunckLen) > (size.x * size.y) ? (size.y * size.y - start) : chunckLen;
          std::vector<uint8_t> chunk = std::vector<uint8_t>(values.begin() + start, values.begin() + start + len);
          auto binary = nlohmann::ordered_json::binary_t(chunk);

          nlohmann::ordered_json jChunk = 
          {
            {"type", "response"},
            {"action", "capture.image.data"},
            {"snapshotIdx", req["snapshotIdx"].get<int>()},
            {"start", start },
            {"length", len },
            {"data", binary }
          };
          requestManager->send(jChunk.dump());

          start += len;
        }

        // 4) send the done message
        nlohmann::ordered_json jDone = 
        {
          {"type", "response"},
          {"action", "capture.image.done"},
          {"snapshotIdx", req["snapshotIdx"].get<int>()}
        };
        requestManager->send(jDone.dump());
      }
      else if (req.contains("type") && req["type"] == "request" && 
               req.contains("action") && req["action"] == "update.view") {
        // int snapshotIdx = req["snapshotIdx"].get<int>();

        if (req.contains("camera")) {
          CameraState state;
          from_json(req["camera"], state);
          context->arcballCamera->setState(state);
        }
      }
      else if (req.contains("type") && req["type"] == "request" && 
               req.contains("action") && req["action"] == "update.transition") {
        // int snapshotIdx = req["snapshotIdx"].get<int>();

        if (req.contains("amount") && req.contains("from") && req.contains("to")) { 
          CameraState from;
          from_json(req["from"], from);

          CameraState to;
          from_json(req["to"], to);

          CameraState curr = from.slerp(to, req["amount"]);
          context->arcballCamera->setState(curr);
        }
      }

      requests.pop();
    }
  }
  else if (key == "start") {
    requestManager->start();
  }

}

}  // namespace storyboard_plugin
}  // namespace ospray
