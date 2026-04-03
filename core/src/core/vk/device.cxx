#include <titan/core.hxx>

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

    VkPhysicalDeviceFeatures2 physical_device_features
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    };
    vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &physical_device_features);

    const VkPhysicalDeviceSynchronization2Features synchronization2_features
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .synchronization2 = true,
    };

    const VkDeviceCreateInfo device_create_info
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &synchronization2_features,
        .queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size()),
        .pQueueCreateInfos = queue_create_infos.data(),
        .enabledLayerCount = VK_DEVICE_LAYERS.size(),
        .ppEnabledLayerNames = VK_DEVICE_LAYERS.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
        .pEnabledFeatures = &physical_device_features.features,
    };

    const XrVulkanDeviceCreateInfoKHR vk_device_create_info
    {
        .type = XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR,
        .systemId = m_SystemId,
        .pfnGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vulkanPhysicalDevice = m_PhysicalDevice,
        .vulkanCreateInfo = &device_create_info,
    };

    VkDevice vk_device;
    VkResult vk_result;

    if (auto res = xrCreateVulkanDeviceKHR(m_XrInstance, &vk_device_create_info, &vk_device, &vk_result);
        res || vk_result)
        return error("xrCreateVulkanDeviceKHR => {}, {}", res, vk_result);

    m_Device = vk::Device::wrap(vk_device);
    return ok();
}
