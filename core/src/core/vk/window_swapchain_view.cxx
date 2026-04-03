#include <titan/core.hxx>

static const VkSurfaceFormatKHR &find_surface_format(
    const std::vector<VkSurfaceFormatKHR> &formats,
    const VkFormat request)
{
    for (auto &format : formats)
        if (format.format == request && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    return formats[0];
}

static VkPresentModeKHR select_present_mode(const std::vector<VkPresentModeKHR> &modes)
{
    for (auto &mode : modes)
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return mode;
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D select_extent(const VkSurfaceCapabilitiesKHR &capabilities, const core::glfw::Window &window)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()
        && capabilities.currentExtent.height != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    int width, height;
    window.GetFramebufferSize(width, height);

    VkExtent2D extent
    {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
    };

    extent.width = std::clamp(
        extent.width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width);
    extent.height = std::clamp(
        extent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height);

    return extent;
}

core::result<> core::Application::CreateWindowSwapchainView()
{
    const VkPhysicalDeviceSurfaceInfo2KHR surface_info
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
        .surface = m_WindowSurface,
    };

    VkSurfaceCapabilities2KHR surface_capabilities
    {
        .sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR,
    };
    if (auto res = vkGetPhysicalDeviceSurfaceCapabilities2KHR(m_PhysicalDevice, &surface_info, &surface_capabilities))
        return error("vkGetPhysicalDeviceSurfaceCapabilitiesKHR => {}", res);

    auto &capabilities = surface_capabilities.surfaceCapabilities;

    std::vector<VkSurfaceFormatKHR> surface_formats;
    auto set_surface_formats = [&surface_formats](std::vector<VkSurfaceFormat2KHR> &&formats)
    {
        surface_formats.resize(formats.size());
        for (uint32_t i = 0; i < formats.size(); ++i)
            surface_formats[i] = formats[i].surfaceFormat;
        return ok();
    };

    TRY(vk::GetPhysicalDeviceSurfaceFormats2KHR(m_PhysicalDevice, surface_info) | set_surface_formats);

    std::vector<VkPresentModeKHR> present_modes;
    TRY(vk::GetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_WindowSurface) >> present_modes);

    const auto &[color_format, color_space] = find_surface_format(surface_formats, m_ColorFormat);
    const auto presentMode = select_present_mode(present_modes);
    const auto image_extent = select_extent(capabilities, m_Window);

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

    auto image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount)
        image_count = capabilities.maxImageCount;

    m_WindowSwapchainView = {
        .Width = image_extent.width,
        .Height = image_extent.height,
    };

    {
        const VkSwapchainReferenceCreateInfo create_info
        {
            .useSwapchain = true,
            .imageCount = image_count,
            .imageExtent = image_extent,
            .imageFormat = color_format,
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageColorSpace = color_space,
            .imageSharingMode = image_sharing_mode,
            .presentMode = presentMode,
            .preTransform = capabilities.currentTransform,
            .queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size()),
            .pQueueFamilyIndices = queue_family_indices.data(),
        };

        TRY(CreateSwapchainReference(create_info) >> m_WindowSwapchainView.Color);
    }

    {
        const VkSwapchainReferenceCreateInfo create_info
        {
            .useSwapchain = false,
            .imageCount = image_count,
            .imageExtent = image_extent,
            .imageFormat = m_DepthFormat,
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .imageSharingMode = image_sharing_mode,
            .queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size()),
            .pQueueFamilyIndices = queue_family_indices.data(),
        };

        TRY(CreateSwapchainReference(create_info) >> m_WindowSwapchainView.Depth);
    }

    return ok();
}
