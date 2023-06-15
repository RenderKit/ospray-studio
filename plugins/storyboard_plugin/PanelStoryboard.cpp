#include <ctime>

#include "PanelStoryboard.h"

#include "app/widgets/GenerateImGuiWidgets.h"

#include "rkcommon/math/vec.h"

#include "sg/JSONDefs.h"
#include "sg/Math.h"
#include "sg/JSONDefs.h"
#include "sg/fb/FrameBuffer.h"

#include "imgui.h"
#include "stb_image_write.h"

namespace ospray {
namespace storyboard_plugin {

static void WriteToMemory_stbi(void *context, void *data, int size) {
  std::vector<unsigned char> *buffer =
      reinterpret_cast<std::vector<unsigned char> *>(context);

  unsigned char *pData = reinterpret_cast<unsigned char *>(data);

  buffer->insert(buffer->end(), pData, pData + size);
}

PanelStoryboard::PanelStoryboard(std::shared_ptr<StudioContext> _context, std::string _panelName, std::string _configFilePath)
    : Panel(_panelName.c_str(), _context)
    , panelName(_panelName)
    , configFilePath(_configFilePath)
{
  requestManager.reset(new RequestManager("localhost", 8888));

  requestManager->start();

  context->frame->child("scale") = 0.5f;
  context->frame->child("scaleNav") = 0.25f;
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
        auto &fb = context->frame->childAs<sg::FrameBuffer>("framebuffer");
        auto img = fb.map(OSP_FB_COLOR);
        auto size = fb.child("size").valueAs<vec2i>(); // 2048 x 1280
        auto fmt = fb.child("colorFormat").valueAs<std::string>(); // sRGB

        std::vector<unsigned char> values;
        stbi_flip_vertically_on_write(1);
        stbi_write_png_to_func(WriteToMemory_stbi, &values, size.x, size.y, 4, img, 4 * size.x);
        requestManager->screenshotContainer->clear();
        requestManager->screenshotContainer->setImage(size.x, size.y, values);

        // 2) send the setup information
        nlohmann::ordered_json jSetUp = 
        {
          {"type", "response"},
          {"action", "capture.image.setup"},
          {"snapshotIdx", req["snapshotIdx"].get<int>()},
          {"width", size.x},
          {"height", size.y},
          {"length", values.size()}
        };
        requestManager->send(jSetUp.dump());
      }
      else if (req.contains("type") && req["type"] == "ack" && 
          req.contains("action") && (req["action"] == "capture.image.setup" || req["action"] == "capture.image.data")) {
        
        if (!requestManager->screenshotContainer->isNextChunkAvailable()) {
          // clear the memory
          requestManager->screenshotContainer->clear();
          // send the message to indicate the sending the image part is done
          nlohmann::ordered_json jChunk = 
          {
            {"type", "response"},
            {"action", "capture.image.done"},
            {"snapshotIdx", req["snapshotIdx"].get<int>()}
          };
          requestManager->send(jChunk.dump());
        }
        else { // send the first message
          int len = 512;
          int start = requestManager->screenshotContainer->getStartIndex();
          std::vector<unsigned char> chunk = requestManager->screenshotContainer->getNextChunk(len);

          auto binary = nlohmann::ordered_json::binary_t(chunk);
          nlohmann::ordered_json jChunk = 
          {
            {"type", "response"},
            {"action", "capture.image.data"},
            {"snapshotIdx", req["snapshotIdx"].get<int>()},
            {"start", start },
            {"data", binary }
          };
          requestManager->send(jChunk.dump());
        }
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
