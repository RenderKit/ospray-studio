#pragma once

#include "TrackingData.h"
#include "tcpsocket.hpp"

#include <string>
#include <mutex>
#include <list>

namespace ospray {
namespace gesture_plugin {

using namespace rkcommon::math;

class TrackingManager
{
public:
    TrackingManager(std::string configFilePath);
    ~TrackingManager();

    void saveConfig(std::string configFilePath);

    void start();
    void close();
    bool isRunning();
    bool isUpdated();

    TrackingState pollState();
    std::string getResultsInReadableForm();

    std::string ipAddress { "localhost" };
    uint portNumber { 8888 };
    // Kinect - right-hand, y-down, z-forward, in milli-meters
    // OSPRay - right-hand, y-up, z-forward, in meters
    vec3f scaleOffset { -0.001f, -0.001f, +0.001f};
    // vec3f rotationOffset { 0.0f, 0.0f, 0.0f};
    vec3f translationOffset { 0.0f, 0.0f, 0.0f};
    int confidenceLevelThreshold { K4ABT_JOINT_CONFIDENCE_LOW };
    float leaningAngleThreshold { 8.0f }; // in degrees
    vec3f leaningDirScaleFactor { 1.0f, 1.0f, 1.0f };

    std::list<std::string> statuses;
private:
    void updateState(std::string message);
    void addStatus(std::string status);

    TCPSocket<> *tcpSocket;
    TrackingState state;
    bool updated;

    std::mutex mtx;
};

}  // namespace gesture_plugin
}  // namespace ospray