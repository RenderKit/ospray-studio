#include "TrackingManager.h"
#include "sg/JSONDefs.h"

#include <iostream>

namespace ospray {
namespace gesture_plugin {

TrackingManager::TrackingManager(std::string configFilePath) {
    tcpSocket = nullptr;
    updated = false;

    JSON config = nullptr;
    try {
        std::ifstream configFile(configFilePath);
        if (configFile)
            configFile >> config;
        else
            std::cerr << "The gesture config file does not exist." << std::endl;
    } catch (nlohmann::json::exception &e) {
        std::cerr << "Failed to parse the gesture config file: " << e.what() << std::endl;
    }

    if (config == nullptr)
        return;

    if (config != nullptr && config.contains("ipAddress"))
        ipAddress = config["ipAddress"];
    if (config != nullptr && config.contains("portNumber"))
        portNumber = config["portNumber"];
    if (config != nullptr && config.contains("scaleOffset"))
        scaleOffset = config["scaleOffset"].get<vec3f>();
    // if (config != nullptr && config.contains("rotationOffset"))
    //     rotationOffset = config["rotationOffset"].get<vec3f>();
    if (config != nullptr && config.contains("translationOffset"))
        translationOffset = config["translationOffset"].get<vec3f>();
    if (config != nullptr && config.contains("confidenceLevelThreshold"))
        confidenceLevelThreshold = config["confidenceLevelThreshold"].get<int>();
    if (config != nullptr && config.contains("leaningAngleThreshold"))
        leaningAngleThreshold = config["leaningAngleThreshold"];
    if (config != nullptr && config.contains("leaningDirScaleFactor"))
        leaningDirScaleFactor = config["leaningDirScaleFactor"].get<vec3f>();
}

TrackingManager::~TrackingManager() {
    this->close();
}

void TrackingManager::saveConfig(std::string configFilePath) {
    std::ofstream config(configFilePath);

    JSON j;
    j["ipAddress"] = ipAddress;
    j["portNumber"] = portNumber;
    j["scaleOffset"] = scaleOffset;
    // j["rotationOffset"] = rotationOffset;
    j["translationOffset"] = translationOffset;
    j["confidenceLevelThreshold"] = confidenceLevelThreshold;
    j["leaningAngleThreshold"] = leaningAngleThreshold;
    j["leaningDirScaleFactor"] = leaningDirScaleFactor;

    config << std::setw(4) << j << std::endl;
    addStatus("Saved the configuration to " + configFilePath);
} 

void TrackingManager::start() {
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
        std::lock_guard<std::mutex> guard(mtx);

        // std::cout << "Message from the Server: " << message << std::endl << std::flush;
        updateState(message);
        updated = true;
    };
    
    // On socket closed:
    tcpSocket->onSocketClosed = [&](int errorCode){
        addStatus("Connection closed: " + std::to_string(errorCode));
        delete tcpSocket;
        tcpSocket = nullptr;
        updateState("{}");
        updated = true;
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

void TrackingManager::close() {
    if (tcpSocket == nullptr)
        return;

    tcpSocket->Close();
}

bool TrackingManager::isRunning() {
    return tcpSocket != nullptr;
}

bool TrackingManager::isUpdated() {
    return updated;
}

TrackingState TrackingManager::pollState() {
    std::lock_guard<std::mutex> guard(mtx);
    
    updated = false;
    return state;
}

void TrackingManager::updateState(std::string message) {
    // set the default state
    for (int i = 0; i < K4ABT_JOINT_COUNT; i++) {
        state.positions[i] = vec3f(0.f);
        state.confidences[i] = K4ABT_JOINT_CONFIDENCE_NONE;
    }
    state.mode = INTERACTION_NONE;
    state.leaningAngle = 0.0f;
    state.leaningDir = vec3f(0.0f);

    // parse the message (which is supposed to be in a JSON format)
    nlohmann::ordered_json j; 
    try {
        j = nlohmann::ordered_json::parse(message);
    } catch (nlohmann::json::exception& e) {
        std::cout << "Parse exception: " << e.what() << std::endl;
        j = nullptr;
    }

    // check if the tracking data is reliable.
    if (j == nullptr || j.size() != K4ABT_JOINT_COUNT) {
        return;
    }

    // update positions and confidence levels
    for (int i = 0; i < K4ABT_JOINT_COUNT; i++) {
        if (j[i].contains("pos")) {
            state.positions[i] = j[i]["pos"].get<vec3f>() * scaleOffset + translationOffset;
        }
        if (j[i].contains("conf")) {
            state.confidences[i] = j[i]["conf"]; 
        }
    }

    // check if the tracking data is reliable for further detections.
    if (state.confidences[K4ABT_JOINT_SPINE_NAVEL] < confidenceLevelThreshold ||
        state.confidences[K4ABT_JOINT_WRIST_LEFT] < confidenceLevelThreshold ||
        state.confidences[K4ABT_JOINT_WRIST_RIGHT] < confidenceLevelThreshold ||
        state.confidences[K4ABT_JOINT_NECK] < confidenceLevelThreshold) {
        return;
    }

    // compute additional states (angle and direction)
    vec3f spine = state.positions[K4ABT_JOINT_NECK] - state.positions[K4ABT_JOINT_SPINE_NAVEL];
    vec3f normal (0.0f, 1.0f, 0.0f);
    state.leaningAngle = acos(dot(spine, normal) / (length(spine) * length(normal))) / M_PI * 180.f;
    state.leaningDir = spine - dot(spine, normal) / length(normal) * normal;
    state.leaningDir *= leaningDirScaleFactor;

    // compute additional states (mode)
    bool leftHandUp = state.positions[K4ABT_JOINT_SPINE_NAVEL].y < state.positions[K4ABT_JOINT_WRIST_LEFT].y;
    bool rightHandUp = state.positions[K4ABT_JOINT_SPINE_NAVEL].y < state.positions[K4ABT_JOINT_WRIST_RIGHT].y;
    state.mode = (leftHandUp && rightHandUp && state.leaningAngle > leaningAngleThreshold) ? INTERACTION_FLYING : INTERACTION_IDLE;
}

void TrackingManager::addStatus(std::string status) {
  // write time before status
  time_t now = time(0);
  tm *ltm = localtime(&now);
  status.insert(0, "(" + std::to_string(ltm->tm_hour) + ":" + std::to_string(ltm->tm_min) + ":" + std::to_string(ltm->tm_sec) + ") ");

  statuses.push_back(status);
}

// show "important" tracking information in a readable format
std::string TrackingManager::getResultsInReadableForm() {
    std::string result;

    result += "headPos.x: " + std::to_string(state.positions[K4ABT_JOINT_HEAD].x) + "\n";
    result += "headPos.y: " + std::to_string(state.positions[K4ABT_JOINT_HEAD].y) + "\n";
    result += "headPos.z: " + std::to_string(state.positions[K4ABT_JOINT_HEAD].z) + "\n";

    if (state.mode == INTERACTION_NONE) result += "mode: NONE\n";
    else if (state.mode == INTERACTION_IDLE) result += "mode: IDLE\n";
    else if (state.mode == INTERACTION_FLYING) result += "mode: FLYING\n";

    result += "leaningAngle: " + std::to_string(state.leaningAngle) + " °\n";

    result += "leaningDir.x: " + std::to_string(state.leaningDir.x) + "\n";
    result += "leaningDir.y: " + std::to_string(state.leaningDir.y) + " (proj. to x-z plane)\n";
    result += "leaningDir.z: " + std::to_string(state.leaningDir.z);
    
    return result;
}

}  // namespace gesture_plugin
}  // namespace ospray