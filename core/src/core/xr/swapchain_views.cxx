#include <titan/core.hxx>

core::result<> core::Application::CreateSwapchainViews()
{
    VkSharingMode image_sharing_mode;
    std::vector<uint32_t> queue_family_indices;

    if (m_QueueFamilyIndices.Default != m_QueueFamilyIndices.Present)
    {
        image_sharing_mode = VK_SHARING_MODE_CONCURRENT;
        queue_family_indices = {
            m_QueueFamilyIndices.Default,
            m_QueueFamilyIndices.Present,
        };
    }
    else
    {
        image_sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    }

    for (auto &[
             view,
             color,
             depth,
             buffer,
             framebuffers
         ] : m_SwapchainViews)
    {
        {
            const XrSwapchainReferenceCreateInfo create_info
            {
                .useSwapchain = true,
                .sampleCount = view.recommendedSwapchainSampleCount,
                .usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
                .imageExtent = {
                    .width = view.recommendedImageRectWidth,
                    .height = view.recommendedImageRectHeight,
                },
                .imageFormat = m_ColorFormat,
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            };

            TRY(CreateSwapchainReference(create_info) >> color);
        }

        {
            const XrSwapchainReferenceCreateInfo create_info
            {
                .useSwapchain = false,
                .imageCount = static_cast<uint32_t>(color.Images.size()),
                .imageExtent = {
                    .width = view.recommendedImageRectWidth,
                    .height = view.recommendedImageRectHeight,
                },
                .imageFormat = m_DepthFormat,
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                .imageSharingMode = image_sharing_mode,
                .queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size()),
                .pQueueFamilyIndices = queue_family_indices.data(),
            };

            TRY(CreateSwapchainReference(create_info) >> depth);
        }
    }

    return ok();
}
