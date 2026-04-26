#include <titan/utils.hxx>

toolkit::result<std::vector<XrViewConfigurationType>> titan::xr::EnumerateViewConfigurationTypes(
    XrInstance instance,
    XrSystemId system_id)
{
    return Enumerate<XrViewConfigurationType>(
        xrEnumerateViewConfigurations,
        {},
        instance,
        system_id);
}

toolkit::result<std::vector<XrViewConfigurationView>> titan::xr::EnumerateViewConfigurationViews(
    XrInstance instance,
    XrSystemId system_id,
    XrViewConfigurationType view_configuration_type)
{
    return Enumerate<XrViewConfigurationView>(
        xrEnumerateViewConfigurationViews,
        { .type = XR_TYPE_VIEW_CONFIGURATION_VIEW },
        instance,
        system_id,
        view_configuration_type);
}

toolkit::result<std::vector<XrEnvironmentBlendMode>> titan::xr::EnumerateEnvironmentBlendModes(
    XrInstance instance,
    XrSystemId system_id,
    XrViewConfigurationType view_configuration_type)
{
    return Enumerate<XrEnvironmentBlendMode>(
        xrEnumerateEnvironmentBlendModes,
        {},
        instance,
        system_id,
        view_configuration_type);
}

toolkit::result<std::vector<XrView>> titan::xr::LocateViews(
    XrSession session,
    const XrViewLocateInfo &view_locate_info,
    XrViewState &view_state)
{
    return Enumerate<XrView>(
        xrLocateViews,
        { .type = XR_TYPE_VIEW },
        session,
        &view_locate_info,
        &view_state);
}

toolkit::result<XrGraphicsRequirementsVulkanKHR> titan::xr::GetVulkanGraphicsRequirements2KHR(
    XrInstance instance,
    XrSystemId system_id)
{
    XrGraphicsRequirementsVulkan2KHR graphics_requirements{ .type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR };
    if (auto res = xrGetVulkanGraphicsRequirements2KHR(instance, system_id, &graphics_requirements))
        return toolkit::make_error("xrGetVulkanGraphicsRequirements2KHR => {}", res);
    return graphics_requirements;
}

toolkit::result<titan::vk::Instance> titan::xr::CreateVulkanInstanceKHR(
    XrInstance instance,
    const XrVulkanInstanceCreateInfoKHR &create_info)
{
    VkInstance vulkan_instance;
    VkResult vulkan_result;

    if (auto res = xrCreateVulkanInstanceKHR(instance, &create_info, &vulkan_instance, &vulkan_result))
        return toolkit::make_error("xrCreateVulkanInstanceKHR => {}, vkCreateInstance => {}", res, vulkan_result);

    return vk::Instance::wrap({}, vulkan_instance);
}

toolkit::result<titan::vk::Device> titan::xr::CreateVulkanDeviceKHR(
    XrInstance instance,
    const XrVulkanDeviceCreateInfoKHR &create_info)
{
    VkDevice vulkan_device;
    VkResult vulkan_result;

    if (auto res = xrCreateVulkanDeviceKHR(instance, &create_info, &vulkan_device, &vulkan_result))
        return toolkit::make_error("xrCreateVulkanDeviceKHR => {}, vkCreateDevice => {}", res, vulkan_result);

    return vk::Device::wrap({}, vulkan_device);
}

toolkit::result<VkPhysicalDevice_T *> titan::xr::GetVulkanGraphicsDevice2KHR(
    XrInstance instance,
    const XrVulkanGraphicsDeviceGetInfoKHR &get_info)
{
    VkPhysicalDevice vulkan_physical_device;

    if (auto res = xrGetVulkanGraphicsDevice2KHR(instance, &get_info, &vulkan_physical_device))
        return toolkit::make_error("xrGetVulkanGraphicsDevice2KHR => {}", res);

    return vulkan_physical_device;
}

toolkit::result<XrSystemId> titan::xr::GetSystem(XrInstance instance, const XrSystemGetInfo &get_info)
{
    XrSystemId system_id;
    if (auto res = xrGetSystem(instance, &get_info, &system_id))
        return toolkit::make_error("xrGetSystem => {}", res);
    return system_id;
}

toolkit::result<XrFrameState> titan::xr::WaitFrame(XrSession session, const XrFrameWaitInfo &frame_wait_info)
{
    XrFrameState frame_state{ .type = XR_TYPE_FRAME_STATE };
    if (auto res = xrWaitFrame(session, &frame_wait_info, &frame_state))
        return toolkit::make_error("xrWaitFrame => {}", res);
    return frame_state;
}

toolkit::result<> titan::xr::BeginFrame(XrSession session, const XrFrameBeginInfo &frame_begin_info)
{
    if (auto res = xrBeginFrame(session, &frame_begin_info))
        return toolkit::make_error("xrBeginFrame => {}", res);
    return ok();
}

toolkit::result<> titan::xr::EndFrame(XrSession session, const XrFrameEndInfo &frame_end_info)
{
    if (auto res = xrEndFrame(session, &frame_end_info))
        return toolkit::make_error("xrEndFrame => {}", res);
    return ok();
}

toolkit::result<XrPath> titan::xr::StringToPath(XrInstance instance, const std::string &str)
{
    XrPath path;
    if (const auto res = xrStringToPath(instance, str.c_str(), &path))
        return toolkit::make_error("xrStringToPath => {}", res);
    return path;
}

toolkit::result<std::string> titan::xr::PathToString(XrInstance instance, XrPath path)
{
    uint32_t capacity;
    if (auto res = xrPathToString(instance, path, 0, &capacity, nullptr))
        return toolkit::make_error("xrPathToString => {}", res);

    std::vector<char> buffer(capacity);
    if (auto res = xrPathToString(instance, path, buffer.size(), &capacity, buffer.data()))
        return toolkit::make_error("xrPathToString => {}", res);

    return std::string(buffer.begin(), buffer.end());
}

toolkit::result<> titan::xr::SuggestInteractionProfileBindings(
    XrInstance instance,
    const std::string &profile,
    const std::vector<std::pair<XrAction, std::string>> &bindings)
{
    XrPath profile_path;
    if (auto res = StringToPath(instance, profile) >> profile_path; !res)
        return res;

    std::vector<XrActionSuggestedBinding> suggested_bindings(bindings.size());
    for (uint32_t i = 0; i < bindings.size(); ++i)
    {
        XrPath binding;
        if (auto res = StringToPath(instance, bindings[i].second) >> binding; !res)
            return res;

        suggested_bindings[i] = {
            .action = bindings[i].first,
            .binding = binding,
        };
    }

    const XrInteractionProfileSuggestedBinding profile_suggested_bindings
    {
        .type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING,
        .interactionProfile = profile_path,
        .countSuggestedBindings = static_cast<uint32_t>(suggested_bindings.size()),
        .suggestedBindings = suggested_bindings.data(),
    };

    if (auto res = xrSuggestInteractionProfileBindings(instance, &profile_suggested_bindings))
        return toolkit::make_error("xrSuggestInteractionProfileBindings => {}", res);
    return ok();
}

toolkit::result<XrInteractionProfileState> titan::xr::GetCurrentInteractionProfile(XrSession session, XrPath path)
{
    XrInteractionProfileState interaction_profile_state{ .type = XR_TYPE_INTERACTION_PROFILE_STATE };
    if (auto res = xrGetCurrentInteractionProfile(session, path, &interaction_profile_state))
        return toolkit::make_error("xrGetCurrentInteractionProfile => {}", res);
    return interaction_profile_state;
}
