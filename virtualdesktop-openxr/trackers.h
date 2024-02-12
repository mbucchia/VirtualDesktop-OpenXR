// MIT License
//
// Copyright(c) 2022-2024 Matthieu Bucchianeri
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright noticeand this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

struct TrackerRoleMapping {
    const std::string role;
    const std::string localizedName;
    XrFullBodyJointMETA joint;
    XrPosef transform{xr::math::Pose::Identity()};
};
const TrackerRoleMapping TrackerRoles[] = {
#pragma warning(push)
#pragma warning(disable : 4305) // truncation from double to float
    // clang-format off
    {"chest", "Chest", XR_FULL_BODY_JOINT_CHEST_META, xr::math::Pose::Orientation({M_PI_2, 0, 0})},
    {"waist", "Waist", XR_FULL_BODY_JOINT_HIPS_META, xr::math::Pose::Orientation({M_PI_2, 0, 0})},
    {"left_shoulder", "Left Shoulder", XR_FULL_BODY_JOINT_LEFT_SCAPULA_META, xr::math::Pose::Orientation({0, M_PI, 0})},
    {"right_shoulder", "Right Shoulder", XR_FULL_BODY_JOINT_RIGHT_SCAPULA_META},
    {"left_elbow", "Left Elbow", XR_FULL_BODY_JOINT_LEFT_ARM_LOWER_META, xr::math::Pose::Orientation({0, M_PI, 0})},
    {"right_elbow", "Right Elbow", XR_FULL_BODY_JOINT_RIGHT_ARM_LOWER_META},
    {"left_wrist", "Left Wrist", XR_FULL_BODY_JOINT_LEFT_HAND_WRIST_META, xr::math::Pose::Orientation({-M_PI_2, 0, 0})},
    {"right_wrist", "Right Wrist", XR_FULL_BODY_JOINT_RIGHT_HAND_WRIST_META, xr::math::Pose::Orientation({M_PI_2,0,0})},
    {"left_knee", "Left Knee", XR_FULL_BODY_JOINT_LEFT_LOWER_LEG_META, xr::math::Pose::Orientation({-M_PI, 0, 0})},
    {"right_knee", "Right Knee", XR_FULL_BODY_JOINT_RIGHT_LOWER_LEG_META, xr::math::Pose::Orientation({M_PI, 0, 0})},
    {"left_ankle", "Left Ankle", XR_FULL_BODY_JOINT_LEFT_FOOT_ANKLE_META, xr::math::Pose::Orientation({0, -M_PI, 0})},
    {"right_ankle", "Right Ankle", XR_FULL_BODY_JOINT_RIGHT_FOOT_ANKLE_META, xr::math::Pose::Orientation({0, M_PI, 0})},
    {"left_foot", "Left Foot", XR_FULL_BODY_JOINT_LEFT_FOOT_TRANSVERSE_META, xr::math::Pose::Orientation({M_PI, -M_PI, 0})},
    {"right_foot", "Right Foot", XR_FULL_BODY_JOINT_RIGHT_FOOT_TRANSVERSE_META, xr::math::Pose::Orientation({-M_PI, M_PI, 0})},
// clang-format on
#pragma warning(pop)
};
