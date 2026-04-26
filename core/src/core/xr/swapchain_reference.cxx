#include <titan/core.hxx>
#include <titan/utils.hxx>

toolkit::result<titan::XrSwapchainReference> titan::Application::CreateSwapchainReference(
    const XrSwapchainReferenceCreateInfo &create_info)
{
    XrSwapchainReference reference
    {
        .Format = create_info.imageFormat,
    };

    if (create_info.useSwapchain)
    {
        const XrSwapchainCreateInfo swapchain_create_info
        {
            .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
            .usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | create_info.usageFlags,
            .format = create_info.imageFormat,
            .sampleCount = create_info.sampleCount,
            .width = create_info.imageExtent.width,
            .height = create_info.imageExtent.height,
            .faceCount = 1,
            .arraySize = 1,
            .mipCount = 1,
        };

        if (auto res = xr::Swapchain::create(m_Session, swapchain_create_info) >> reference.Swapchain; !res)
            return res;

        auto set_images = [&](const std::vector<XrSwapchainImageVulkan2KHR> &images)
        {
            reference.Images.resize(images.size());
            for (uint32_t i = 0; i < images.size(); ++i)
                reference.Images[i] = vk::Image::wrap(images[i].image);
            return ok();
        };

        if (auto res = xr::EnumerateSwapchainImages<XrSwapchainImageVulkan2KHR>(
                           reference.Swapchain,
                           { .type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR })
                       & set_images; !res)
            return res;
    }
    else
    {
        reference.Images.resize(create_info.imageCount);
        reference.Memory.resize(create_info.imageCount);

        for (uint32_t i = 0; i < create_info.imageCount; ++i)
        {
            auto &image = reference.Images[i];
            auto &memory = reference.Memory[i];

            const VkImageCreateInfo image_create_info
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = create_info.imageFormat,
                .extent = {
                    .width = create_info.imageExtent.width,
                    .height = create_info.imageExtent.height,
                    .depth = 1,
                },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = create_info.imageUsage,
                .sharingMode = create_info.imageSharingMode,
                .queueFamilyIndexCount = create_info.queueFamilyIndexCount,
                .pQueueFamilyIndices = create_info.pQueueFamilyIndices,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            };

            if (auto res = vk::Image::create(m_Device, image_create_info) >> image; !res)
                return res;

            const VkImageMemoryRequirementsInfo2 memory_requirements_info
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
                .image = image,
            };

            VkMemoryRequirements2 memory_requirements2
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
            };
            vkGetImageMemoryRequirements2(m_Device, &memory_requirements_info, &memory_requirements2);

            auto &memory_requirements = memory_requirements2.memoryRequirements;

            uint32_t memory_type_index;
            if (auto res = FindMemoryType(
                               m_PhysicalDevice,
                               memory_requirements.memoryTypeBits,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                           >> memory_type_index; !res)
                return res;

            const VkMemoryAllocateInfo allocate_info
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = memory_requirements.size,
                .memoryTypeIndex = memory_type_index,
            };

            if (auto res = vk::DeviceMemory::create(m_Device, allocate_info) >> memory; !res)
                return res;

            const VkBindImageMemoryInfo bind_info
            {
                .sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
                .image = image,
                .memory = memory,
                .memoryOffset = 0,
            };

            if (auto res = vk::BindImageMemory2(m_Device, bind_info); !res)
                return res;
        }
    }

    reference.Views.resize(reference.Images.size());

    for (uint32_t i = 0; i < reference.Images.size(); ++i)
    {
        const VkImageViewCreateInfo view_create_info
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = reference.Images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = create_info.imageFormat,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = create_info.aspectMask,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        if (auto res = vk::ImageView::create(m_Device, view_create_info) >> reference.Views[i]; !res)
            return res;
    }

    return reference;
}
