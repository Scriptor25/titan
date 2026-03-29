#include <titan/core.hxx>
#include <titan/log.hxx>
#include <titan/format/xr.hxx>

core::result<> core::Instance::CreateXrInstance()
{
    const XrApplicationInfo application_info
    {
        .applicationName = "Titan Game",
        .applicationVersion = XR_MAKE_VERSION(0, 0, 1),
        .engineName = "Titan Core",
        .engineVersion = XR_MAKE_VERSION(0, 0, 1),
        .apiVersion = XR_API_VERSION_1_1,
    };

    const XrInstanceCreateInfo instance_create_info
    {
        .type = XR_TYPE_INSTANCE_CREATE_INFO,
        .next = nullptr,
        .createFlags = 0,
        .applicationInfo = application_info,
        .enabledApiLayerCount = 0,
        .enabledApiLayerNames = nullptr,
        .enabledExtensionCount = XR_INSTANCE_EXTENSIONS.size(),
        .enabledExtensionNames = XR_INSTANCE_EXTENSIONS.data(),
    };

    return xr::Instance::create(instance_create_info) >> m_XrInstance;
}

core::result<> core::Instance::CreateXrMessenger()
{
    const XrDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info
    {
        .type = XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .next = nullptr,
        .messageSeverities = XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                             | XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                             | XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                             | XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageTypes = XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                        | XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                        | XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
                        | XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT,
        .userCallback = XrDebugCallback,
        .userData = this,
    };

    return xr::DebugUtilsMessengerEXT::create(m_XrInstance, debug_utils_messenger_create_info) >> m_XrMessenger;
}

core::result<> core::Instance::GetSystemId()
{
    const XrSystemGetInfo system_get_info
    {
        .type = XR_TYPE_SYSTEM_GET_INFO,
        .next = nullptr,
        .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY,
    };

    return xr::SystemId::create(m_XrInstance, system_get_info) >> m_SystemId;
}

core::result<> core::Instance::CreateSession()
{
    const XrGraphicsBindingVulkanKHR graphics_binding_vulkan
    {
        .type = XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR,
        .next = nullptr,
        .instance = m_VkInstance,
        .physicalDevice = m_PhysicalDevice,
        .device = m_Device,
        .queueFamilyIndex = m_QueueFamilyIndex,
        .queueIndex = m_QueueIndex,
    };

    const XrSessionCreateInfo session_create_info
    {
        .type = XR_TYPE_SESSION_CREATE_INFO,
        .next = &graphics_binding_vulkan,
        .createFlags = 0,
        .systemId = m_SystemId,
    };

    return xr::Session::create(m_XrInstance, session_create_info) >> m_Session;
}

core::result<> core::Instance::GetViewConfigurationType()
{
    return xr::EnumerateViewConfigurationTypes(m_XrInstance, m_SystemId)
           | [&](const auto &view_configuration_types)
           {
               m_ViewConfigurationType = {};

               for (const auto view_configuration_type : view_configuration_types)
                   for (const auto allowed_view_configuration_type : XR_VIEW_CONFIGURATION_TYPES)
                       if (view_configuration_type == allowed_view_configuration_type)
                       {
                           m_ViewConfigurationType = view_configuration_type;
                           break;
                       }

               if (!m_ViewConfigurationType)
                   return error("failed to find any suitable view configuration type.");

               return ok();
           };
}

core::result<> core::Instance::GetViewConfigurationViews()
{
    return xr::EnumerateViewConfigurationViews(
               m_XrInstance,
               m_SystemId,
               m_ViewConfigurationType,
               { .type = XR_TYPE_VIEW_CONFIGURATION_VIEW }) >> m_ViewConfigurationViews;
}

core::result<> core::Instance::GetFormats()
{
    return xr::EnumerateSwapchainFormats(m_Session)
           | [&](const auto &value)
           {
               return FindFormats(
                   m_PhysicalDevice,
                   value,
                   {
                       { "color", m_ColorFormat, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT },
                       { "depth", m_DepthFormat, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT },
                   });
           };
}

