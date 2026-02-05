#pragma once

#include "rkcommon/math/vec.h"

namespace ospray {
namespace gesture_plugin {

/** Model fitting joint definition (copied from Kinect SDK)
 */
typedef enum
{
    K4ABT_JOINT_PELVIS = 0,
    K4ABT_JOINT_SPINE_NAVEL,
    K4ABT_JOINT_SPINE_CHEST,
    K4ABT_JOINT_NECK,
    K4ABT_JOINT_CLAVICLE_LEFT,
    K4ABT_JOINT_SHOULDER_LEFT,
    K4ABT_JOINT_ELBOW_LEFT,
    K4ABT_JOINT_WRIST_LEFT,
    K4ABT_JOINT_HAND_LEFT,
    K4ABT_JOINT_HANDTIP_LEFT,
    K4ABT_JOINT_THUMB_LEFT,
    K4ABT_JOINT_CLAVICLE_RIGHT,
    K4ABT_JOINT_SHOULDER_RIGHT,
    K4ABT_JOINT_ELBOW_RIGHT,
    K4ABT_JOINT_WRIST_RIGHT,
    K4ABT_JOINT_HAND_RIGHT,
    K4ABT_JOINT_HANDTIP_RIGHT,
    K4ABT_JOINT_THUMB_RIGHT,
    K4ABT_JOINT_HIP_LEFT,
    K4ABT_JOINT_KNEE_LEFT,
    K4ABT_JOINT_ANKLE_LEFT,
    K4ABT_JOINT_FOOT_LEFT,
    K4ABT_JOINT_HIP_RIGHT,
    K4ABT_JOINT_KNEE_RIGHT,
    K4ABT_JOINT_ANKLE_RIGHT,
    K4ABT_JOINT_FOOT_RIGHT,
    K4ABT_JOINT_HEAD,
    K4ABT_JOINT_NOSE,
    K4ABT_JOINT_EYE_LEFT,
    K4ABT_JOINT_EAR_LEFT,
    K4ABT_JOINT_EYE_RIGHT,
    K4ABT_JOINT_EAR_RIGHT,
    K4ABT_JOINT_COUNT
} k4abt_joint_id_t;

const std::string k4abt_joint_id_t_str[] = 
{
    "PELVIS",
    "SPINE_NAVEL",
    "SPINE_CHEST",
    "NECK",
    "CLAVICLE_LEFT",
    "SHOULDER_LEFT",
    "ELBOW_LEFT",
    "WRIST_LEFT",
    "HAND_LEFT",
    "HANDTIP_LEFT",
    "THUMB_LEFT",
    "CLAVICLE_RIGHT",
    "SHOULDER_RIGHT",
    "ELBOW_RIGHT",
    "WRIST_RIGHT",
    "HAND_RIGHT",
    "HANDTIP_RIGHT",
    "THUMB_RIGHT",
    "HIP_LEFT",
    "KNEE_LEFT",
    "ANKLE_LEFT",
    "FOOT_LEFT",
    "HIP_RIGHT",
    "KNEE_RIGHT",
    "ANKLE_RIGHT",
    "FOOT_RIGHT",
    "HEAD",
    "NOSE",
    "EYE_LEFT",
    "EAR_LEFT",
    "EYE_RIGHT",
    "EAR_RIGHT"
};

/** k4abt_joint_confidence_level_t  (copied from Kinect SDK)
 *
 * \remarks
 * This enumeration specifies the joint confidence level.
*/
typedef enum
{
    K4ABT_JOINT_CONFIDENCE_NONE = 0,          /**< The joint is out of range (too far from depth camera) */
    K4ABT_JOINT_CONFIDENCE_LOW = 1,           /**< The joint is not observed (likely due to occlusion), predicted joint pose */
    K4ABT_JOINT_CONFIDENCE_MEDIUM = 2,        /**< Medium confidence in joint pose. Current SDK will only provide joints up to this confidence level */
    K4ABT_JOINT_CONFIDENCE_HIGH = 3,          /**< High confidence in joint pose. Placeholder for future SDK */
    K4ABT_JOINT_CONFIDENCE_LEVELS_COUNT = 4,  /**< The total number of confidence levels. */
} k4abt_joint_confidence_level_t;

typedef enum
{
    INTERACTION_NONE = 0,
    INTERACTION_IDLE = 1,
    INTERACTION_FLYING = 2,
} InteractionMode;

struct TrackingState 
{
    rkcommon::math::vec3f positions[K4ABT_JOINT_COUNT];
    int confidences[K4ABT_JOINT_COUNT];

    InteractionMode mode;
    float leaningAngle; // in degrees
    rkcommon::math::vec3f leaningDir; // against the global y-axis (0, 1, 0)

    TrackingState() {
        for (int i = 0; i < K4ABT_JOINT_COUNT; i++) {
            positions[i] = rkcommon::math::vec3f(0.);
            confidences[i] = K4ABT_JOINT_CONFIDENCE_NONE;
        }
    }
};

}  // namespace gesture_plugin
}  // namespace ospray
