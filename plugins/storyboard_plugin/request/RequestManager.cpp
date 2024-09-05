#include "RequestManager.h"
#include "sg/JSONDefs.h"

#include <iostream>

namespace ospray {
namespace storyboard_plugin {

RequestManager::RequestManager(std::string configFilePath) {
    tcpSocket = nullptr;
    screenshotContainer.reset(new ScreenShotContainer());

    JSON config = nullptr;
    try {
        std::ifstream configFile(configFilePath);
        if (configFile)
            configFile >> config;
        else
            std::cerr << "The storyboard config file does not exist." << std::endl;
    } catch (nlohmann::json::exception &e) {
        std::cerr << "Failed to parse the storyboard config file: " << e.what() << std::endl;
    }

    if (config == nullptr)
        return;

    if (config != nullptr && config.contains("ipAddress"))
        ipAddress = config["ipAddress"];
    if (config != nullptr && config.contains("portNumber"))
        portNumber = config["portNumber"];
}

RequestManager::~RequestManager() {
    this->close();
}

void RequestManager::start() {
    if (tcpSocket != nullptr) {
        std::cout << "Connection has already been set." << std::endl;
        return;
    }

    // Initialize socket.
    tcpSocket = new TCPSocket<>([&](int errorCode, std::string errorMessage){
        addStatus("Socket creation error: " + std::to_string(errorCode) + " : " + errorMessage);
    });

    // Start receiving from the host.
    tcpSocket->onMessageReceived = [&](std::string message) {
        // std::cout << message << std::endl;

        std::lock_guard<std::mutex> guard(mtx);

        // parse the message (which is supposed to be in a JSON format)
        nlohmann::ordered_json j; 
        try {
            j = nlohmann::ordered_json::parse(message);
        } catch (nlohmann::json::exception& e) {
            std::cout << "Parse exception: " << e.what() << std::endl;
            j = nullptr;
        }

        // add it to the pending actions
        if (j != nullptr) {
             pendingRequests.push(j);
        }
    };
    
    // On socket closed:
    tcpSocket->onSocketClosed = [&](int errorCode){
        std::lock_guard<std::mutex> guard(mtx);
        while (!pendingRequests.empty()) pendingRequests.pop();

        addStatus("Connection closed: " + std::to_string(errorCode));
        delete tcpSocket;
        tcpSocket = nullptr;
    };

    // Connect to the host.
    tcpSocket->Connect(ipAddress, portNumber, [&] {
        addStatus("Connected to the server successfully."); 
    },
    [&](int errorCode, std::string errorMessage){ // Connection failed
        addStatus(std::to_string(errorCode) + " : " + errorMessage);
        delete tcpSocket;
        tcpSocket = nullptr;
    });
}

void RequestManager::close() {
    if (tcpSocket == nullptr) return;

    tcpSocket->Close();
}

bool RequestManager::isRunning() {
    return tcpSocket != nullptr;
}

bool RequestManager::isUpdated() {
    std::lock_guard<std::mutex> guard(mtx);

    return pendingRequests.size() > 0;
}

void RequestManager::pollRequests(std::queue<nlohmann::ordered_json>& requests) {
    std::lock_guard<std::mutex> guard(mtx);
    
    while (!pendingRequests.empty()) {
        requests.push(pendingRequests.front());
        pendingRequests.pop();
    }
}

void RequestManager::send(std::string message) {
    if (tcpSocket == nullptr) return;
    
    tcpSocket->Send(message);
}

void RequestManager::addStatus(std::string status) {
  // write time before status
  time_t now = time(0);
  tm *ltm = localtime(&now);
  status.insert(0, "(" + std::to_string(ltm->tm_hour) + ":" + std::to_string(ltm->tm_min) + ":" + std::to_string(ltm->tm_sec) + ") ");

  statuses.push_back(status);
}

// show "important" storyboard information in a readable format
std::string RequestManager::getResultsInReadableForm() {
    std::lock_guard<std::mutex> guard(mtx);

    return std::to_string(pendingRequests.size()) + " pending action(s)";
}

}  // namespace storyboard_plugin
}  // namespace ospray