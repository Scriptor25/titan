#include <titan/utils.hxx>

core::result<std::vector<XrViewConfigurationType>> core::xr::EnumerateViewConfigurationTypes(
    XrInstance instance,
    XrSystemId system_id)
{
    return Enumerate<XrViewConfigurationType>(
        xrEnumerateViewConfigurations,
        {},
        instance,
        system_id);
}

core::result<std::vector<XrViewConfigurationView>> core::xr::EnumerateViewConfigurationViews(
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

core::result<std::vector<XrEnvironmentBlendMode>> core::xr::EnumerateEnvironmentBlendModes(
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

core::result<std::vector<XrView>> core::xr::LocateViews(
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

core::result<XrGraphicsRequirementsVulkanKHR> core::xr::GetVulkanGraphicsRequirements2KHR(
    XrInstance instance,
    XrSystemId system_id)
{
    XrGraphicsRequirementsVulkan2KHR graphics_requirements{ .type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR };
    if (auto res = xrGetVulkanGraphicsRequirements2KHR(instance, system_id, &graphics_requirements))
        return error<XrGraphicsRequirementsVulkan2KHR>("xrGetVulkanGraphicsRequirements2KHR => {}", res);
    return graphics_requirements;
}

core::result<core::vk::Instance> core::xr::CreateVulkanInstanceKHR(
    XrInstance instance,
    const XrVulkanInstanceCreateInfoKHR &create_info)
{
    VkInstance vulkan_instance;
    VkResult vulkan_result;

    if (auto res = xrCreateVulkanInstanceKHR(instance, &create_info, &vulkan_instance, &vulkan_result))
        return error<vk::Instance>("xrCreateVulkanInstanceKHR => {}, vkCreateInstance => {}", res, vulkan_result);

    return vk::Instance::wrap(vulkan_instance);
}

core::result<core::vk::Device> core::xr::CreateVulkanDeviceKHR(
    XrInstance instance,
    const XrVulkanDeviceCreateInfoKHR &create_info)
{
    VkDevice vulkan_device;
    VkResult vulkan_result;

    if (auto res = xrCreateVulkanDeviceKHR(instance, &create_info, &vulkan_device, &vulkan_result))
        return error<vk::Device>("xrCreateVulkanDeviceKHR => {}, vkCreateDevice => {}", res, vulkan_result);

    return vk::Device::wrap(vulkan_device);
}

core::result<VkPhysicalDevice_T *> core::xr::GetVulkanGraphicsDevice2KHR(
    XrInstance instance,
    const XrVulkanGraphicsDeviceGetInfoKHR &get_info)
{
    VkPhysicalDevice vulkan_physical_device;

    if (auto res = xrGetVulkanGraphicsDevice2KHR(instance, &get_info, &vulkan_physical_device))
        return error<VkPhysicalDevice>("xrGetVulkanGraphicsDevice2KHR => {}", res);

    return vulkan_physical_device;
}

core::result<XrSystemId> core::xr::GetSystem(XrInstance instance, const XrSystemGetInfo &get_info)
{
    XrSystemId system_id;
    if (auto res = xrGetSystem(instance, &get_info, &system_id))
        return error<XrSystemId>("xrGetSystem => {}", res);
    return system_id;
}
