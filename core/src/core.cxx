#include <map>
#include <string_view>

#include <core.hxx>
#include <extension.hxx>
#include <log.hxx>
#include <result.hxx>
#include <format/xr.hxx>

core::result<> core::Instance::Initialize(const std::string_view exec, const std::vector<std::string_view> &args)
{
    (void) exec;
    (void) args;

    TRY(InitializeWindow());
    TRY(InitializeAudio());
    TRY(InitializeGraphics());

    return ok();
}

void core::Instance::Terminate()
{
    m_GlfwWindow.Close();
}

core::result<> core::Instance::Spin()
{
    glfw::PollEvents();

    if (m_GlfwWindow.ShouldClose())
        return error("window should close");

    TRY(PollEvents());
    TRY(RenderFrame());

    return ok();
}

static void glfw_error_callback(const int code, const char *description)
{
    std::cerr << std::format("[GLFW:{}] {}", code, description) << std::endl;
}

core::result<> core::Instance::InitializeWindow()
{
    TRY(glfw::Library::Create() >> m_GlfwLibrary);

    glfwSetErrorCallback(glfw_error_callback);

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    glfw::Monitor monitor;
    TRY(glfw::Monitor::GetPrimary() >> monitor);

    const auto mode = monitor.GetVideoMode();

    TRY(glfw::Window::Create(mode->width, mode->height, "Titan Game", monitor) >> m_GlfwWindow);

    m_GlfwWindow.SetUserPointer(this);
    m_GlfwWindow.Show();

    return ok();
}

static void get_instance_extensions(std::vector<const char *> &dst)
{
    static constexpr std::array EXTENSIONS
    {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    };

    dst.insert(dst.end(), EXTENSIONS.begin(), EXTENSIONS.end());

    std::uint32_t glfw_extension_count;
    const auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    dst.insert(dst.end(), glfw_extensions, glfw_extensions + glfw_extension_count);
}

static void get_device_extensions(std::vector<const char *> &dst)
{
    static constexpr std::array EXTENSIONS
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    dst.insert(dst.end(), EXTENSIONS.begin(), EXTENSIONS.end());
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    const VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data)
{
    const auto instance = static_cast<core::Instance *>(user_data);
    (void) instance;

    std::cerr << std::format(
        "[{}:{}] {}",
        message_severity,
        static_cast<VkDebugUtilsMessageTypeFlagBitsEXT>(message_type),
        callback_data->pMessage) << std::endl;

    return VK_FALSE;
}

core::result<> core::Instance::InitializeGraphics()
{
    TRY(CreateXrInstance());
    TRY(CreateXrMessenger());
    TRY(GetSystemId());
    TRY(CreateVkInstance());
    TRY(CreateVkMessenger());
    TRY(GetPhysicalDevice());
    TRY(CreateDevice());
    TRY(GetQueueFamilyIndex());
    TRY(GetQueueIndex());
    TRY(CreateSession());
    TRY(GetViewConfigurationType());
    TRY(GetViewConfigurationViews());
    TRY(GetFormats());
    TRY(CreateSwapchains());
    TRY(GetEnvironmentBlendMode());
    TRY(CreateReferenceSpace());
    TRY(CreateSurface());

    return ok();
}

core::result<> core::Instance::InitializeAudio()
{
    TRY(al::Device::Open() >> m_AlDevice);
    TRY(al::Context::Create(m_AlDevice) >> m_AlContext);

    m_AlContext.MakeCurrent();

    return ok();
}

