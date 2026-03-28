#include <map>
#include <string_view>

#include <core.hxx>
#include <cstring>
#include <extension.hxx>
#include <fstream>
#include <log.hxx>
#include <obj.hxx>
#include <result.hxx>
#include <format/xr.hxx>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

core::result<> core::Instance::Initialize(const std::string_view exec, const std::vector<std::string_view> &args)
{
    (void) exec;
    (void) args;

    m_Mesh = obj::Open("res/mesh/cube.obj");

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

    uint32_t glfw_extension_count;
    const auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    dst.insert(dst.end(), glfw_extensions, glfw_extensions + glfw_extension_count);
}

static void get_device_extensions(std::vector<const char *> &dst)
{
    static constexpr std::array EXTENSIONS
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
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
    TRY(CreateRenderPass());
    TRY(CreateFramebuffers());
    TRY(CreatePipelineCache());
    TRY(CreateDescriptorSetLayout());
    TRY(CreateDescriptorPool());
    TRY(CreateDescriptorSet());
    TRY(CreatePipelineLayout());
    TRY(CreatePipeline());
    TRY(CreateCommandPool());
    TRY(CreateCommandBuffer());
    TRY(CreateSynchronization());
    TRY(CreateVertexBuffer());
    TRY(CreateVertexMemory());
    TRY(FillVertexBuffer());
    TRY(CreateCameraBuffer());
    TRY(CreateCameraMemory());

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
        .apiVersion = VK_API_VERSION_1_3,
    };

    const VkInstanceCreateInfo instance_create_info
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &application_info,
        .enabledLayerCount = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
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

    const VkPhysicalDeviceFeatures physical_device_features
    {
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
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
        .pEnabledFeatures = &physical_device_features,
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

    for (uint32_t i = 0; i < queue_family_properties.size(); ++i)
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
    const std::vector<int64_t> &formats,
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

core::result<> core::Instance::CreateDescriptorSetLayout()
{
    const std::array bindings
    {
        VkDescriptorSetLayoutBinding
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr,
        },
    };

    const VkDescriptorSetLayoutCreateInfo create_info
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = bindings.size(),
        .pBindings = bindings.data(),
    };

    return vk::DescriptorSetLayout::create(m_Device, create_info) >> m_DescriptorSetLayout;
}

core::result<> core::Instance::CreateDescriptorPool()
{
    const std::array pool_sizes
    {
        VkDescriptorPoolSize
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
        },
    };

    const VkDescriptorPoolCreateInfo create_info
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = 1,
        .poolSizeCount = pool_sizes.size(),
        .pPoolSizes = pool_sizes.data(),
    };

    return vk::DescriptorPool::create(m_Device, create_info) >> m_DescriptorPool;
}

core::result<> core::Instance::CreateDescriptorSet()
{
    const std::array<VkDescriptorSetLayout, 1> set_layouts
    {
        m_DescriptorSetLayout
    };

    const VkDescriptorSetAllocateInfo allocate_info
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = m_DescriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = set_layouts.data(),
    };

    return vk::DescriptorSet::create(m_Device, allocate_info) >> m_DescriptorSet;
}

core::result<> core::Instance::CreateRenderPass()
{
    const std::array attachments
    {
        VkAttachmentDescription
        {
            .flags = 0,
            .format = m_ColorFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
        VkAttachmentDescription
        {
            .flags = 0,
            .format = m_DepthFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        }
    };

    const std::array color_attachments
    {
        VkAttachmentReference
        {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        }
    };

    const VkAttachmentReference depth_attachment
    {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    const std::array subpasses
    {
        VkSubpassDescription
        {
            .flags = 0,
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .inputAttachmentCount = 0,
            .pInputAttachments = nullptr,
            .colorAttachmentCount = color_attachments.size(),
            .pColorAttachments = color_attachments.data(),
            .pResolveAttachments = nullptr,
            .pDepthStencilAttachment = &depth_attachment,
            .preserveAttachmentCount = 0,
            .pPreserveAttachments = nullptr,
        }
    };

    const std::array dependencies
    {
        VkSubpassDependency
        {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = 0,
        }
    };

    const VkRenderPassCreateInfo create_info
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .attachmentCount = attachments.size(),
        .pAttachments = attachments.data(),
        .subpassCount = subpasses.size(),
        .pSubpasses = subpasses.data(),
        .dependencyCount = dependencies.size(),
        .pDependencies = dependencies.data(),
    };

    return vk::RenderPass::create(m_Device, create_info) >> m_RenderPass;
}

core::result<> core::Instance::CreatePipelineCache()
{
    const VkPipelineCacheCreateInfo create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .initialDataSize = 0,
        .pInitialData = nullptr,
    };

    return vk::PipelineCache::create(m_Device, create_info) >> m_PipelineCache;
}

