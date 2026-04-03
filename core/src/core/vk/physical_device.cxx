#include <titan/core.hxx>
#include <titan/utils.hxx>

core::result<> core::Application::GetPhysicalDevice()
{
    return ok()
           & [&]
           {
               const XrVulkanGraphicsDeviceGetInfoKHR get_info
               {
                   .type = XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR,
                   .systemId = m_SystemId,
                   .vulkanInstance = m_VkInstance,
               };

               return xr::GetVulkanGraphicsDevice2KHR(m_XrInstance, get_info) >> m_PhysicalDevice;
           }
           & [&]
           {
               const auto properties = vk::GetPhysicalDeviceProperties2(m_PhysicalDevice).properties;

               if (sizeof(CameraData) <= properties.limits.maxPushConstantsSize)
                   return ok();

               return error(
                   "camera data struct size is greater than physical device max push constants size ({} > {}).",
                   sizeof(CameraData),
                   properties.limits.maxPushConstantsSize);
           };
}