core::result<> core::Instance::CreateVkInstance()
{
    static constexpr std::array layers
    {
        "VK_LAYER_KHRONOS_validation",
    };

    std::vector<const char *> extensions;
    get_instance_extensions(extensions);

    XrGraphicsRequirementsVulkan2KHR graphics_requirements
    {
        .type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR,
        .next = nullptr,
        .minApiVersionSupported = {},
        .maxApiVersionSupported = {},
    };
    if (auto res = xrGetVulkanGraphicsRequirements2KHR(m_XrInstance, m_SystemId, &graphics_requirements))
        return error("xrGetVulkanGraphicsRequirements2KHR => {}", res);

    const VkApplicationInfo application_info
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "Titan Game",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
        .pEngineName = "Titan Core",
        .engineVersion = VK_MAKE_VERSION(0, 0, 1),
        .apiVersion = VK_API_VERSION_1_1,
    };

    const VkInstanceCreateInfo instance_create_info
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &application_info,
        .enabledLayerCount = static_cast<std::uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<std::uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

    const XrVulkanInstanceCreateInfoKHR vk_instance_create_info
    {
        .type = XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR,
        .next = nullptr,
        .systemId = m_SystemId,
        .createFlags = 0,
        .pfnGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vulkanCreateInfo = &instance_create_info,
        .vulkanAllocator = nullptr,
    };

    VkResult vk_result;
    VkInstance vk_instance;

    if (auto res = xrCreateVulkanInstanceKHR(m_XrInstance, &vk_instance_create_info, &vk_instance, &vk_result);
        res || vk_result)
        return error("xrCreateVulkanInstanceKHR => {}, {}", res, vk_result);

    m_VkInstance = vk::Instance::wrap(vk_instance);
    return ok();
}

core::result<> core::Instance::CreateVkMessenger()
{
    const VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = nullptr,
        .flags = 0,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                           | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                           | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                           | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                       | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                       | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = vk_debug_callback,
        .pUserData = this,
    };

    return vk::DebugUtilsMessengerEXT::create(m_VkInstance, debug_utils_messenger_create_info) >> m_VkMessenger;
}

core::result<> core::Instance::GetPhysicalDevice()
{
    const XrVulkanGraphicsDeviceGetInfoKHR get_info
    {
        .type = XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR,
        .next = nullptr,
        .systemId = m_SystemId,
        .vulkanInstance = m_VkInstance,
    };

    VkPhysicalDevice physical_device;
    if (auto res = xrGetVulkanGraphicsDevice2KHR(m_XrInstance, &get_info, &physical_device))
        return error("xrGetVulkanGraphicsDevice2KHR => {}", res);

    m_PhysicalDevice = physical_device;

    return ok();
}

core::result<> core::Instance::CreateDevice()
{
    std::vector<const char *> extensions;
    get_device_extensions(extensions);

    auto queue_priority = 1.0f;
    const VkDeviceQueueCreateInfo queue_create_info
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = 0,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };

    const VkDeviceCreateInfo device_create_info
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_create_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<std::uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
        .pEnabledFeatures = nullptr,
    };

    const XrVulkanDeviceCreateInfoKHR vk_device_create_info
    {
        .type = XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR,
        .next = nullptr,
        .systemId = m_SystemId,
        .createFlags = 0,
        .pfnGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vulkanPhysicalDevice = m_PhysicalDevice,
        .vulkanCreateInfo = &device_create_info,
        .vulkanAllocator = nullptr,
    };

    VkDevice vk_device;
    VkResult vk_result;

    if (auto res = xrCreateVulkanDeviceKHR(m_XrInstance, &vk_device_create_info, &vk_device, &vk_result);
        res || vk_result)
        return error("xrCreateVulkanDeviceKHR => {}, {}", res, vk_result);

    m_Device = vk::Device::wrap(vk_device);

    return ok();
}

core::result<> core::Instance::GetQueueFamilyIndex()
{
    auto queue_family_properties = vk::GetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice);

    m_QueueFamilyIndex = UINT32_MAX;

    for (std::uint32_t i = 0; i < queue_family_properties.size(); ++i)
    {
        if (auto &[flags, count, _3, _4] = queue_family_properties[i];
            flags & VK_QUEUE_GRAPHICS_BIT && flags & VK_QUEUE_COMPUTE_BIT && count > 0)
        {
            m_QueueFamilyIndex = i;
            break;
        }
    }

    if (m_QueueFamilyIndex == UINT32_MAX)
        return error("failed to find any suitable queue family.");

    return ok();
}

