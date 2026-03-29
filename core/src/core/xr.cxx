#include <titan/core.hxx>
#include <titan/log.hxx>
#include <titan/format/xr.hxx>

#include <cstring>

core::result<> core::Application::CreateXrInstance()
{
    XrApplicationInfo application_info
    {
        .applicationName = {},
        .applicationVersion = static_cast<uint32_t>(XR_MAKE_VERSION(
            m_Info.Version.Major,
            m_Info.Version.Minor,
            m_Info.Version.Patch)),
        .engineName = "Titan Core",
        .engineVersion = XR_MAKE_VERSION(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH),
        .apiVersion = XR_CURRENT_API_VERSION,
    };

    memcpy(application_info.applicationName, m_Info.Name.data(), std::min(128LU, m_Info.Name.size()));

    const XrInstanceCreateInfo instance_create_info
    {
        .type = XR_TYPE_INSTANCE_CREATE_INFO,
        .applicationInfo = application_info,
        .enabledExtensionCount = XR_INSTANCE_EXTENSIONS.size(),
        .enabledExtensionNames = XR_INSTANCE_EXTENSIONS.data(),
    };

    return xr::Instance::create(instance_create_info) >> m_XrInstance;
}

core::result<> core::Application::CreateXrMessenger()
{
    const XrDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info
    {
        .type = XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
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

core::result<> core::Application::GetSystemId()
{
    const XrSystemGetInfo system_get_info
    {
        .type = XR_TYPE_SYSTEM_GET_INFO,
        .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY,
    };

    if (auto res = xrGetSystem(m_XrInstance, &system_get_info, &m_SystemId))
        return error("xrGetSystem => {}", res);

    return ok();
}

core::result<> core::Application::CreateSession()
{
    const XrGraphicsBindingVulkanKHR graphics_binding_vulkan
    {
        .type = XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR,
        .instance = m_VkInstance,
        .physicalDevice = m_PhysicalDevice,
        .device = m_Device,
        .queueFamilyIndex = m_QueueFamilyIndices.Default,
        .queueIndex = 0,
    };

    const XrSessionCreateInfo session_create_info
    {
        .type = XR_TYPE_SESSION_CREATE_INFO,
        .next = &graphics_binding_vulkan,
        .systemId = m_SystemId,
    };

    return xr::Session::create(m_XrInstance, session_create_info) >> m_Session;
}

core::result<> core::Application::GetViewConfigurationType()
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

core::result<> core::Application::GetViewConfigurationViews()
{
    return xr::EnumerateViewConfigurationViews(
               m_XrInstance,
               m_SystemId,
               m_ViewConfigurationType) >> m_ViewConfigurationViews;
}

core::result<> core::Application::GetFormats()
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

core::result<> core::Application::CreateSwapchains()
{
    m_SwapchainViews.resize(m_ViewConfigurationViews.size());

    for (uint32_t i = 0; i < m_ViewConfigurationViews.size(); ++i)
    {
        TRY(
            CreateSwapchain(
                m_ViewConfigurationViews[i],
                XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_SRC_BIT,
                m_ColorFormat,
                VK_IMAGE_ASPECT_COLOR_BIT) >> m_SwapchainViews[i].Color
        );

        TRY(
            CreateSwapchain(
                m_ViewConfigurationViews[i],
                XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                m_DepthFormat,
                VK_IMAGE_ASPECT_DEPTH_BIT) >> m_SwapchainViews[i].Depth
        );
    }

    return ok();
}

core::result<> core::Application::GetEnvironmentBlendMode()
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

core::result<> core::Application::CreateReferenceSpace()
{
    const XrReferenceSpaceCreateInfo reference_space_create_info
    {
        .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
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

core::result<core::XrSwapchainInfo> core::Application::CreateSwapchain(
    const XrViewConfigurationView &view,
    const XrSwapchainUsageFlags usage,
    const VkFormat format,
    const VkImageAspectFlags aspect)
{
    XrSwapchainInfo reference
    {
        .Format = format,
        .Swapchain = {},
        .Images = {},
        .Views = {},
    };

    const XrSwapchainCreateInfo swapchain_create_info
    {
        .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
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
        XrSwapchainInfo
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
        XrSwapchainInfo
    );

    reference.Views.resize(reference.Images.size());

    for (uint32_t i = 0; i < reference.Images.size(); ++i)
    {
        const VkImageViewCreateInfo image_view_create_info
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
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
            XrSwapchainInfo
        );
    }

    return reference;
}