core::result<> core::Instance::CreatePipelineLayout()
{
    const std::array<VkDescriptorSetLayout, 1> set_layouts
    {
        m_DescriptorSetLayout,
    };

    const VkPipelineLayoutCreateInfo create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = set_layouts.size(),
        .pSetLayouts = set_layouts.data(),
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };

    return vk::PipelineLayout::create(m_Device, create_info) >> m_PipelineLayout;
}

core::result<> core::Instance::CreatePipeline()
{
    vk::ShaderModule shader_module_vertex, shader_module_fragment;

    auto shader_module_vertex_binary = LoadShaderModuleBinary("res/shader/vert.spv");
    auto shader_module_fragment_binary = LoadShaderModuleBinary("res/shader/frag.spv");

    const VkShaderModuleCreateInfo shader_module_vertex_create_info
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = shader_module_vertex_binary.size(),
        .pCode = reinterpret_cast<const uint32_t *>(shader_module_vertex_binary.data()),
    };

    const VkShaderModuleCreateInfo shader_module_fragment_create_info
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = shader_module_fragment_binary.size(),
        .pCode = reinterpret_cast<const uint32_t *>(shader_module_fragment_binary.data()),
    };

    TRY(vk::ShaderModule::create(m_Device, shader_module_vertex_create_info) >> shader_module_vertex);
    TRY(vk::ShaderModule::create(m_Device, shader_module_fragment_create_info) >> shader_module_fragment);

    const std::array stage_create_info
    {
        VkPipelineShaderStageCreateInfo
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = shader_module_vertex,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        },
        VkPipelineShaderStageCreateInfo
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = shader_module_fragment,
            .pName = "main",
            .pSpecializationInfo = nullptr
        },
    };

    const std::array vertex_binding_descriptions
    {
        VkVertexInputBindingDescription
        {
            .binding = 0,
            .stride = sizeof(obj::Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        }
    };

    const std::array vertex_attribute_descriptions
    {
        VkVertexInputAttributeDescription
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(obj::Vertex, Position),
        },
        VkVertexInputAttributeDescription
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(obj::Vertex, Normal),
        },
        VkVertexInputAttributeDescription
        {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(obj::Vertex, Texture),
        },
    };

    const VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = vertex_binding_descriptions.size(),
        .pVertexBindingDescriptions = vertex_binding_descriptions.data(),
        .vertexAttributeDescriptionCount = vertex_attribute_descriptions.size(),
        .pVertexAttributeDescriptions = vertex_attribute_descriptions.data(),
    };

    const VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = false,
    };

    const VkPipelineTessellationStateCreateInfo tessellation_state_create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .patchControlPoints = 0,
    };

    const VkPipelineViewportStateCreateInfo viewport_state_create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 0,
        .pViewports = nullptr,
        .scissorCount = 0,
        .pScissors = nullptr,
    };

    const VkPipelineRasterizationStateCreateInfo rasterization_state_create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = false,
        .rasterizerDiscardEnable = false,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = false,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    const VkPipelineMultisampleStateCreateInfo multisample_state_create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = false,
        .minSampleShading = 0.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = false,
        .alphaToOneEnable = false,
    };

    const VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthTestEnable = true,
        .depthWriteEnable = true,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = false,
        .stencilTestEnable = false,
        .front = {},
        .back = {},
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };

    const std::array attachments
    {
        VkPipelineColorBlendAttachmentState
        {
            .blendEnable = false,
            .srcColorBlendFactor = {},
            .dstColorBlendFactor = {},
            .colorBlendOp = {},
            .srcAlphaBlendFactor = {},
            .dstAlphaBlendFactor = {},
            .alphaBlendOp = {},
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                              | VK_COLOR_COMPONENT_G_BIT
                              | VK_COLOR_COMPONENT_B_BIT
                              | VK_COLOR_COMPONENT_A_BIT,
        },
    };

    const VkPipelineColorBlendStateCreateInfo color_blend_state_create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = false,
        .logicOp = VK_LOGIC_OP_CLEAR,
        .attachmentCount = attachments.size(),
        .pAttachments = attachments.data(),
        .blendConstants = {
            0.0f,
            0.0f,
            0.0f,
            0.0f,
        },
    };

    const std::array dynamic_states
    {
        VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT,
        VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT,
    };

    const VkPipelineDynamicStateCreateInfo dynamic_state_create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = dynamic_states.size(),
        .pDynamicStates = dynamic_states.data(),
    };

    const VkGraphicsPipelineCreateInfo create_info
    {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = stage_create_info.size(),
        .pStages = stage_create_info.data(),
        .pVertexInputState = &vertex_input_state_create_info,
        .pInputAssemblyState = &input_assembly_state_create_info,
        .pTessellationState = &tessellation_state_create_info,
        .pViewportState = &viewport_state_create_info,
        .pRasterizationState = &rasterization_state_create_info,
        .pMultisampleState = &multisample_state_create_info,
        .pDepthStencilState = &depth_stencil_state_create_info,
        .pColorBlendState = &color_blend_state_create_info,
        .pDynamicState = &dynamic_state_create_info,
        .layout = m_PipelineLayout,
        .renderPass = m_RenderPass,
        .subpass = 0,
        .basePipelineHandle = nullptr,
        .basePipelineIndex = 0,
    };

    return vk::Pipeline::create(m_Device, m_PipelineCache, create_info) >> m_Pipeline;
}