core::result<> core::Instance::GetQueueIndex()
{
    m_QueueIndex = 0;

    return ok();
}

core::result<> core::Instance::CreateSurface()
{
    return vk::GLFWSurface::create(m_VkInstance, m_GlfwWindow) >> m_Surface;
}

static XRAPI_ATTR XrBool32 XRAPI_CALL xr_debug_callback(
    const XrDebugUtilsMessageSeverityFlagsEXT message_severity,
    const XrDebugUtilsMessageTypeFlagsEXT message_types,
    const XrDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data)
{
    const auto instance = static_cast<core::Instance *>(user_data);
    (void) instance;

    static const std::map<XrDebugUtilsMessageSeverityFlagsEXT, const char *> message_severity_map
    {
        { XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT, "VERBOSE" },
        { XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT, "INFO" },
        { XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT, "WARNING" },
        { XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT, "ERROR" },
    };
    static const std::map<XrDebugUtilsMessageTypeFlagsEXT, const char *> message_type_map
    {
        { XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT, "GENERAL" },
        { XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT, "VALIDATION" },
        { XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT, "PERFORMANCE" },
        { XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT, "CONFORMANCE" },
    };

    std::cerr << std::format(
        "[{}:{}] {}",
        message_severity_map.at(message_severity),
        message_type_map.at(message_types),
        callback_data->message) << std::endl;

    return XR_FALSE;
}

