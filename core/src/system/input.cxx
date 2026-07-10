#include <titan/core.hxx>
#include <titan/log.hxx>
#include <titan/utils.hxx>
#include <titan/system/input.hxx>

#include <cstring>

titan::InputSystem::InputSystem(Application &application)
    : m_Application(application),
      m_Instance(application.GetXrInstance()),
      m_Session(application.GetXrSession())
{
}

toolkit::result<> titan::InputSystem::Destroy()
{
    return {};
}

titan::HeadState titan::InputSystem::GetHead()
{
    return {
        .Orientation = m_Head.Pose.Orientation,
        .Position = m_Head.Pose.Position,
    };
}

titan::HandState titan::InputSystem::GetHand(const size_t index)
{
    auto &hand = m_Hands[index];

    return {
        .IsActive = static_cast<bool>(hand.PoseState.isActive),
        .Orientation = hand.Pose.Orientation,
        .Position = hand.Pose.Position,
        .Haptic = hand.Haptic,
    };
}

toolkit::result<> titan::InputSystem::Initialize()
{
    HANDLE(CreateActionSet());
    HANDLE(CreateActions());

    HANDLE(SuggestBindings());

    HANDLE(AttachActionSet());

    HANDLE(xr::StringToPath(m_Instance, "/user/hand/left") >> m_Hands[0].Path);
    HANDLE(xr::StringToPath(m_Instance, "/user/hand/right") >> m_Hands[1].Path);

    HANDLE(
        xr::CreateActionSpace(
            m_Instance,
            m_Session,
            m_ActionPalmPose,
            "/user/hand/left"
        ) >> m_Hands[0].Space);

    HANDLE(
        xr::CreateActionSpace(
            m_Instance,
            m_Session,
            m_ActionPalmPose,
            "/user/hand/right"
        ) >> m_Hands[1].Space);

    return {};
}

toolkit::result<> titan::InputSystem::Update(const detail::InputUpdateInfo &update_info)
{
    {
        XrSpaceLocation location;
        HANDLE(
            xr::LocateSpace(
                update_info.ViewSpace,
                update_info.ReferenceSpace,
                update_info.Time
            ) >> location);

        const glm::quat orientation
        {
            location.pose.orientation.w,
            location.pose.orientation.x,
            location.pose.orientation.y,
            location.pose.orientation.z,
        };

        const glm::vec3 position
        {
            location.pose.position.x,
            location.pose.position.y,
            location.pose.position.z,
        };

        m_Head.Pose = {
            .Orientation = glm::slerp(m_Head.Pose.Orientation, orientation, 0.1f),
            .Position = glm::mix(m_Head.Pose.Position, position, 0.1f),
        };
    }

    if (update_info.SessionState != XR_SESSION_STATE_FOCUSED)
        return {};

    const std::array active_action_sets
    {
        XrActiveActionSet
        {
            .actionSet = m_ActionSet,
        },
    };

    const XrActionsSyncInfo sync_info
    {
        .type = XR_TYPE_ACTIONS_SYNC_INFO,
        .countActiveActionSets = static_cast<uint32_t>(active_action_sets.size()),
        .activeActionSets = active_action_sets.data(),
    };

    HANDLE(xr::SyncActions(m_Session, sync_info));

    for (auto &hand : m_Hands)
    {
        const XrActionStateGetInfo get_info
        {
            .type = XR_TYPE_ACTION_STATE_GET_INFO,
            .action = m_ActionPalmPose,
            .subactionPath = hand.Path,
        };

        HANDLE(xr::GetActionStatePose(m_Session, get_info) >> hand.PoseState);

        if (!hand.PoseState.isActive)
            continue;

        XrSpaceLocation location{ .type = XR_TYPE_SPACE_LOCATION };
        if (auto res = xr::LocateSpace(hand.Space, update_info.ReferenceSpace, update_info.Time) >> location; !res)
        {
            info("{}", res.error());
            hand.PoseState.isActive = false;
            continue;
        }

        if (!(location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT))
        {
            hand.PoseState.isActive = false;
            continue;
        }

        if (!(location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))
        {
            hand.PoseState.isActive = false;
            continue;
        }

        const glm::quat orientation
        {
            location.pose.orientation.w,
            location.pose.orientation.x,
            location.pose.orientation.y,
            location.pose.orientation.z,
        };

        const glm::vec3 position
        {
            location.pose.position.x,
            location.pose.position.y,
            location.pose.position.z,
        };

        hand.Pose = {
            .Orientation = orientation,
            .Position = position,
        };
    }

    for (auto &hand : m_Hands)
    {
        const XrActionStateGetInfo get_info
        {
            .type = XR_TYPE_ACTION_STATE_GET_INFO,
            .action = m_ActionGrab,
            .subactionPath = hand.Path,
        };

        HANDLE(xr::GetActionStateFloat(m_Session, get_info) >> hand.GrabState);
    }

    for (auto &hand : m_Hands)
    {
        hand.Haptic *= 0.5f;
        if (hand.Haptic < 0.01f)
            hand.Haptic = 0.0f;

        const XrHapticActionInfo action_info
        {
            .type = XR_TYPE_HAPTIC_ACTION_INFO,
            .action = m_ActionHaptic,
            .subactionPath = hand.Path,
        };

        const XrHapticVibration haptic_vibration
        {
            .type = XR_TYPE_HAPTIC_VIBRATION,
            .duration = XR_MIN_HAPTIC_DURATION,
            .frequency = XR_FREQUENCY_UNSPECIFIED,
            .amplitude = hand.Haptic,
        };

        auto &haptic_base = reinterpret_cast<const XrHapticBaseHeader &>(haptic_vibration);

        HANDLE(xr::ApplyHapticFeedback(m_Session, action_info, haptic_base));
    }

    return {};
}

