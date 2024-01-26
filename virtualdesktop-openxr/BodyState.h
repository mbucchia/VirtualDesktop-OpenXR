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

namespace virtualdesktop_openxr {

    namespace BodyTracking {

        // See type definitions from Virtual Desktop.
        // https://github.com/guygodin/VirtualDesktop.VRCFaceTracking

        struct Vector3 {
            float x, y, z;
        };

        struct Quaternion {
            float x, y, z, w;
        };

        struct Pose {
            Quaternion orientation;
            Vector3 position;
        };

        struct HandTrackingAimState {
            uint64_t AimStatus;
            Pose AimPose;
            float PinchStrengthIndex;
            float PinchStrengthMiddle;
            float PinchStrengthRing;
            float PinchStrengthLittle;
        };

        struct FingerJointState {
            Pose Pose;
            float Radius;
            Vector3 AngularVelocity;
            Vector3 LinearVelocity;
        };

        struct SkeletonJoint {
            int32_t Joint;
            int32_t ParentJoint;
            Pose Pose;
        };

        static constexpr int ExpressionCount = 70;
        static_assert(ExpressionCount == XR_FACE_EXPRESSION2_COUNT_FB);
        static constexpr int ConfidenceCount = 2;
        static_assert(ConfidenceCount == XR_FACE_CONFIDENCE_COUNT_FB);
        static constexpr int HandJointCount = 26;
        static_assert(HandJointCount == XR_HAND_JOINT_COUNT_EXT);

        struct BodyStateV2 {
            uint8_t FaceIsValid;
            uint8_t IsEyeFollowingBlendshapesValid;
            float ExpressionWeights[ExpressionCount];
            float ExpressionConfidences[ConfidenceCount];

            uint8_t LeftEyeIsValid;
            uint8_t RightEyeIsValid;
            Pose LeftEyePose;
            Pose RightEyePose;
            float LeftEyeConfidence;
            float RightEyeConfidence;

            uint8_t LeftHandActive;
            uint8_t RightHandActive;
            FingerJointState LeftHandJointStates[HandJointCount];
            FingerJointState RightHandJointStates[HandJointCount];

            HandTrackingAimState LeftAimState;
            HandTrackingAimState RightAimState;

            // Body Joints exposed but we do not use them.
        };

    } // namespace BodyTracking

} // namespace virtualdesktop_openxr
