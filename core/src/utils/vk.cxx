#include <titan/utils.hxx>

core::result<std::vector<VkSurfaceFormat2KHR>> core::vk::GetPhysicalDeviceSurfaceFormats2KHR(
    VkPhysicalDevice physical_device,
    const VkPhysicalDeviceSurfaceInfo2KHR &surface_info)
{
    return Enumerate<VkSurfaceFormat2KHR>(
        vkGetPhysicalDeviceSurfaceFormats2KHR,
        { .sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR },
        physical_device,
        &surface_info);
}

core::result<std::vector<VkPresentModeKHR>> core::vk::GetPhysicalDeviceSurfacePresentModesKHR(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface)
{
    return Enumerate<VkPresentModeKHR>(
        vkGetPhysicalDeviceSurfacePresentModesKHR,
        {},
        physical_device,
        surface);
}

core::result<std::vector<VkImage>> core::vk::GetSwapchainImagesKHR(
    VkDevice device,
    VkSwapchainKHR swapchain)
{
    return Enumerate<VkImage>(
        vkGetSwapchainImagesKHR,
        {},
        device,
        swapchain);
}

std::vector<VkQueueFamilyProperties2> core::vk::GetPhysicalDeviceQueueFamilyProperties2(
    VkPhysicalDevice physical_device)
{
    return Get<VkQueueFamilyProperties2>(
        vkGetPhysicalDeviceQueueFamilyProperties2,
        { .sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2 },
        physical_device);
}

VkPhysicalDeviceFeatures2 core::vk::GetPhysicalDeviceFeatures2(VkPhysicalDevice physical_device)
{
    VkPhysicalDeviceFeatures2 physical_device_features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    vkGetPhysicalDeviceFeatures2(physical_device, &physical_device_features);
    return physical_device_features;
}

VkMemoryRequirements2 core::vk::GetBufferMemoryRequirements2(
    VkDevice device,
    const VkBufferMemoryRequirementsInfo2 &requirements_info)
{
    VkMemoryRequirements2 requirements{ .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
    vkGetBufferMemoryRequirements2(device, &requirements_info, &requirements);
    return requirements;
}

core::result<> core::vk::BindBufferMemory2(VkDevice device, const VkBindBufferMemoryInfo &bind_info)
{
    if (auto res = vkBindBufferMemory2(device, 1, &bind_info))
        return error("vkBindBufferMemory2 => {}", res);
    return ok();
}

core::result<> core::vk::BindBufferMemory2(VkDevice device, const std::vector<VkBindBufferMemoryInfo> &bind_infos)
{
    if (auto res = vkBindBufferMemory2(device, bind_infos.size(), bind_infos.data()))
        return error("vkBindBufferMemory2 => {}", res);
    return ok();
}

core::result<> core::vk::BindImageMemory2(VkDevice device, const VkBindImageMemoryInfo &bind_info)
{
    if (auto res = vkBindImageMemory2(device, 1, &bind_info))
        return error("vkBindImageMemory2 => {}", res);
    return ok();
}

core::result<> core::vk::BindImageMemory2(VkDevice device, const std::vector<VkBindImageMemoryInfo> &bind_infos)
{
    if (auto res = vkBindImageMemory2(device, bind_infos.size(), bind_infos.data()))
        return error("vkBindImageMemory2 => {}", res);
    return ok();
}

VkPhysicalDeviceProperties2 core::vk::GetPhysicalDeviceProperties2(VkPhysicalDevice physical_device)
{
    VkPhysicalDeviceProperties2 properties{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
    vkGetPhysicalDeviceProperties2(physical_device, &properties);
    return properties;
}

core::result<std::vector<char>> core::vk::GetPipelineCacheData(VkDevice device, VkPipelineCache pipeline_cache)
{
    size_t data_size;
    if (auto res = vkGetPipelineCacheData(device, pipeline_cache, &data_size, nullptr))
        return error<std::vector<char>>("vkGetPipelineCacheData => {}", res);

    std::vector<char> data(data_size);
    if (auto res = vkGetPipelineCacheData(device, pipeline_cache, &data_size, data.data()))
        return error<std::vector<char>>("vkGetPipelineCacheData => {}", res);

    return data;
}

core::result<> core::vk::ResetCommandBuffer(VkCommandBuffer command_buffer, VkCommandBufferResetFlags flags)
{
    if (auto res = vkResetCommandBuffer(command_buffer, flags))
        return error("vkResetCommandBuffer => {}", res);
    return ok();
}

core::result<> core::vk::BeginCommandBuffer(VkCommandBuffer command_buffer, const VkCommandBufferBeginInfo &begin_info)
{
    if (auto res = vkBeginCommandBuffer(command_buffer, &begin_info))
        return error("vkBeginCommandBuffer => {}", res);
    return ok();
}

core::result<> core::vk::EndCommandBuffer(VkCommandBuffer command_buffer)
{
    if (auto res = vkEndCommandBuffer(command_buffer))
        return error("vkEndCommandBuffer => {}", res);
    return ok();
}

core::result<void *> core::vk::MapMemory2(VkDevice device, const VkMemoryMapInfo &map_info)
{
    void *data;
    if (auto res = vkMapMemory2(device, &map_info, &data))
        return error<void *>("vkMapMemory2 => {}", res);
    return data;
}

core::result<> core::vk::UnmapMemory2(VkDevice device, const VkMemoryUnmapInfo &unmap_info)
{
    if (auto res = vkUnmapMemory2(device, &unmap_info))
        return error("vkUnmapMemory2 => {}", res);
    return ok();
}

core::result<VkSurfaceCapabilities2KHR> core::vk::GetPhysicalDeviceSurfaceCapabilities2KHR(
    VkPhysicalDevice physical_device,
    const VkPhysicalDeviceSurfaceInfo2KHR &surface_info)
{
    VkSurfaceCapabilities2KHR surface_capabilities{ .sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR };
    if (auto res = vkGetPhysicalDeviceSurfaceCapabilities2KHR(physical_device, &surface_info, &surface_capabilities))
        return error<VkSurfaceCapabilities2KHR>("vkGetPhysicalDeviceSurfaceCapabilities2KHR => {}", res);
    return surface_capabilities;
}
