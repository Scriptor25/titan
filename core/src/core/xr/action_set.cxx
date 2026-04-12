#include <titan/core.hxx>
#include <titan/utils.hxx>

#include <cstring>
#include <iostream>
#include <titan/log.hxx>

core::result<> core::Application::CreateActionSet()
{
    const XrActionSetCreateInfo create_info
    {
        .type = XR_TYPE_ACTION_SET_CREATE_INFO,
        .actionSetName = "titan-core-action-set",
        .localizedActionSetName = "Titan Core Action Set",
        .priority = 0,
    };

    return xr::ActionSet::create(m_XrInstance, create_info) >> m_ActionSet;
}

core::result<> core::Application::CreateActions()
{
    return ok()
           & [&]
           {
               return CreateAction(
                          "grab",
                          "Grab",
                          XR_ACTION_TYPE_FLOAT_INPUT,
                          { "/user/hand/left", "/user/hand/right" }) >> m_ActionGrab;
           }
           & [&]
           {
               return CreateAction(
                          "palm-pose",
                          "Palm Pose",
                          XR_ACTION_TYPE_POSE_INPUT,
                          { "/user/hand/left", "/user/hand/right" }) >> m_ActionPalmPose;
           }
           & [&]
           {
               return CreateAction(
                          "haptic",
                          "Haptic",
                          XR_ACTION_TYPE_VIBRATION_OUTPUT,
                          { "/user/hand/left", "/user/hand/right" }) >> m_ActionHaptic;
           };
}

core::result<core::xr::Action> core::Application::CreateAction(
    const std::string &name,
    const std::string &localized_name,
    const XrActionType type,
    const std::vector<std::string> &sub_path_strings)
{
    std::vector<XrPath> sub_paths(sub_path_strings.size());
    for (uint32_t i = 0; i < sub_path_strings.size(); ++i)
    {
        if (auto res = xr::StringToPath(m_XrInstance, sub_path_strings[i]) >> sub_paths[i])
            return res;
    }

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

core::result<> core::Application::CreateHands()
{
    return ok()
           & [&]
           {
               return xr::StringToPath(m_XrInstance, "/user/hand/left") >> m_Hands[0].Path;
           }
           & [&]
           {
               return xr::StringToPath(m_XrInstance, "/user/hand/right") >> m_Hands[1].Path;
           };
}

core::result<> core::Application::SuggestBindings()
{
    return ok()
           & [&]
           {
               return xr::SuggestInteractionProfileBindings(
                   m_XrInstance,
                   "/interaction_profiles/khr/simple_controller",
                   {
                       { m_ActionGrab, "/user/hand/left/input/select/click" },
                       { m_ActionGrab, "/user/hand/right/input/select/click" },
                       { m_ActionPalmPose, "/user/hand/left/input/grip/pose" },
                       { m_ActionPalmPose, "/user/hand/right/input/grip/pose" },
                       { m_ActionHaptic, "/user/hand/left/output/haptic" },
                       { m_ActionHaptic, "/user/hand/right/output/haptic" },
                   });
           }
           | [&]
           {
               return xr::SuggestInteractionProfileBindings(
                   m_XrInstance,
                   "/interaction_profiles/oculus/touch_controller",
                   {
                       { m_ActionGrab, "/user/hand/left/input/squeeze/value" },
                       { m_ActionGrab, "/user/hand/right/input/squeeze/value" },
                       { m_ActionPalmPose, "/user/hand/left/input/grip/pose" },
                       { m_ActionPalmPose, "/user/hand/right/input/grip/pose" },
                       { m_ActionHaptic, "/user/hand/left/output/haptic" },
                       { m_ActionHaptic, "/user/hand/right/output/haptic" },
                   });
           };
}

core::result<> core::Application::RecordBindings()
{
    if (!m_Session)
        return ok();

    return ok()
           & [&]
           {
               return xr::GetCurrentInteractionProfile(m_Session, m_Hands[0].Path);
           }
           & [&](XrInteractionProfileState &&state)
           {
               if (state.interactionProfile)
               {
                   std::string str;
                   if (auto res = xr::PathToString(m_XrInstance, state.interactionProfile) >> str)
                       return result<XrInteractionProfileState>(std::move(res));
                   info("/user/hand/left => ", str);
               }

               return xr::GetCurrentInteractionProfile(m_Session, m_Hands[1].Path);
           }
           & [&](XrInteractionProfileState &&state)
           {
               if (state.interactionProfile)
               {
                   std::string str;
                   if (auto res = xr::PathToString(m_XrInstance, state.interactionProfile) >> str)
                       return res;
                   info("/user/hand/left => ", str);
               }

               return ok();
           };
}

core::result<> core::Application::CreateActionSpaces()
{
    return ok()
           & [&]
           {
               return CreateActionSpace(m_ActionPalmPose, "/user/hand/left") >> m_Hands[0].Space;
           }
           & [&]
           {
               return CreateActionSpace(m_ActionPalmPose, "/user/hand/right") >> m_Hands[1].Space;
           };
}

core::result<core::xr::ActionSpace> core::Application::CreateActionSpace(
    XrAction action,
    const std::optional<std::string> &sub_path_string)
{
    XrPath sub_path{};
    if (sub_path_string)
        if (auto res = xr::StringToPath(m_XrInstance, *sub_path_string) >> sub_path)
            return res;

    const XrActionSpaceCreateInfo create_info
    {
        .type = XR_TYPE_ACTION_SPACE_CREATE_INFO,
        .action = action,
        .subactionPath = sub_path,
        .poseInActionSpace = {
            .orientation = { 0.0f, 0.0f, 0.0f, 1.0f },
            .position = { 0.0f, 0.0f, 0.0f },
        },
    };

    return xr::ActionSpace::create(m_Session, create_info);
}

core::result<> core::Application::AttachActionSet()
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
        return error("xrAttachSessionActionSets => {}", res);
    return ok();
}
