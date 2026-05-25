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

            XrActionStateFloat GrabState{};
            XrActionStatePose PoseState{};

            float Haptic{};

            PoseData Pose;
        };

        struct InputUpdateInfo
        {
            const xr::ReferenceSpace &ViewSpace;
            const xr::ReferenceSpace &ReferenceSpace;

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
        InputSystem(xr::Instance &instance, xr::Session &session);

        HeadState GetHead();
        HandState GetHand(size_t index);

    protected:
        [[nodiscard]] toolkit::result<> Initialize();
        [[nodiscard]] toolkit::result<> Update(const detail::InputUpdateInfo &update_info);

        [[nodiscard]] toolkit::result<> CreateActionSet();
        [[nodiscard]] toolkit::result<> CreateActions();

        [[nodiscard]] toolkit::result<xr::Action> CreateAction(
            const std::string &name,
            const std::string &localized_name,
            XrActionType type,
            const std::vector<std::string> &sub_path_strings = {});

        [[nodiscard]] toolkit::result<> SuggestBindings();

        [[nodiscard]] toolkit::result<> RecordBindings() const;
        [[nodiscard]] toolkit::result<> AttachActionSet() const;

    private:
        xr::Instance &m_Instance;
        xr::Session &m_Session;

        xr::ActionSet m_ActionSet;
        xr::Action m_ActionGrab, m_ActionHaptic, m_ActionPalmPose;

        detail::HeadInfo m_Head;
        std::array<detail::HandInfo, 2> m_Hands;
    };
}
