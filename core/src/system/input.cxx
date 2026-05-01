#include <titan/log.hxx>
#include <titan/utils.hxx>
#include <titan/system/input.hxx>

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

toolkit::result<> titan::InputSystem::Initialize(const detail::InputInitializeInfo &info)
{
    auto res = toolkit::result()
               & [&]
               {
                   return xr::StringToPath(info.Instance, "/user/hand/left") >> m_Hands[0].Path;
               }
               & [&]
               {
                   return xr::StringToPath(info.Instance, "/user/hand/right") >> m_Hands[1].Path;
               } & [&]
               {
                   return xr::CreateActionSpace(
                              info.Instance,
                              info.Session,
                              info.ActionPalmPose,
                              "/user/hand/left"
                          ) >> m_Hands[0].Space;
               }
               & [&]
               {
                   return xr::CreateActionSpace(
                              info.Instance,
                              info.Session,
                              info.ActionPalmPose,
                              "/user/hand/right"
                          ) >> m_Hands[1].Space;
               };

    return {};
}

toolkit::result<> titan::InputSystem::RecordBindings(const detail::InputRecordBindingsInfo &bindings_info)
{
    return toolkit::result()
           & [&]
           {
               return xr::GetCurrentInteractionProfile(bindings_info.Session, m_Hands[0].Path);
           }
           & [&](XrInteractionProfileState &&state) -> toolkit::result<XrInteractionProfileState>
           {
               if (state.interactionProfile)
               {
                   std::string str;
                   if (auto res = xr::PathToString(bindings_info.Instance, state.interactionProfile) >> str; !res)
                       return res;
                   info("/user/hand/left => {}", str);
               }

               return xr::GetCurrentInteractionProfile(bindings_info.Session, m_Hands[1].Path);
           }
           & [&](XrInteractionProfileState &&state)
           {
               if (state.interactionProfile)
               {
                   std::string str;
                   if (auto res = xr::PathToString(bindings_info.Instance, state.interactionProfile) >> str; !res)
                       return res;
                   info("/user/hand/right => {}", str);
               }

               return toolkit::result();
           };
}

toolkit::result<> titan::InputSystem::Update(const detail::InputUpdateInfo &update_info)
{
    {
        XrSpaceLocation view
        {
            .type = XR_TYPE_SPACE_LOCATION,
        };

        if (auto res = xrLocateSpace(update_info.ViewSpace, update_info.ReferenceSpace, update_info.Time, &view))
            return toolkit::make_error("xrLocateSpace => {}", res);

        const glm::quat orientation
        {
            view.pose.orientation.w,
            view.pose.orientation.x,
            view.pose.orientation.y,
            view.pose.orientation.z,
        };

        const glm::vec3 position
        {
            view.pose.position.x,
            view.pose.position.y,
            view.pose.position.z,
        };

        m_Head.Pose = {
            .Orientation = glm::slerp(m_Head.Pose.Orientation, orientation, 0.1f),
            .Position = glm::mix(m_Head.Pose.Position, position, 0.1f),
        };
    }

    const std::array active_action_sets
    {
        XrActiveActionSet
        {
            .actionSet = update_info.ActionSet,
        },
    };

    const XrActionsSyncInfo sync_info
    {
        .type = XR_TYPE_ACTIONS_SYNC_INFO,
        .countActiveActionSets = static_cast<uint32_t>(active_action_sets.size()),
        .activeActionSets = active_action_sets.data(),
    };

    if (update_info.SessionState != XR_SESSION_STATE_FOCUSED)
        return {};

    if (auto res = xrSyncActions(update_info.Session, &sync_info))
        return toolkit::make_error("xrSyncActions => {}", res);

    for (auto &hand : m_Hands)
    {
        const XrActionStateGetInfo get_info
        {
            .type = XR_TYPE_ACTION_STATE_GET_INFO,
            .action = update_info.ActionPalmPose,
            .subactionPath = hand.Path,
        };

        if (auto res = xrGetActionStatePose(update_info.Session, &get_info, &hand.PoseState))
            return toolkit::make_error("xrGetActionStatePose => {}", res);

        if (!hand.PoseState.isActive)
            continue;

        XrSpaceLocation location{ .type = XR_TYPE_SPACE_LOCATION };
        if (auto res = xrLocateSpace(hand.Space, update_info.ReferenceSpace, update_info.Time, &location))
        {
            info("xrLocateSpace => {}", res);
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
            .action = update_info.ActionGrab,
            .subactionPath = hand.Path,
        };

        if (auto res = xrGetActionStateFloat(update_info.Session, &get_info, &hand.GrabState))
            return toolkit::make_error("xrGetActionStateFloat => {}", res);
    }

    for (auto &hand : m_Hands)
    {
        hand.Haptic *= 0.5f;
        if (hand.Haptic < 0.01f)
            hand.Haptic = 0.0f;

        const XrHapticActionInfo action_info
        {
            .type = XR_TYPE_HAPTIC_ACTION_INFO,
            .action = update_info.ActionHaptic,
            .subactionPath = hand.Path,
        };

        const XrHapticVibration haptic_vibration
        {
            .type = XR_TYPE_HAPTIC_VIBRATION,
            .duration = XR_MIN_HAPTIC_DURATION,
            .frequency = XR_FREQUENCY_UNSPECIFIED,
            .amplitude = hand.Haptic,
        };

        if (auto res = xrApplyHapticFeedback(
            update_info.Session,
            &action_info,
            reinterpret_cast<const XrHapticBaseHeader *>(&haptic_vibration)))
            return toolkit::make_error("xrApplyHapticFeedback => {}", res);
    }

    return {};
}