core::result<> core::Instance::CreateXrInstance()
{
    static constexpr std::array EXTENSIONS
    {
        XR_EXT_DEBUG_UTILS_EXTENSION_NAME,
        XR_KHR_VULKAN_ENABLE_EXTENSION_NAME,
        XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME,
    };

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
        .enabledExtensionCount = EXTENSIONS.size(),
        .enabledExtensionNames = EXTENSIONS.data(),
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
        .userCallback = xr_debug_callback,
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
                   for (const auto allowed_view_configuration_type : VIEW_CONFIGURATION_TYPES)
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

struct FormatReference
{
    std::string_view name;
    VkFormat &format;
    VkFormatFeatureFlags features;
};

static core::result<> find_formats(
    VkPhysicalDevice physical_device,
    const std::vector<std::int64_t> &formats,
    const std::vector<FormatReference> &references)
{
    std::map<VkFormat, VkFormatProperties> properties;

    for (const auto format : formats)
        vkGetPhysicalDeviceFormatProperties(
            physical_device,
            static_cast<VkFormat>(format),
            &properties[static_cast<VkFormat>(format)]);

    auto count = 0;

    for (const auto format : formats)
        for (auto &reference : references)
            if (!reference.format
                && properties[static_cast<VkFormat>(format)].optimalTilingFeatures & reference.features)
            {
                reference.format = static_cast<VkFormat>(format);
                count++;
            }

    if (count >= references.size())
        return core::ok();

    for (const auto format : formats)
        for (auto &reference : references)
            if (!reference.format
                && properties[static_cast<VkFormat>(format)].linearTilingFeatures & reference.features)
            {
                reference.format = static_cast<VkFormat>(format);
                count++;
            }

    if (count >= references.size())
        return core::ok();

    std::vector<std::string_view> missing;
    for (auto &reference : references)
        if (!reference.format)
            missing.push_back(reference.name);

    return core::error("failed to find formats for references {}.", missing);
}

core::result<> core::Instance::GetFormats()
{
    return xr::EnumerateSwapchainFormats(m_Session)
           | [&](const auto &value)
           {
               return find_formats(
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
    m_ColorSwapchainReferences.resize(m_ViewConfigurationViews.size());
    m_DepthSwapchainReferences.resize(m_ViewConfigurationViews.size());

    for (std::uint32_t i = 0; i < m_ViewConfigurationViews.size(); ++i)
    {
        TRY(
            CreateSwapchain(
                m_ViewConfigurationViews[i],
                XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
                m_ColorFormat,
                VK_IMAGE_ASPECT_COLOR_BIT) >> m_ColorSwapchainReferences[i]
        );

        TRY(
            CreateSwapchain(
                m_ViewConfigurationViews[i],
                XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                m_DepthFormat,
                VK_IMAGE_ASPECT_DEPTH_BIT) >> m_DepthSwapchainReferences[i]
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
                   for (auto allowed_mode : ENVIRONMENT_BLEND_MODES)
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
        .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL,
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
    const XrViewConfigurationView &view_configuration_view,
    const XrSwapchainUsageFlags usage_flags,
    const VkFormat format,
    const VkImageAspectFlags aspect_mask)
{
    SwapchainReference reference;

    const XrSwapchainCreateInfo swapchain_create_info
    {
        .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
        .next = nullptr,
        .createFlags = 0,
        .usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | usage_flags,
        .format = format,
        .sampleCount = view_configuration_view.recommendedSwapchainSampleCount,
        .width = view_configuration_view.recommendedImageRectWidth,
        .height = view_configuration_view.recommendedImageRectHeight,
        .faceCount = 1,
        .arraySize = 1,
        .mipCount = 1,
    };

    TRY_CAST(
        xr::Swapchain::create(m_Session, swapchain_create_info) >> reference.swapchain,
        SwapchainReference
    );

    auto set_images = [&](const auto &value)
    {
        reference.images.resize(value.size());

        for (std::uint32_t i = 0; i < value.size(); ++i)
            reference.images[i] = vk::Image::wrap(value[i].image);

        return ok();
    };

    TRY_CAST(
        xr::EnumerateSwapchainImages<XrSwapchainImageVulkanKHR>(reference.swapchain) | set_images,
        SwapchainReference
    );

    reference.views.resize(reference.images.size());

    for (std::uint32_t i = 0; i < reference.images.size(); ++i)
    {
        const VkImageViewCreateInfo image_view_create_info
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = reference.images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = aspect_mask,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        TRY_CAST(
            vk::ImageView::create(m_Device, image_view_create_info) >> reference.views[i],
            SwapchainReference
        );
    }

    return reference;
}

core::result<> core::Instance::PollEvents()
{
    XrEventDataBuffer event_data
    {
        .type = XR_TYPE_EVENT_DATA_BUFFER,
        .next = nullptr,
        .varying = {},
    };

    while (xrPollEvent(m_XrInstance, &event_data) == XR_SUCCESS)
    {
        switch (event_data.type)
        {
        case XR_TYPE_EVENT_DATA_EVENTS_LOST:
        {
            const auto events_lost = reinterpret_cast<XrEventDataEventsLost *>(&event_data);
            info("XrEventDataEventsLost {{ lostEventCount={} }}", events_lost->lostEventCount);
            break;
        }

        case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
        {
            const auto instance_loss_pending = reinterpret_cast<XrEventDataInstanceLossPending *>(&event_data);
            info("XrEventDataInstanceLossPending {{ lossTime={} }}", instance_loss_pending->lossTime);

            return error("XrEventDataInstanceLossPending {{ lossTime={} }}", instance_loss_pending->lossTime);
        }

        case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
        {
            const auto interaction_profile_changed = reinterpret_cast<XrEventDataInteractionProfileChanged *>(
                &event_data);
            info(
                "XrEventDataInteractionProfileChanged {{ session={} }}",
                static_cast<void *>(interaction_profile_changed->session));

            if (interaction_profile_changed->session != m_Session)
            {
                info("XrEventDataInteractionProfileChanged for foreign session!");
                break;
            }

            break;
        }

        case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
        {
            const auto reference_space_change_pending = reinterpret_cast<XrEventDataReferenceSpaceChangePending *>(
                &event_data);
            info(
                "XrEventDataReferenceSpaceChangePending {{ session={}, referenceSpaceType={}, changeTime={}, poseValid={} }}",
                static_cast<void *>(reference_space_change_pending->session),
                reference_space_change_pending->referenceSpaceType,
                reference_space_change_pending->changeTime,
                reference_space_change_pending->poseValid);

            if (reference_space_change_pending->session != m_Session)
            {
                info("XrEventDataReferenceSpaceChangePending for foreign session!");
                break;
            }

            break;
        }

        case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
        {
            const auto session_state_changed = reinterpret_cast<XrEventDataSessionStateChanged *>(&event_data);
            info(
                "XrEventDataSessionStateChanged {{ session={}, state={}, time={} }}",
                static_cast<void *>(session_state_changed->session),
                session_state_changed->state,
                session_state_changed->time);

            if (session_state_changed->session != m_Session)
            {
                info("XrEventDataSessionStateChanged for foreign session!");
                break;
            }

            m_SessionState = session_state_changed->state;

            switch (session_state_changed->state)
            {
            case XR_SESSION_STATE_READY:
            {
                const XrSessionBeginInfo session_begin_info
                {
                    .type = XR_TYPE_SESSION_BEGIN_INFO,
                    .next = nullptr,
                    .primaryViewConfigurationType = m_ViewConfigurationType,
                };

                if (auto res = xrBeginSession(m_Session, &session_begin_info))
                    return error("xrBeginSession => {}", res);

                break;
            }

            case XR_SESSION_STATE_STOPPING:
            {
                if (auto res = xrEndSession(m_Session))
                    return error("xrEndSession => {}", res);

                return error(
                    "XrEventDataSessionStateChanged {{ session={}, state={}, time={} }}",
                    static_cast<void *>(session_state_changed->session),
                    session_state_changed->state,
                    session_state_changed->time);
            }

            case XR_SESSION_STATE_LOSS_PENDING:
            case XR_SESSION_STATE_EXITING:
            {
                return error(
                    "XrEventDataSessionStateChanged {{ session={}, state={}, time={} }}",
                    static_cast<void *>(session_state_changed->session),
                    session_state_changed->state,
                    session_state_changed->time);
            }

            default:
                break;
            }

            break;
        }

        default:
            break;
        }

        event_data.type = XR_TYPE_EVENT_DATA_BUFFER;
    }

    return ok();
}

core::result<> core::Instance::RenderFrame()
{
    const XrFrameWaitInfo frame_wait_info
    {
        .type = XR_TYPE_FRAME_WAIT_INFO,
        .next = nullptr,
    };

    XrFrameState frame_state
    {
        .type = XR_TYPE_FRAME_STATE,
        .next = nullptr,
        .predictedDisplayTime = {},
        .predictedDisplayPeriod = {},
        .shouldRender = {},
    };

    if (auto res = xrWaitFrame(m_Session, &frame_wait_info, &frame_state))
        return error("xrWaitFrame => {}", res);

    const XrFrameBeginInfo frame_begin_info
    {
        .type = XR_TYPE_FRAME_BEGIN_INFO,
        .next = nullptr,
    };

    if (auto res = xrBeginFrame(m_Session, &frame_begin_info))
        return error("xrBeginFrame => {}", res);

    LayerReference render_layer_reference
    {
        .predicted_display_time = frame_state.predictedDisplayTime,
    };

    const auto session_active = m_SessionState == XR_SESSION_STATE_SYNCHRONIZED
                                || m_SessionState == XR_SESSION_STATE_VISIBLE
                                || m_SessionState == XR_SESSION_STATE_FOCUSED;

    if (session_active && frame_state.shouldRender)
    {
        TRY(RenderLayer(render_layer_reference));

        render_layer_reference.layers.push_back(
            reinterpret_cast<XrCompositionLayerBaseHeader *>(&render_layer_reference.projection));
    }

    const XrFrameEndInfo frame_end_info
    {
        .type = XR_TYPE_FRAME_END_INFO,
        .next = nullptr,
        .displayTime = frame_state.predictedDisplayTime,
        .environmentBlendMode = m_EnvironmentBlendMode,
        .layerCount = static_cast<std::uint32_t>(render_layer_reference.layers.size()),
        .layers = render_layer_reference.layers.data(),
    };

    if (auto res = xrEndFrame(m_Session, &frame_end_info))
        return error("xrEndFrame => {}", res);

    return ok();
}

core::result<> core::Instance::RenderLayer(LayerReference &reference)
{
    std::vector<XrView> views(m_ViewConfigurationViews.size(), { .type = XR_TYPE_VIEW });

    XrViewState view_state
    {
        .type = XR_TYPE_VIEW_STATE
    };

    XrViewLocateInfo view_locate_info
    {
        .type = XR_TYPE_VIEW_LOCATE_INFO,
        .next = nullptr,
        .viewConfigurationType = m_ViewConfigurationType,
        .displayTime = reference.predicted_display_time,
        .space = m_ReferenceSpace,
    };

    std::uint32_t view_count{};

    if (auto res = xrLocateViews(
        m_Session,
        &view_locate_info,
        &view_state,
        views.size(),
        &view_count,
        views.data()))
        return error("xrLocateViews => {}", res);

    reference.projection_views.resize(view_count, { .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW });

    for (std::uint32_t i = 0; i < view_count; ++i)
    {
        auto &color = m_ColorSwapchainReferences[i];
        auto &depth = m_DepthSwapchainReferences[i];

        std::uint32_t color_index{}, depth_index{};

        const XrSwapchainImageAcquireInfo swapchain_image_acquire_info
        {
            .type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO,
            .next = nullptr,
        };

        if (auto res = xrAcquireSwapchainImage(color.swapchain, &swapchain_image_acquire_info, &color_index))
            return error("xrAcquireSwapchainImage => {}", res);
        if (auto res = xrAcquireSwapchainImage(depth.swapchain, &swapchain_image_acquire_info, &depth_index))
            return error("xrAcquireSwapchainImage => {}", res);

        const XrSwapchainImageWaitInfo swapchain_image_wait_info
        {
            .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
            .next = nullptr,
            .timeout = XR_INFINITE_DURATION,
        };

        if (auto res = xrWaitSwapchainImage(color.swapchain, &swapchain_image_wait_info))
            return error("xrWaitSwapchainImage => {}", res);
        if (auto res = xrWaitSwapchainImage(depth.swapchain, &swapchain_image_wait_info))
            return error("xrWaitSwapchainImage => {}", res);

        const auto width = m_ViewConfigurationViews[i].recommendedImageRectWidth;
        const auto height = m_ViewConfigurationViews[i].recommendedImageRectHeight;

        const VkViewport viewport
        {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(width),
            .height = static_cast<float>(height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };

        const VkRect2D scissor
        {
            .offset = {
                .x = 0,
                .y = 0,
            },
            .extent = {
                .width = width,
                .height = height,
            },
        };

        const auto near = 0.01f, far = 100.0f;

        reference.projection_views[i] = {
            .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
            .next = nullptr,
            .pose = views[i].pose,
            .fov = views[i].fov,
            .subImage = {
                .swapchain = color.swapchain,
                .imageRect = {
                    .offset = {
                        .x = 0,
                        .y = 0,
                    },
                    .extent = {
                        .width = static_cast<std::int32_t>(width),
                        .height = static_cast<std::int32_t>(height),
                    },
                },
                .imageArrayIndex = 0,
            },
        };

        // TODO: begin rendering
        // TODO: if environment blend mode is opaque, then clear color for color image view to something
        // TODO: else clear color for color image view to black
        // TODO: clear depth for depth image view to 1
        // TODO: end rendering

        const XrSwapchainImageReleaseInfo swapchain_image_release_info
        {
            .type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO,
            .next = nullptr,
        };

        if (auto res = xrReleaseSwapchainImage(color.swapchain, &swapchain_image_release_info))
            return error("xrReleaseSwapchainImage => {}", res);
        if (auto res = xrReleaseSwapchainImage(depth.swapchain, &swapchain_image_release_info))
            return error("xrReleaseSwapchainImage => {}", res);
    }

    reference.projection = {
        .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
        .next = nullptr,
        .layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT |
                      XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT,
        .space = m_ReferenceSpace,
        .viewCount = static_cast<std::uint32_t>(reference.projection_views.size()),
        .views = reference.projection_views.data(),
    };

    return ok();
}