core::result<> core::Instance::CreateSwapchains()
{
    m_SwapchainFrames.resize(m_ViewConfigurationViews.size());

    for (uint32_t i = 0; i < m_ViewConfigurationViews.size(); ++i)
    {
        TRY(
            CreateSwapchain(
                m_ViewConfigurationViews[i],
                XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
                m_ColorFormat,
                VK_IMAGE_ASPECT_COLOR_BIT) >> m_SwapchainFrames[i].Color
        );

        TRY(
            CreateSwapchain(
                m_ViewConfigurationViews[i],
                XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                m_DepthFormat,
                VK_IMAGE_ASPECT_DEPTH_BIT) >> m_SwapchainFrames[i].Depth
        );
    }

    return ok();
}

core::result<> core::Instance::GetEnvironmentBlendMode()
{
    return xr::EnumerateEnvironmentBlendModes(m_XrInstance, m_SystemId, m_ViewConfigurationType)
           | [&](const auto &value)
           {
               m_EnvironmentBlendMode = {};

               for (auto mode : value)
                   for (auto allowed_mode : XR_ENVIRONMENT_BLEND_MODES)
                       if (mode == allowed_mode)
                       {
                           m_EnvironmentBlendMode = mode;
                           break;
                       }

               if (!m_EnvironmentBlendMode)
               {
                   info("failed to find any suitable environment blend mode.");
                   m_EnvironmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
               }

               return ok();
           };
}

core::result<> core::Instance::CreateReferenceSpace()
{
    const XrReferenceSpaceCreateInfo reference_space_create_info
    {
        .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
        .next = nullptr,
        .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE,
        .poseInReferenceSpace = {
            .orientation = {
                .x = 0.0f,
                .y = 0.0f,
                .z = 0.0f,
                .w = 1.0f,
            },
            .position = {
                .x = 0.0f,
                .y = 0.0f,
                .z = 0.0f,
            },
        },
    };

    return xr::ReferenceSpace::create(m_Session, reference_space_create_info)
           | [&](auto value)
           {
               m_ReferenceSpace = std::move(value);
               return ok();
           };
}

core::result<core::SwapchainReference> core::Instance::CreateSwapchain(
    const XrViewConfigurationView &view,
    const XrSwapchainUsageFlags usage,
    const VkFormat format,
    const VkImageAspectFlags aspect)
{
    SwapchainReference reference
    {
        .Format = format,
        .Swapchain = {},
        .Images = {},
        .Views = {},
    };

    const XrSwapchainCreateInfo swapchain_create_info
    {
        .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
        .next = nullptr,
        .createFlags = 0,
        .usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | usage,
        .format = format,
        .sampleCount = view.recommendedSwapchainSampleCount,
        .width = view.recommendedImageRectWidth,
        .height = view.recommendedImageRectHeight,
        .faceCount = 1,
        .arraySize = 1,
        .mipCount = 1,
    };

    TRY_CAST(
        xr::Swapchain::create(m_Session, swapchain_create_info) >> reference.Swapchain,
        SwapchainReference
    );

    auto set_images = [&](const auto &value)
    {
        reference.Images.resize(value.size());

        for (uint32_t i = 0; i < value.size(); ++i)
            reference.Images[i] = vk::Image::wrap(value[i].image);

        return ok();
    };

    TRY_CAST(
        xr::EnumerateSwapchainImages<XrSwapchainImageVulkanKHR>(reference.Swapchain) | set_images,
        SwapchainReference
    );

    reference.Views.resize(reference.Images.size());

    for (uint32_t i = 0; i < reference.Images.size(); ++i)
    {
        const VkImageViewCreateInfo image_view_create_info
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = reference.Images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = aspect,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        TRY_CAST(
            vk::ImageView::create(m_Device, image_view_create_info) >> reference.Views[i],
            SwapchainReference
        );
    }

    return reference;
}
