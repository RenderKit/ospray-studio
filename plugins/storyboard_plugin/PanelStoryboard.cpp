#include "PanelStoryboard.h"

#include "app/widgets/GenerateImGuiWidgets.h"
#include "app/MainWindow.h"
#include "app/CameraStack.h"

#include "sg/Frame.h"
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
  requestManager.reset(new RequestManager(configFilePath));
}

void PanelStoryboard::processRequests() {
  if (!requestManager->isRunning())
    return;

  std::queue<nlohmann::ordered_json> requests;
  requestManager->pollRequests(requests);
  if (requests.size() <= 0) 
    return;

  std::cout << "Processing " << requests.size() << " request(s)..." << std::endl;
  
  MainWindow* pMW = reinterpret_cast<MainWindow*>(context->getMainWindow()); 

  while (!requests.empty()) {
    nlohmann::ordered_json req = requests.front();

    if (req.contains("type") && req["type"] == "request" && 
        req.contains("action") && req["action"] == "capture.view") {
      
      nlohmann::ordered_json j = 
      {
        {"type", "response"},
        {"action", "capture.view"},
        {"snapshotIdx", req["snapshotIdx"].get<int>()},
        {"camera", pMW->arcballCamera->getState()}
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
        
        // update the state
        pMW->arcballCamera->setState(state);
        context->updateCamera();
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
        float frac = req["amount"].get<float>();

        // interplate between two camera states
        CameraState cs{};
        cs.centerTranslation = lerp(frac, from.centerTranslation, to.centerTranslation);
        cs.translation = lerp(frac, from.translation, to.translation);
        if (from.rotation != to.rotation)
          cs.rotation = ospray::sg::slerp(from.rotation, to.rotation, frac);
        else
          cs.rotation = from.rotation;

        // update the state
        pMW->arcballCamera->setState(cs);
        context->updateCamera();
      }
    }

    requests.pop();
  }
}

void PanelStoryboard::buildUI(void *ImGuiCtx)
{
  processRequests();

  // Allows plugin to still do other work if the UI isn't shown.
  if (!isShown())
    return;

  // Need to set ImGuiContext in *this* address space
  ImGui::SetCurrentContext((ImGuiContext *)ImGuiCtx);
  ImGui::OpenPopup(panelName.c_str());

  if (ImGui::BeginPopupModal(
          panelName.c_str(), nullptr, ImGuiWindowFlags_None)) {
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
    
    if (ImGui::Button("Close")) {
      setShown(false);
      ImGui::CloseCurrentPopup();
    }
    ImGui::Separator();

    // Display statuses in a scrolling region
    if (ImGui::CollapsingHeader("Status", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::BeginChild("Scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
      for (std::string status : requestManager->statuses) {
        ImGui::Text("%s", status.c_str());
      }
      ImGui::EndChild();
    }

    ImGui::EndPopup();
  }
}

}  // namespace storyboard_plugin
}  // namespace ospray
