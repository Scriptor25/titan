#include <titan/core.hxx>
#include <titan/utils.hxx>

#define XR_TO_VK_VERSION(VERSION) VK_MAKE_VERSION(XR_VERSION_MAJOR(VERSION), XR_VERSION_MINOR(VERSION), XR_VERSION_PATCH(VERSION))

core::result<> core::Application::CreateVkInstance()
{
    return ok()
           & [&]
           {
               return xr::GetVulkanGraphicsRequirements2KHR(m_XrInstance, m_SystemId);
           }
           & [&](XrGraphicsRequirementsVulkan2KHR &&graphics_requirements)
           {
               std::vector<const char *> extensions;
               GetInstanceExtensions(extensions);

               const VkApplicationInfo application_info
               {
                   .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                   .pApplicationName = m_Info.Name.c_str(),
                   .applicationVersion = VK_MAKE_VERSION(
                       m_Info.Version.Major,
                       m_Info.Version.Minor,
                       m_Info.Version.Patch),
                   .pEngineName = "Titan Core",
                   .engineVersion = VK_MAKE_VERSION(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH),
                   .apiVersion = XR_TO_VK_VERSION(graphics_requirements.maxApiVersionSupported),
               };

               const VkInstanceCreateInfo instance_create_info
               {
                   .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                   .pApplicationInfo = &application_info,
                   .enabledLayerCount = VK_INSTANCE_LAYERS.size(),
                   .ppEnabledLayerNames = VK_INSTANCE_LAYERS.data(),
                   .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
                   .ppEnabledExtensionNames = extensions.data(),
               };

               const XrVulkanInstanceCreateInfoKHR vulkan_instance_create_info
               {
                   .type = XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR,
                   .systemId = m_SystemId,
                   .pfnGetInstanceProcAddr = vkGetInstanceProcAddr,
                   .vulkanCreateInfo = &instance_create_info,
               };

               return xr::CreateVulkanInstanceKHR(m_XrInstance, vulkan_instance_create_info) >> m_VkInstance;
           };
}