core::result<> core::Instance::CreateFramebuffers()
{
    for (uint32_t j = 0; j < m_SwapchainFrames.size(); ++j)
    {
        const auto &view = m_ViewConfigurationViews[j];

        auto &[color, depth, framebuffers] = m_SwapchainFrames[j];

        framebuffers.resize(color.Views.size());
        for (uint32_t i = 0; i < framebuffers.size(); ++i)
        {
            const std::array<VkImageView, 2> attachments
            {
                color.Views[i],
                depth.Views[i],
            };

            const VkFramebufferCreateInfo create_info
            {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .renderPass = m_RenderPass,
                .attachmentCount = attachments.size(),
                .pAttachments = attachments.data(),
                .width = view.recommendedImageRectWidth,
                .height = view.recommendedImageRectHeight,
                .layers = 1,
            };

            TRY(vk::Framebuffer::create(m_Device, create_info) >> framebuffers[i]);
        }
    }

    return ok();
}

core::result<> core::Instance::CreateCommandPool()
{
    const VkCommandPoolCreateInfo create_info
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = m_QueueFamilyIndex,
    };

    return vk::CommandPool::create(m_Device, create_info) >> m_CommandPool;
}

core::result<> core::Instance::CreateCommandBuffer()
{
    const VkCommandBufferAllocateInfo allocate_info
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = m_CommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    return vk::CommandBuffer::create(m_Device, allocate_info) >> m_CommandBuffer;
}

core::result<> core::Instance::CreateSynchronization()
{
    const VkSemaphoreCreateInfo semaphore_create_info
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };

    TRY(vk::Semaphore::create(m_Device, semaphore_create_info) >> m_ImageAvailableSemaphore);
    TRY(vk::Semaphore::create(m_Device, semaphore_create_info) >> m_RenderFinishedSemaphore);

    const VkFenceCreateInfo fence_create_info
    {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    TRY(vk::Fence::create(m_Device, fence_create_info) >> m_InFlightFence);

    return ok();
}

static core::result<uint32_t> find_memory_type(
    VkPhysicalDevice physical_device,
    uint32_t type_filter,
    VkMemoryPropertyFlags type_flags)
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
    {
        auto &[flags, _1] = memory_properties.memoryTypes[i];
        if (type_filter & 1 << i && type_flags & flags)
            return i;
    }

    return core::error<uint32_t>("failed to find any suitable memory type.");
}

core::result<> core::Instance::CreateVertexBuffer()
{
    const VkBufferCreateInfo create_info
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = m_Mesh.Vertices.size() * sizeof(obj::Vertex),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };

    return vk::Buffer::create(m_Device, create_info) >> m_VertexBuffer;
}

