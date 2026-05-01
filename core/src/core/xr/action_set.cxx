#include <titan/core.hxx>
#include <titan/log.hxx>
#include <titan/utils.hxx>

#include <cstring>

toolkit::result<> titan::Application::CreateActionSet()
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

toolkit::result<> titan::Application::CreateActions()
{
    return toolkit::result()
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

toolkit::result<titan::xr::Action> titan::Application::CreateAction(
    const std::string &name,
    const std::string &localized_name,
    const XrActionType type,
    const std::vector<std::string> &sub_path_strings)
{
    std::vector<XrPath> sub_paths(sub_path_strings.size());
    for (uint32_t i = 0; i < sub_path_strings.size(); ++i)
    {
        if (auto res = xr::StringToPath(m_XrInstance, sub_path_strings[i]) >> sub_paths[i]; !res)
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

toolkit::result<> titan::Application::SuggestBindings()
{
    return toolkit::result()
           & [&]
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
           }
           | [&](auto &&)
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
           };
}

toolkit::result<> titan::Application::RecordBindings()
{
    if (!m_Session)
        return {};

    return m_Inputs.RecordBindings(
        {
            .Instance = m_XrInstance,
            .Session = m_Session,
        });
}

toolkit::result<> titan::Application::AttachActionSet()
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
