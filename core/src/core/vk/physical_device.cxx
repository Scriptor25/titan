#include <titan/core.hxx>

core::result<> core::Application::GetPhysicalDevice()
{
    const XrVulkanGraphicsDeviceGetInfoKHR get_info
    {
        .type = XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR,
        .systemId = m_SystemId,
        .vulkanInstance = m_VkInstance,
    };

    VkPhysicalDevice physical_device;
    if (auto res = xrGetVulkanGraphicsDevice2KHR(m_XrInstance, &get_info, &physical_device))
        return error("xrGetVulkanGraphicsDevice2KHR => {}", res);

    m_PhysicalDevice = physical_device;

    VkPhysicalDeviceProperties2 physical_device_properties
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
    };
    vkGetPhysicalDeviceProperties2(m_PhysicalDevice, &physical_device_properties);

    if (sizeof(CameraData) > physical_device_properties.properties.limits.maxPushConstantsSize)
        return error(
            "camera data struct size is greater than physical device max push constants size ({} > {}).",
            sizeof(CameraData),
            physical_device_properties.properties.limits.maxPushConstantsSize);

    return ok();
}