core::result<> core::Instance::CreateVertexMemory()
{
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(m_Device, m_VertexBuffer, &memory_requirements);

    uint32_t memory_type_index;
    TRY(
        find_memory_type(
            m_PhysicalDevice,
            memory_requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        >> memory_type_index
    )

    const VkMemoryAllocateInfo allocate_info
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = memory_type_index,
    };

    TRY(vk::DeviceMemory::create(m_Device, allocate_info) >> m_VertexMemory);

    if (auto res = vkBindBufferMemory(m_Device, m_VertexBuffer, m_VertexMemory, 0))
        return error("vkBindBufferMemory => {}", res);

    return ok();
}

core::result<> core::Instance::FillVertexBuffer()
{
    void *data;
    if (auto res = vkMapMemory(m_Device, m_VertexMemory, 0, m_Mesh.Vertices.size() * sizeof(obj::Vertex), 0, &data))
        return error("vkMapMemory => {}", res);

    memcpy(data, m_Mesh.Vertices.data(), m_Mesh.Vertices.size() * sizeof(obj::Vertex));

    vkUnmapMemory(m_Device, m_VertexMemory);

    return ok();
}

core::result<> core::Instance::CreateCameraBuffer()
{
    const VkBufferCreateInfo create_info
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = sizeof(CameraData),
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };

    return vk::Buffer::create(m_Device, create_info) >> m_CameraBuffer;
}

core::result<> core::Instance::CreateCameraMemory()
{
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(m_Device, m_CameraBuffer, &memory_requirements);

    uint32_t memory_type_index;
    TRY(
        find_memory_type(
            m_PhysicalDevice,
            memory_requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        >> memory_type_index
    )

    const VkMemoryAllocateInfo allocate_info
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = memory_type_index,
    };

    TRY(vk::DeviceMemory::create(m_Device, allocate_info) >> m_CameraMemory);

    if (auto res = vkBindBufferMemory(m_Device, m_CameraBuffer, m_CameraMemory, 0))
        return error("vkBindBufferMemory => {}", res);

    return ok();
}

