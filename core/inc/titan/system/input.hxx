#pragma once

#include <titan/api.hxx>
#include <titan/wrapper/xr.hxx>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace titan
{
    namespace detail
    {
        struct PoseData
        {
            glm::quat Orientation{ 1.0f, 0.0f, 0.0f, 0.0f };
            glm::vec3 Position{ 0.0f, 0.0f, 0.0f };
        };

        struct HeadInfo
        {
            PoseData Pose;
        };

        struct HandInfo
        {
            XrPath Path{};
            xr::ActionSpace Space;

            XrActionStateFloat GrabState{ XR_TYPE_ACTION_STATE_FLOAT };
            XrActionStatePose PoseState{ XR_TYPE_ACTION_STATE_POSE };

            float Haptic{};

            PoseData Pose;
        };

        struct InputInitializeInfo
        {
            const xr::Instance &Instance;
            const xr::Session &Session;

            const xr::Action &ActionPalmPose;
        };

        struct InputRecordBindingsInfo
        {
            const xr::Instance &Instance;
            const xr::Session &Session;
        };

        struct InputUpdateInfo
        {
            const xr::Session &Session;

            const xr::ReferenceSpace &ViewSpace;
            const xr::ReferenceSpace &ReferenceSpace;

            const xr::ActionSet &ActionSet;
            const xr::Action &ActionPalmPose;
            const xr::Action &ActionGrab;
            const xr::Action &ActionHaptic;

            XrSessionState SessionState;
            XrTime Time;
        };
    }

    struct HeadState
    {
        const glm::quat &Orientation;
        const glm::vec3 &Position;
    };

    struct HandState
    {
        bool IsActive;

        const glm::quat &Orientation;
        const glm::vec3 &Position;

        float &Haptic;
    };

    class InputSystem
    {
        friend class Application;

    public:
        HeadState GetHead();
        HandState GetHand(size_t index);

    protected:
        toolkit::result<> Initialize(const detail::InputInitializeInfo &info);
        toolkit::result<> RecordBindings(const detail::InputRecordBindingsInfo &bindings_info);
        toolkit::result<> Update(const detail::InputUpdateInfo &update_info);

    private:
        detail::HeadInfo m_Head;
        std::array<detail::HandInfo, 2> m_Hands;
    };
}