toolkit::result<> titan::InputSystem::CreateActionSet()
{
    const XrActionSetCreateInfo create_info
    {
        .type = XR_TYPE_ACTION_SET_CREATE_INFO,
        .actionSetName = "titan-core-action-set",
        .localizedActionSetName = "Titan Core Action Set",
        .priority = 0,
    };

    return xr::ActionSet::create(m_Instance, create_info) >> m_ActionSet;
}

toolkit::result<> titan::InputSystem::CreateActions()
{
    HANDLE(
        CreateAction(
            "grab",
            "Grab",
            XR_ACTION_TYPE_FLOAT_INPUT,
            { "/user/hand/left", "/user/hand/right" }
        ) >> m_ActionGrab);
    HANDLE(
        CreateAction(
            "palm-pose",
            "Palm Pose",
            XR_ACTION_TYPE_POSE_INPUT,
            { "/user/hand/left", "/user/hand/right" }
        ) >> m_ActionPalmPose);
    HANDLE(
        CreateAction(
            "haptic",
            "Haptic",
            XR_ACTION_TYPE_VIBRATION_OUTPUT,
            { "/user/hand/left", "/user/hand/right" }
        ) >> m_ActionHaptic);

    return {};
}

toolkit::result<titan::xr::Action> titan::InputSystem::CreateAction(
    const std::string &name,
    const std::string &localized_name,
    XrActionType type,
    const std::vector<std::string> &sub_path_strings)
{
    std::vector<XrPath> sub_paths(sub_path_strings.size());
    for (uint32_t i = 0; i < sub_path_strings.size(); ++i)
        HANDLE(xr::StringToPath(m_Instance, sub_path_strings[i]) >> sub_paths[i]);

    XrActionCreateInfo create_info
    {
        .type = XR_TYPE_ACTION_CREATE_INFO,
        .actionType = type,
        .countSubactionPaths = static_cast<uint32_t>(sub_paths.size()),
        .subactionPaths = sub_paths.data(),
    };

    strncpy(
        create_info.actionName,
        name.data(),
        std::min<size_t>(name.size(), XR_MAX_ACTION_NAME_SIZE));
    strncpy(
        create_info.localizedActionName,
        localized_name.data(),
        std::min<size_t>(name.size(), XR_MAX_LOCALIZED_ACTION_NAME_SIZE));

    return xr::Action::create(m_ActionSet, create_info);
}

toolkit::result<> titan::InputSystem::SuggestBindings()
{
    HANDLE(
        xr::SuggestInteractionProfileBindings(
            m_Instance,
            "/interaction_profiles/oculus/touch_controller",
            {
            { m_ActionGrab, "/user/hand/left/input/squeeze/value" },
            { m_ActionGrab, "/user/hand/right/input/squeeze/value" },
            { m_ActionPalmPose, "/user/hand/left/input/grip/pose" },
            { m_ActionPalmPose, "/user/hand/right/input/grip/pose" },
            { m_ActionHaptic, "/user/hand/left/output/haptic" },
            { m_ActionHaptic, "/user/hand/right/output/haptic" },
            }));

    HANDLE(
        xr::SuggestInteractionProfileBindings(
            m_Instance,
            "/interaction_profiles/khr/simple_controller",
            {
            { m_ActionGrab, "/user/hand/left/input/select/click" },
            { m_ActionGrab, "/user/hand/right/input/select/click" },
            { m_ActionPalmPose, "/user/hand/left/input/grip/pose" },
            { m_ActionPalmPose, "/user/hand/right/input/grip/pose" },
            { m_ActionHaptic, "/user/hand/left/output/haptic" },
            { m_ActionHaptic, "/user/hand/right/output/haptic" },
            }));

    return {};
}

toolkit::result<> titan::InputSystem::RecordBindings() const
{
    XrInteractionProfileState state;
    HANDLE(xr::GetCurrentInteractionProfile(m_Session, m_Hands[0].Path) >> state);

    if (state.interactionProfile)
    {
        std::string str;
        HANDLE(xr::PathToString(m_Instance, state.interactionProfile) >> str);

        info("/user/hand/left => {}", str);
    }

    HANDLE(xr::GetCurrentInteractionProfile(m_Session, m_Hands[1].Path) >> state);

    if (state.interactionProfile)
    {
        std::string str;
        HANDLE(xr::PathToString(m_Instance, state.interactionProfile) >> str);

        info("/user/hand/right => {}", str);
    }

    return {};
}

toolkit::result<> titan::InputSystem::AttachActionSet() const
{
    const std::vector<XrActionSet> action_sets
    {
        m_ActionSet,
    };

    const XrSessionActionSetsAttachInfo attach_info
    {
        .type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO,
        .countActionSets = static_cast<uint32_t>(action_sets.size()),
        .actionSets = action_sets.data(),
    };

    if (auto res = xrAttachSessionActionSets(m_Session, &attach_info))
        return toolkit::make_error("xrAttachSessionActionSets => {}", res);

    return {};
}