core::result<> core::Instance::RecordCommandBuffer(const uint32_t view_index, const uint32_t image_index)
{
    const auto &view = m_ViewConfigurationViews[view_index];

    const auto width = view.recommendedImageRectWidth;
    const auto height = view.recommendedImageRectHeight;

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

    const VkCommandBufferBeginInfo command_buffer_begin_info
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pInheritanceInfo = nullptr,
    };

    if (auto res = vkResetCommandBuffer(m_CommandBuffer, 0))
        return error("vkResetCommandBuffer => {}", res);

    if (auto res = vkBeginCommandBuffer(m_CommandBuffer, &command_buffer_begin_info))
        return error("vkBeginCommandBuffer => {}", res);

    const std::array clear_values
    {
        VkClearValue{ .color = { .float32 = { 0.0f, 0.0f, 0.0f, 1.0f } } },
        VkClearValue{ .depthStencil = { .depth = 1.0f } },
    };

    const VkRenderPassBeginInfo render_pass_begin_info
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = m_RenderPass,
        .framebuffer = m_SwapchainFrames[view_index].Framebuffers[image_index],
        .renderArea = {
            .offset = {
                .x = 0,
                .y = 0,
            },
            .extent = {
                .width = width,
                .height = height,
            },
        },
        .clearValueCount = clear_values.size(),
        .pClearValues = clear_values.data(),
    };

    vkCmdBeginRenderPass(m_CommandBuffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    vkCmdSetViewportWithCount(m_CommandBuffer, 1, &viewport);
    vkCmdSetScissorWithCount(m_CommandBuffer, 1, &scissor);

    const std::array<VkBuffer, 1> buffers
    {
        m_VertexBuffer,
    };

    const std::array<VkDeviceSize, 1> offsets
    {
        0,
    };

    vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, buffers.data(), offsets.data());

    const std::array<VkDescriptorSet, 1> descriptor_sets
    {
        m_DescriptorSet,
    };

    vkCmdBindDescriptorSets(
        m_CommandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_PipelineLayout,
        0,
        descriptor_sets.size(),
        descriptor_sets.data(),
        0,
        nullptr);

    vkCmdDraw(m_CommandBuffer, m_Mesh.Vertices.size(), 1, 0, 0);

    vkCmdEndRenderPass(m_CommandBuffer);

    if (auto res = vkEndCommandBuffer(m_CommandBuffer))
        return error("vkEndCommandBuffer => {}", res);

    return ok();
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
        .PredictedDisplayTime = frame_state.predictedDisplayTime,
    };

    const auto session_active = m_SessionState == XR_SESSION_STATE_SYNCHRONIZED
                                || m_SessionState == XR_SESSION_STATE_VISIBLE
                                || m_SessionState == XR_SESSION_STATE_FOCUSED;

    if (session_active && frame_state.shouldRender)
    {
        TRY(RenderLayer(render_layer_reference));

        render_layer_reference.Layers.push_back(
            reinterpret_cast<XrCompositionLayerBaseHeader *>(&render_layer_reference.Projection));
    }

    const XrFrameEndInfo frame_end_info
    {
        .type = XR_TYPE_FRAME_END_INFO,
        .next = nullptr,
        .displayTime = frame_state.predictedDisplayTime,
        .environmentBlendMode = m_EnvironmentBlendMode,
        .layerCount = static_cast<uint32_t>(render_layer_reference.Layers.size()),
        .layers = render_layer_reference.Layers.data(),
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
        .displayTime = reference.PredictedDisplayTime,
        .space = m_ReferenceSpace,
    };

    uint32_t view_count;
    if (auto res = xrLocateViews(
        m_Session,
        &view_locate_info,
        &view_state,
        views.size(),
        &view_count,
        views.data()))
        return error("xrLocateViews => {}", res);

    reference.Views.resize(view_count, { .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW });

    for (uint32_t i = 0; i < view_count; ++i)
    {
        const auto &configuration = m_ViewConfigurationViews[i];

        auto &[color, depth, framebuffers] = m_SwapchainFrames[i];

        uint32_t color_index, depth_index;

        const XrSwapchainImageAcquireInfo swapchain_image_acquire_info
        {
            .type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO,
            .next = nullptr,
        };

        if (auto res = xrAcquireSwapchainImage(color.Swapchain, &swapchain_image_acquire_info, &color_index))
            return error("xrAcquireSwapchainImage => {}", res);
        if (auto res = xrAcquireSwapchainImage(depth.Swapchain, &swapchain_image_acquire_info, &depth_index))
            return error("xrAcquireSwapchainImage => {}", res);

        const XrSwapchainImageWaitInfo swapchain_image_wait_info
        {
            .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
            .next = nullptr,
            .timeout = XR_INFINITE_DURATION,
        };

        if (auto res = xrWaitSwapchainImage(color.Swapchain, &swapchain_image_wait_info))
            return error("xrWaitSwapchainImage => {}", res);
        if (auto res = xrWaitSwapchainImage(depth.Swapchain, &swapchain_image_wait_info))
            return error("xrWaitSwapchainImage => {}", res);

        const auto width = configuration.recommendedImageRectWidth;
        const auto height = configuration.recommendedImageRectHeight;

        const auto &pose = views[i].pose;
        const auto &fov = views[i].fov;

        glm::mat4 model_mat;
        {
            static auto begin = std::chrono::high_resolution_clock::now();
            auto now = std::chrono::high_resolution_clock::now();

            auto delta = std::chrono::duration_cast<std::chrono::duration<float>>(now - begin).count();

            model_mat = { 1.0f };
            model_mat = glm::translate(model_mat, glm::vec3(0.0f, -0.5f, -0.5f));
            model_mat = glm::scale(model_mat, glm::vec3(0.05f));
            model_mat = glm::rotate(model_mat, delta * glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model_mat = glm::rotate(model_mat, delta * glm::radians(10.0f), glm::vec3(1.0f, 0.0f, 1.0f));
            model_mat = glm::translate(model_mat, glm::vec3(-0.5f, -0.5f, -0.5f));
        }

        glm::mat4 view_mat;
        {
            const glm::quat orientation
            {
                pose.orientation.w,
                pose.orientation.x,
                pose.orientation.y,
                pose.orientation.z,
            };

            const glm::vec3 position
            {
                pose.position.x,
                pose.position.y,
                pose.position.z,
            };

            const auto rotation = glm::mat4_cast(glm::conjugate(orientation));
            const auto translation = glm::translate(glm::mat4(1.0f), -position);

            view_mat = rotation * translation;
        }

        glm::mat4 proj_mat;
        {
            constexpr auto near = 0.01f;
            constexpr auto far = 100.0f;

            auto l = near * tanf(fov.angleLeft);
            auto r = near * tanf(fov.angleRight);
            auto b = near * tanf(fov.angleDown);
            auto t = near * tanf(fov.angleUp);

            proj_mat = glm::frustumRH_ZO(l, r, t, b, near, far);
        }

        reference.Views[i] = {
            .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
            .next = nullptr,
            .pose = pose,
            .fov = fov,
            .subImage = {
                .swapchain = color.Swapchain,
                .imageRect = {
                    .offset = {
                        .x = 0,
                        .y = 0,
                    },
                    .extent = {
                        .width = static_cast<int32_t>(width),
                        .height = static_cast<int32_t>(height),
                    },
                },
                .imageArrayIndex = 0,
            },
        };

        const std::array<VkFence, 1> fences
        {
            m_InFlightFence,
        };

        if (auto res = vkWaitForFences(m_Device, fences.size(), fences.data(), true, UINT64_MAX))
            return error("vkWaitForFences => {}", res);

        vkResetFences(m_Device, fences.size(), fences.data());

        m_CameraData = {
            .model = model_mat,
            .inv_model = glm::inverse(model_mat),
            .view = view_mat,
            .inv_view = glm::inverse(view_mat),
            .proj = proj_mat,
            .inv_proj = glm::inverse(proj_mat),
        };

        {
            void *data;
            if (auto res = vkMapMemory(m_Device, m_CameraMemory, 0, sizeof(CameraData), 0, &data))
                return error("vkMapMemory => {}", res);

            memcpy(data, &m_CameraData, sizeof(CameraData));

            vkUnmapMemory(m_Device, m_CameraMemory);
        }

        const VkDescriptorBufferInfo buffer_info
        {
            .buffer = m_CameraBuffer,
            .offset = 0,
            .range = sizeof(CameraData),
        };

        const std::array writes
        {
            VkWriteDescriptorSet
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = m_DescriptorSet,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pImageInfo = nullptr,
                .pBufferInfo = &buffer_info,
                .pTexelBufferView = nullptr,
            },
        };

        vkUpdateDescriptorSets(m_Device, writes.size(), writes.data(), 0, nullptr);

        TRY(RecordCommandBuffer(i, color_index));

        const std::array<VkCommandBuffer, 1> command_buffers
        {
            m_CommandBuffer,
        };

        const std::array<VkSemaphore, 1> wait_semaphores
        {
            m_ImageAvailableSemaphore,
        };

        const std::array<VkPipelineStageFlags, 1> wait_stages
        {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        };

        const std::array<VkSemaphore, 1> signal_semaphores
        {
            m_RenderFinishedSemaphore,
        };

        const VkSubmitInfo submit_info
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = wait_semaphores.data(),
            .pWaitDstStageMask = wait_stages.data(),
            .commandBufferCount = command_buffers.size(),
            .pCommandBuffers = command_buffers.data(),
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = signal_semaphores.data(),
        };

        VkQueue queue;
        vkGetDeviceQueue(m_Device, m_QueueFamilyIndex, m_QueueIndex, &queue);

        if (auto res = vkQueueSubmit(queue, 1, &submit_info, m_InFlightFence))
            return error("vkQueueSubmit => {}", res);

        const XrSwapchainImageReleaseInfo swapchain_image_release_info
        {
            .type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO,
            .next = nullptr,
        };

        if (auto res = xrReleaseSwapchainImage(color.Swapchain, &swapchain_image_release_info))
            return error("xrReleaseSwapchainImage => {}", res);
        if (auto res = xrReleaseSwapchainImage(depth.Swapchain, &swapchain_image_release_info))
            return error("xrReleaseSwapchainImage => {}", res);
    }

    reference.Projection = {
        .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
        .next = nullptr,
        .layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT
                      | XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT,
        .space = m_ReferenceSpace,
        .viewCount = static_cast<uint32_t>(reference.Views.size()),
        .views = reference.Views.data(),
    };

    return ok();
}

std::vector<char> core::Instance::LoadShaderModuleBinary(const std::filesystem::path &path)
{
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream)
        return {};

    const auto count = stream.tellg();
    stream.seekg(0, std::ios::beg);

    std::vector<char> binary(count);
    stream.read(binary.data(), static_cast<std::streamsize>(binary.size()));

    return binary;
}
