#include <titan/core.hxx>
#include <titan/utils.hxx>

core::result<core::VkSwapchainReference> core::Application::CreateSwapchainReference(
    const VkSwapchainReferenceCreateInfo &create_info)
{
    VkSwapchainReference reference
    {
        .Format = create_info.imageFormat,
    };

    if (create_info.useSwapchain)
    {
        auto set_images = [this, &reference](const std::vector<VkImage> &images)
        {
            reference.Images.resize(images.size());
            for (uint32_t i = 0; i < images.size(); ++i)
                reference.Images[i] = vk::Image::wrap(images[i]);
            return ok();
        };

        const VkSwapchainCreateInfoKHR swapchain_create_info
        {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = m_WindowSurface,
            .minImageCount = create_info.imageCount,
            .imageFormat = create_info.imageFormat,
            .imageColorSpace = create_info.imageColorSpace,
            .imageExtent = create_info.imageExtent,
            .imageArrayLayers = 1,
            .imageUsage = create_info.imageUsage,
            .imageSharingMode = create_info.imageSharingMode,
            .queueFamilyIndexCount = create_info.queueFamilyIndexCount,
            .pQueueFamilyIndices = create_info.pQueueFamilyIndices,
            .preTransform = create_info.preTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = create_info.presentMode,
            .clipped = true,
        };

        if (auto res = vk::SwapchainKHR::create(m_Device, swapchain_create_info) >> reference.Swapchain)
            return error<VkSwapchainReference>(std::move(res));

        if (auto res = vk::GetSwapchainImagesKHR(m_Device, reference.Swapchain) & set_images)
            return error<VkSwapchainReference>(std::move(res));
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

            if (auto res = vk::Image::create(m_Device, image_create_info) >> image)
                return error<VkSwapchainReference>(std::move(res));

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
                           >> memory_type_index)
                return error<VkSwapchainReference>(std::move(res));

            const VkMemoryAllocateInfo allocate_info
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = memory_requirements.size,
                .memoryTypeIndex = memory_type_index,
            };

            if (auto res = vk::DeviceMemory::create(m_Device, allocate_info) >> memory)
                return error<VkSwapchainReference>(std::move(res));

            const VkBindImageMemoryInfo bind_info
            {
                .sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
                .image = image,
                .memory = memory,
                .memoryOffset = 0,
            };

            if (auto res = vk::BindImageMemory2(m_Device, bind_info))
                return error<VkSwapchainReference>(std::move(res));
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

        if (auto res = vk::ImageView::create(m_Device, view_create_info) >> reference.Views[i])
            return error<VkSwapchainReference>(std::move(res));
    }

    return reference;
}
