#include <titan/utils.hxx>

titan::result<std::vector<XrViewConfigurationType>> titan::xr::EnumerateViewConfigurationTypes(
    XrInstance instance,
    XrSystemId system_id)
{
    return Enumerate<XrViewConfigurationType>(
        xrEnumerateViewConfigurations,
        {},
        instance,
        system_id);
}

titan::result<std::vector<XrViewConfigurationView>> titan::xr::EnumerateViewConfigurationViews(
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

titan::result<std::vector<XrEnvironmentBlendMode>> titan::xr::EnumerateEnvironmentBlendModes(
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

titan::result<std::vector<XrView>> titan::xr::LocateViews(
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

titan::result<XrGraphicsRequirementsVulkanKHR> titan::xr::GetVulkanGraphicsRequirements2KHR(
    XrInstance instance,
    XrSystemId system_id)
{
    XrGraphicsRequirementsVulkan2KHR graphics_requirements{ .type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR };
    if (auto res = xrGetVulkanGraphicsRequirements2KHR(instance, system_id, &graphics_requirements))
        return error<XrGraphicsRequirementsVulkan2KHR>("xrGetVulkanGraphicsRequirements2KHR => {}", res);
    return graphics_requirements;
}

titan::result<titan::vk::Instance> titan::xr::CreateVulkanInstanceKHR(
    XrInstance instance,
    const XrVulkanInstanceCreateInfoKHR &create_info)
{
    VkInstance vulkan_instance;
    VkResult vulkan_result;

    if (auto res = xrCreateVulkanInstanceKHR(instance, &create_info, &vulkan_instance, &vulkan_result))
        return error<vk::Instance>("xrCreateVulkanInstanceKHR => {}, vkCreateInstance => {}", res, vulkan_result);

    return vk::Instance::wrap({}, vulkan_instance);
}

titan::result<titan::vk::Device> titan::xr::CreateVulkanDeviceKHR(
    XrInstance instance,
    const XrVulkanDeviceCreateInfoKHR &create_info)
{
    VkDevice vulkan_device;
    VkResult vulkan_result;

    if (auto res = xrCreateVulkanDeviceKHR(instance, &create_info, &vulkan_device, &vulkan_result))
        return error<vk::Device>("xrCreateVulkanDeviceKHR => {}, vkCreateDevice => {}", res, vulkan_result);

    return vk::Device::wrap({}, vulkan_device);
}

titan::result<VkPhysicalDevice_T *> titan::xr::GetVulkanGraphicsDevice2KHR(
    XrInstance instance,
    const XrVulkanGraphicsDeviceGetInfoKHR &get_info)
{
    VkPhysicalDevice vulkan_physical_device;

    if (auto res = xrGetVulkanGraphicsDevice2KHR(instance, &get_info, &vulkan_physical_device))
        return error<VkPhysicalDevice>("xrGetVulkanGraphicsDevice2KHR => {}", res);

    return vulkan_physical_device;
}

titan::result<XrSystemId> titan::xr::GetSystem(XrInstance instance, const XrSystemGetInfo &get_info)
{
    XrSystemId system_id;
    if (auto res = xrGetSystem(instance, &get_info, &system_id))
        return error<XrSystemId>("xrGetSystem => {}", res);
    return system_id;
}

titan::result<XrFrameState> titan::xr::WaitFrame(XrSession session, const XrFrameWaitInfo &frame_wait_info)
{
    XrFrameState frame_state{ .type = XR_TYPE_FRAME_STATE };
    if (auto res = xrWaitFrame(session, &frame_wait_info, &frame_state))
        return error<XrFrameState>("xrWaitFrame => {}", res);
    return frame_state;
}

titan::result<> titan::xr::BeginFrame(XrSession session, const XrFrameBeginInfo &frame_begin_info)
{
    if (auto res = xrBeginFrame(session, &frame_begin_info))
        return error("xrBeginFrame => {}", res);
    return ok();
}

titan::result<> titan::xr::EndFrame(XrSession session, const XrFrameEndInfo &frame_end_info)
{
    if (auto res = xrEndFrame(session, &frame_end_info))
        return error("xrEndFrame => {}", res);
    return ok();
}

titan::result<XrPath> titan::xr::StringToPath(XrInstance instance, const std::string &str)
{
    XrPath path;
    if (const auto res = xrStringToPath(instance, str.c_str(), &path))
        return error<XrPath>("xrStringToPath => {}", res);
    return path;
}

titan::result<std::string> titan::xr::PathToString(XrInstance instance, XrPath path)
{
    uint32_t capacity;
    if (auto res = xrPathToString(instance, path, 0, &capacity, nullptr))
        return error<std::string>("xrPathToString => {}", res);

    std::vector<char> buffer(capacity);
    if (auto res = xrPathToString(instance, path, buffer.size(), &capacity, buffer.data()))
        return error<std::string>("xrPathToString => {}", res);

    return std::string(buffer.begin(), buffer.end());
}

titan::result<> titan::xr::SuggestInteractionProfileBindings(
    XrInstance instance,
    const std::string &profile,
    const std::vector<std::pair<XrAction, std::string>> &bindings)
{
    XrPath profile_path;
    if (auto res = StringToPath(instance, profile) >> profile_path)
        return res;

    std::vector<XrActionSuggestedBinding> suggested_bindings(bindings.size());
    for (uint32_t i = 0; i < bindings.size(); ++i)
    {
        XrPath binding;
        if (auto res = StringToPath(instance, bindings[i].second) >> binding)
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
        return error("xrSuggestInteractionProfileBindings => {}", res);
    return ok();
}

titan::result<XrInteractionProfileState> titan::xr::GetCurrentInteractionProfile(XrSession session, XrPath path)
{
    XrInteractionProfileState interaction_profile_state{ .type = XR_TYPE_INTERACTION_PROFILE_STATE };
    if (auto res = xrGetCurrentInteractionProfile(session, path, &interaction_profile_state))
        return error<XrInteractionProfileState>("xrGetCurrentInteractionProfile => {}", res);
    return interaction_profile_state;
}
