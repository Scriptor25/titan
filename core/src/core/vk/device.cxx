#include <titan/core.hxx>
#include <titan/utils.hxx>

#include <set>

core::result<> core::Application::CreateDevice()
{
    std::vector<const char *> extensions;
    GetDeviceExtensions(extensions);

    auto queue_priority = 1.0f;

    const std::set<uint32_t> queue_family_indices
    {
        reinterpret_cast<uint32_t *>(&m_QueueFamilyIndices),
        reinterpret_cast<uint32_t *>(&m_QueueFamilyIndices + 1)
    };

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    queue_create_infos.reserve(queue_family_indices.size());

    for (auto &queue_family_index : queue_family_indices)
        queue_create_infos.push_back(
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = queue_family_index,
                .queueCount = 1,
                .pQueuePriorities = &queue_priority,
            });

    auto physical_device_features = vk::GetPhysicalDeviceFeatures2(m_PhysicalDevice);

    VkPhysicalDeviceSynchronization2Features synchronization2_features
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .synchronization2 = true,
    };

    VkPhysicalDeviceMultiviewFeatures multiview_features
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES,
        .pNext = &synchronization2_features,
        .multiview = true,
        .multiviewGeometryShader = false,
        .multiviewTessellationShader = false,
    };

    VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address_features
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .pNext = &multiview_features,
        .bufferDeviceAddress = true,
    };

    const VkDeviceCreateInfo device_create_info
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &buffer_device_address_features,
        .queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size()),
        .pQueueCreateInfos = queue_create_infos.data(),
        .enabledLayerCount = VK_DEVICE_LAYERS.size(),
        .ppEnabledLayerNames = VK_DEVICE_LAYERS.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
        .pEnabledFeatures = &physical_device_features.features,
    };

    const XrVulkanDeviceCreateInfoKHR create_info
    {
        .type = XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR,
        .systemId = m_SystemId,
        .pfnGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vulkanPhysicalDevice = m_PhysicalDevice,
        .vulkanCreateInfo = &device_create_info,
    };

    return xr::CreateVulkanDeviceKHR(m_XrInstance, create_info) >> m_Device;
}
