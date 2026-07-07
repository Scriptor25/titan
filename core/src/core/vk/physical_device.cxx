#include <titan/core.hxx>
#include <titan/utils.hxx>

toolkit::result<> titan::Application::GetPhysicalDevice()
{
    const XrVulkanGraphicsDeviceGetInfoKHR get_info
    {
        .type = XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR,
        .systemId = m_SystemId,
        .vulkanInstance = m_VkInstance,
    };

    VkPhysicalDevice physical_device;
    HANDLE(xr::GetVulkanGraphicsDevice2KHR(m_XrInstance, get_info) >> physical_device);

    m_PhysicalDevice = vk::PhysicalDevice::wrap(physical_device);

    const auto properties = vk::GetPhysicalDeviceProperties2(m_PhysicalDevice).properties;

    if (sizeof(ShaderData) > properties.limits.maxPushConstantsSize)
        return toolkit::make_error(
            "shader data struct size is greater than physical device max push constants size ({} > {}).",
            sizeof(ShaderData),
            properties.limits.maxPushConstantsSize);

    return {};
}
