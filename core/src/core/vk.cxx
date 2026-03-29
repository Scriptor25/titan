#include <titan/core.hxx>
#include <titan/format/vk.hxx>

#include <cstring>
#include <set>

#define XR_TO_VK_VERSION(VERSION) VK_MAKE_VERSION(XR_VERSION_MAJOR(VERSION), XR_VERSION_MINOR(VERSION), XR_VERSION_PATCH(VERSION))

core::result<> core::Application::CreateVkInstance()
{
    std::vector<const char *> extensions;
    GetInstanceExtensions(extensions);

    XrGraphicsRequirementsVulkan2KHR graphics_requirements
    {
        .type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR,
        .minApiVersionSupported = {},
        .maxApiVersionSupported = {},
    };
    if (auto res = xrGetVulkanGraphicsRequirements2KHR(m_XrInstance, m_SystemId, &graphics_requirements))
        return error("xrGetVulkanGraphicsRequirements2KHR => {}", res);

    const VkApplicationInfo application_info
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = m_Info.Name.c_str(),
        .applicationVersion = VK_MAKE_VERSION(m_Info.Version.Major, m_Info.Version.Minor, m_Info.Version.Patch),
        .pEngineName = "Titan Core",
        .engineVersion = VK_MAKE_VERSION(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH),
        .apiVersion = XR_TO_VK_VERSION(graphics_requirements.maxApiVersionSupported),
    };

    const VkInstanceCreateInfo instance_create_info
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &application_info,
        .enabledLayerCount = VK_INSTANCE_LAYERS.size(),
        .ppEnabledLayerNames = VK_INSTANCE_LAYERS.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

    const XrVulkanInstanceCreateInfoKHR vk_instance_create_info
    {
        .type = XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR,
        .systemId = m_SystemId,
        .pfnGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vulkanCreateInfo = &instance_create_info,
    };

    VkResult vk_result;
    VkInstance vk_instance;

    if (auto res = xrCreateVulkanInstanceKHR(m_XrInstance, &vk_instance_create_info, &vk_instance, &vk_result);
        res || vk_result)
        return error("xrCreateVulkanInstanceKHR => {}, {}", res, vk_result);

    m_VkInstance = vk::Instance::wrap(vk_instance);
    return ok();
}

core::result<> core::Application::CreateVkMessenger()
{
    const VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                           | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                           | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                           | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                       | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                       | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = VkDebugCallback,
        .pUserData = this,
    };

    return vk::DebugUtilsMessengerEXT::create(m_VkInstance, debug_utils_messenger_create_info) >> m_VkMessenger;
}

core::result<> core::Application::GetPhysicalDevice()
{
    const XrVulkanGraphicsDeviceGetInfoKHR get_info
    {
        .type = XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR,
        .systemId = m_SystemId,
        .vulkanInstance = m_VkInstance,
    };

    VkPhysicalDevice physical_device;
    if (auto res = xrGetVulkanGraphicsDevice2KHR(m_XrInstance, &get_info, &physical_device))
        return error("xrGetVulkanGraphicsDevice2KHR => {}", res);

    m_PhysicalDevice = physical_device;

    VkPhysicalDeviceProperties2 physical_device_properties
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
    };
    vkGetPhysicalDeviceProperties2(m_PhysicalDevice, &physical_device_properties);

    if (sizeof(CameraData) > physical_device_properties.properties.limits.maxPushConstantsSize)
        return error(
            "camera data struct size is greater than physical device max push constants size ({} > {}).",
            sizeof(CameraData),
            physical_device_properties.properties.limits.maxPushConstantsSize);

    return ok();
}

core::result<> core::Application::GetQueueFamilyIndices()
{
    std::optional<uint32_t> index_default, index_graphics, index_compute, index_transfer, index_present;

    auto queue_family_properties = vk::GetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice);

    for (uint32_t i = 0; i < queue_family_properties.size(); ++i)
    {
        auto &[flags, count, _0, _1] = queue_family_properties[i].queueFamilyProperties;

        if (!count)
            continue;

        if (!index_default
            && flags & VK_QUEUE_GRAPHICS_BIT
            && flags & VK_QUEUE_COMPUTE_BIT
            && flags & VK_QUEUE_TRANSFER_BIT)
            index_default = i;

        if (!index_graphics && flags & VK_QUEUE_GRAPHICS_BIT)
            index_graphics = i;
        if (!index_compute && flags & VK_QUEUE_COMPUTE_BIT)
            index_compute = i;
        if (!index_transfer && flags & VK_QUEUE_TRANSFER_BIT)
            index_transfer = i;

        if (!index_present)
        {
            VkBool32 supported;
            vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, i, m_WindowSurface, &supported);

            if (supported)
                index_present = i;
        }
    }

    if (!index_default)
    {
        for (uint32_t i = 0; i < queue_family_properties.size(); ++i)
        {
            auto &[flags, count, _0, _1] = queue_family_properties[i].queueFamilyProperties;

            if (!count)
                continue;

            if (!index_default
                && flags & VK_QUEUE_GRAPHICS_BIT
                && flags & VK_QUEUE_COMPUTE_BIT)
            {
                index_default = i;
                break;
            }
        }
    }

    if (!index_default)
    {
        for (uint32_t i = 0; i < queue_family_properties.size(); ++i)
        {
            auto &[flags, count, _0, _1] = queue_family_properties[i].queueFamilyProperties;

            if (!count)
                continue;

            if (!index_default && flags & VK_QUEUE_GRAPHICS_BIT)
            {
                index_default = i;
                break;
            }
        }
    }

    std::set<std::string> missing;
    if (!index_default)
        missing.insert("default");
    if (!index_graphics)
        missing.insert("graphics");
    if (!index_compute)
        missing.insert("compute");
    if (!index_transfer)
        missing.insert("transfer");
    if (!index_present)
        missing.insert("present");

    if (missing.empty())
    {
        m_QueueFamilyIndices = {
            .Default = *index_default,
            .Graphics = *index_graphics,
            .Compute = *index_compute,
            .Transfer = *index_transfer,
            .Present = *index_present,
        };
        return ok();
    }

    return error("failed to find any suitable queue family for {}.", missing);
}

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

core::result<> core::Application::GetDeviceQueues()
{
    {
        const VkDeviceQueueInfo2 device_queue_info
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
            .queueFamilyIndex = m_QueueFamilyIndices.Default,
            .queueIndex = 0,
        };

        vkGetDeviceQueue2(m_Device, &device_queue_info, &m_DefaultQueue);
    }

    {
        const VkDeviceQueueInfo2 device_queue_info
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
            .queueFamilyIndex = m_QueueFamilyIndices.Transfer,
            .queueIndex = 0,
        };

        vkGetDeviceQueue2(m_Device, &device_queue_info, &m_TransferQueue);
    }

    {
        const VkDeviceQueueInfo2 device_queue_info
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
            .queueFamilyIndex = m_QueueFamilyIndices.Present,
            .queueIndex = 0,
        };

        vkGetDeviceQueue2(m_Device, &device_queue_info, &m_PresentQueue);
    }

    return ok();
}

core::result<> core::Application::CreateWindowSurface()
{
    return vk::SurfaceKHR::create(m_VkInstance, m_Window) >> m_WindowSurface;
}

static const VkSurfaceFormatKHR &select_surface_format(const std::vector<VkSurfaceFormatKHR> &formats)
{
    for (auto &format : formats)
        if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
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

core::result<> core::Application::CreateWindowSwapchain()
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
    auto set_surface_formats = [&](std::vector<VkSurfaceFormat2KHR> &&formats)
    {
        surface_formats.resize(formats.size());
        for (uint32_t i = 0; i < formats.size(); ++i)
            surface_formats[i] = formats[i].surfaceFormat;
        return ok();
    };

    TRY(vk::GetPhysicalDeviceSurfaceFormats2KHR(m_PhysicalDevice, surface_info) | set_surface_formats);

    std::vector<VkPresentModeKHR> present_modes;
    TRY(vk::GetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_WindowSurface) >> present_modes);

    const auto &[format, color_space] = select_surface_format(surface_formats);
    const auto present_mode = select_present_mode(present_modes);
    const auto surface_extent = select_extent(capabilities, m_Window);

    VkSharingMode sharing_mode;
    std::vector<uint32_t> queue_family_indices;

    if (m_QueueFamilyIndices.Default != m_QueueFamilyIndices.Present)
    {
        sharing_mode = VK_SHARING_MODE_CONCURRENT;
        queue_family_indices = {
            m_QueueFamilyIndices.Default,
            m_QueueFamilyIndices.Present,
        };
    }
    else
    {
        sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    }

    auto image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount)
        image_count = capabilities.maxImageCount;

    const VkSwapchainCreateInfoKHR create_info
    {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = m_WindowSurface,
        .minImageCount = image_count,
        .imageFormat = format,
        .imageColorSpace = color_space,
        .imageExtent = surface_extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = sharing_mode,
        .queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size()),
        .pQueueFamilyIndices = queue_family_indices.data(),
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = true,
    };

    m_WindowSwapchain = {
        .Format = format,
        .Width = surface_extent.width,
        .Height = surface_extent.height,
    };

    TRY(vk::SwapchainKHR::create(m_Device, create_info) >> m_WindowSwapchain.Swapchain);

    auto set_images = [&](std::vector<VkImage> &&images)
    {
        m_WindowSwapchain.Images.resize(images.size());
        for (uint32_t i = 0; i < images.size(); ++i)
            m_WindowSwapchain.Images[i] = vk::Image::wrap(m_Device, images[i]);
        return ok();
    };

    return vk::GetSwapchainImagesKHR(m_Device, m_WindowSwapchain.Swapchain) | set_images;
}

core::result<> core::Application::CreateDescriptorSetLayouts()
{
    // {
    //     const std::array bindings
    //     {
    //         VkDescriptorSetLayoutBinding
    //         {
    //             .binding = 0,
    //             .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    //             .descriptorCount = 1,
    //             .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    //         },
    //     };
    //
    //     const VkDescriptorSetLayoutCreateInfo create_info
    //     {
    //         .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    //         .bindingCount = bindings.size(),
    //         .pBindings = bindings.data(),
    //     };
    //
    //     TRY(vk::DescriptorSetLayout::create(m_Device, create_info) >> m_DescriptorSetLayouts.emplace_back());
    // }

    return ok();
}

core::result<> core::Application::CreateDescriptorPool()
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
        .maxSets = 1,
        .poolSizeCount = pool_sizes.size(),
        .pPoolSizes = pool_sizes.data(),
    };

    return vk::DescriptorPool::create(m_Device, create_info) >> m_DescriptorPool;
}

core::result<> core::Application::CreateDescriptorSet()
{
    std::vector<VkDescriptorSetLayout> set_layouts(m_DescriptorSetLayouts.size());
    for (uint32_t i = 0; i < m_DescriptorSetLayouts.size(); ++i)
        set_layouts[i] = m_DescriptorSetLayouts[i];

    const VkDescriptorSetAllocateInfo allocate_info
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_DescriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(set_layouts.size()),
        .pSetLayouts = set_layouts.data(),
    };

    return vk::DescriptorSet::create2(m_Device, allocate_info) >> m_DescriptorSets;
}

core::result<> core::Application::CreateRenderPass()
{
    const std::array attachments
    {
        VkAttachmentDescription2
        {
            .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
            .format = m_ColorFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
        VkAttachmentDescription2
        {
            .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
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
        VkAttachmentReference2
        {
            .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        }
    };

    const VkAttachmentReference2 depth_attachment
    {
        .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
    };

    const std::array subpasses
    {
        VkSubpassDescription2
        {
            .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .viewMask = 0,
            .colorAttachmentCount = color_attachments.size(),
            .pColorAttachments = color_attachments.data(),
            .pDepthStencilAttachment = &depth_attachment,
        }
    };

    const std::array dependencies
    {
        VkSubpassDependency2
        {
            .sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = 0,
            .viewOffset = 0,
        }
    };

    const VkRenderPassCreateInfo2 create_info
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
        .attachmentCount = attachments.size(),
        .pAttachments = attachments.data(),
        .subpassCount = subpasses.size(),
        .pSubpasses = subpasses.data(),
        .dependencyCount = dependencies.size(),
        .pDependencies = dependencies.data(),
    };

    return vk::RenderPass::create(m_Device, create_info) >> m_RenderPass;
}

core::result<> core::Application::CreatePipelineCache()
{
    // TODO: read/write pipeline cache from/to file
    const VkPipelineCacheCreateInfo create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
        .initialDataSize = 0,
        .pInitialData = nullptr,
    };

    return vk::PipelineCache::create(m_Device, create_info) >> m_PipelineCache;
}

core::result<> core::Application::CreatePipelineLayout()
{
    std::vector<VkDescriptorSetLayout> set_layouts(m_DescriptorSetLayouts.size());
    for (uint32_t i = 0; i < m_DescriptorSetLayouts.size(); ++i)
        set_layouts[i] = m_DescriptorSetLayouts[i];

    const std::array push_constant_ranges
    {
        VkPushConstantRange
        {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(CameraData),
        },
    };

    const VkPipelineLayoutCreateInfo create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(set_layouts.size()),
        .pSetLayouts = set_layouts.data(),
        .pushConstantRangeCount = push_constant_ranges.size(),
        .pPushConstantRanges = push_constant_ranges.data(),
    };

    return vk::PipelineLayout::create(m_Device, create_info) >> m_PipelineLayout;
}

core::result<> core::Application::CreatePipeline()
{
    vk::ShaderModule shader_module_vertex, shader_module_fragment;

    auto shader_module_vertex_binary = LoadShaderModuleBinary("res/shader/vert.spv");
    auto shader_module_fragment_binary = LoadShaderModuleBinary("res/shader/frag.spv");

    const VkShaderModuleCreateInfo shader_module_vertex_create_info
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shader_module_vertex_binary.size(),
        .pCode = reinterpret_cast<const uint32_t *>(shader_module_vertex_binary.data()),
    };

    const VkShaderModuleCreateInfo shader_module_fragment_create_info
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
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
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = shader_module_vertex,
            .pName = "main",
        },
        VkPipelineShaderStageCreateInfo
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
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
        .vertexBindingDescriptionCount = vertex_binding_descriptions.size(),
        .pVertexBindingDescriptions = vertex_binding_descriptions.data(),
        .vertexAttributeDescriptionCount = vertex_attribute_descriptions.size(),
        .pVertexAttributeDescriptions = vertex_attribute_descriptions.data(),
    };

    const VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = false,
    };

    const VkPipelineTessellationStateCreateInfo tessellation_state_create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .patchControlPoints = 0,
    };

    const VkPipelineViewportStateCreateInfo viewport_state_create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    };

    const VkPipelineRasterizationStateCreateInfo rasterization_state_create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
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
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    const VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
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
        .dynamicStateCount = dynamic_states.size(),
        .pDynamicStates = dynamic_states.data(),
    };

    const VkGraphicsPipelineCreateInfo create_info
    {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
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
    };

    return vk::Pipeline::create(m_Device, m_PipelineCache, create_info) >> m_Pipeline;
}

core::result<> core::Application::CreateFramebuffers()
{
    for (uint32_t j = 0; j < m_SwapchainViews.size(); ++j)
    {
        const auto &view = m_ViewConfigurationViews[j];

        auto &[color, depth, buffer, framebuffers] = m_SwapchainViews[j];

        framebuffers.resize(color.Views.size());
        for (uint32_t i = 0; i < framebuffers.size(); ++i)
        {
            const std::array attachments
            {
                *color.Views[i],
                *depth.Views[i],
            };

            const VkFramebufferCreateInfo create_info
            {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
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

core::result<> core::Application::CreateCommandPools()
{
    {
        const VkCommandPoolCreateInfo create_info
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = m_QueueFamilyIndices.Default,
        };

        TRY(vk::CommandPool::create(m_Device, create_info) >> m_DefaultPool);
    }

    {
        const VkCommandPoolCreateInfo create_info
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = m_QueueFamilyIndices.Transfer,
        };

        TRY(vk::CommandPool::create(m_Device, create_info) >> m_TransferPool);
    }

    return ok();
}

core::result<> core::Application::CreateCommandBuffers()
{
    {
        const VkCommandBufferAllocateInfo allocate_info
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_DefaultPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<uint32_t>(m_SwapchainViews.size()),
        };

        auto set_buffers = [&](std::vector<vk::CommandBuffer> &&buffers)
        {
            for (uint32_t i = 0; i < buffers.size(); ++i)
                m_SwapchainViews[i].Buffer = std::move(buffers[i]);
            return ok();
        };

        TRY(vk::CommandBuffer::create2(m_Device, allocate_info) | set_buffers);
    }

    {
        const VkCommandBufferAllocateInfo allocate_info
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_TransferPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<uint32_t>(m_Frames.size()),
        };

        auto set_buffers = [&](std::vector<vk::CommandBuffer> &&buffers)
        {
            for (uint32_t i = 0; i < buffers.size(); ++i)
                m_Frames[i].Buffer = std::move(buffers[i]);
            return ok();
        };

        TRY(vk::CommandBuffer::create2(m_Device, allocate_info) | set_buffers);
    }

    return ok();
}

core::result<> core::Application::CreateSynchronization()
{
    const VkSemaphoreCreateInfo semaphore_create_info
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    const VkFenceCreateInfo fence_create_info
    {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    TRY(vk::Fence::create(m_Device, fence_create_info) >> m_Fence);

    for (auto &[available, finished, fence, buffer] : m_Frames)
    {
        TRY(vk::Semaphore::create(m_Device, semaphore_create_info) >> available);
        TRY(vk::Semaphore::create(m_Device, semaphore_create_info) >> finished);
        TRY(vk::Fence::create(m_Device, fence_create_info) >> fence);
    }

    return ok();
}

core::result<> core::Application::CreateVertexBuffer()
{
    const VkBufferCreateInfo create_info
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = m_Mesh.Vertices.size() * sizeof(obj::Vertex),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    return vk::Buffer::create(m_Device, create_info) >> m_VertexBuffer;
}

core::result<> core::Application::CreateVertexMemory()
{
    const VkBufferMemoryRequirementsInfo2 memory_requirements_info
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
        .buffer = m_VertexBuffer,
    };

    VkMemoryRequirements2 memory_requirements
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
    };

    vkGetBufferMemoryRequirements2(m_Device, &memory_requirements_info, &memory_requirements);

    const auto &requirements = memory_requirements.memoryRequirements;

    uint32_t memory_type_index;
    TRY(
        FindMemoryType(
            m_PhysicalDevice,
            requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        >> memory_type_index
    )

    const VkMemoryAllocateInfo allocate_info
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = requirements.size,
        .memoryTypeIndex = memory_type_index,
    };

    TRY(vk::DeviceMemory::create(m_Device, allocate_info) >> m_VertexMemory);

    const std::array bind_infos
    {
        VkBindBufferMemoryInfo
        {
            .sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
            .buffer = m_VertexBuffer,
            .memory = m_VertexMemory,
            .memoryOffset = 0,
        },
    };

    if (auto res = vkBindBufferMemory2(m_Device, bind_infos.size(), bind_infos.data()))
        return error("vkBindBufferMemory2 => {}", res);

    return ok();
}

core::result<> core::Application::FillVertexBuffer()
{
    const VkMemoryMapInfo memory_map_info
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO,
        .memory = m_VertexMemory,
        .offset = 0,
        .size = m_Mesh.Vertices.size() * sizeof(obj::Vertex),
    };

    void *data;
    if (auto res = vkMapMemory2(m_Device, &memory_map_info, &data))
        return error("vkMapMemory2 => {}", res);

    memcpy(data, m_Mesh.Vertices.data(), m_Mesh.Vertices.size() * sizeof(obj::Vertex));

    const VkMemoryUnmapInfo memory_unmap_info
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO,
        .memory = m_VertexMemory,
    };

    if (auto res = vkUnmapMemory2(m_Device, &memory_unmap_info))
        return error("vkUnmapMemory2 => {}", res);

    return ok();
}

core::result<> core::Application::RecordCommandBuffer(
    const uint32_t view_index,
    const uint32_t image_index,
    const CameraData &camera_data)
{
    const auto &view = m_ViewConfigurationViews[view_index];
    const auto &buffer = m_SwapchainViews[view_index].Buffer;

    const auto width = view.recommendedImageRectWidth;
    const auto height = view.recommendedImageRectHeight;

    if (auto res = vkResetCommandBuffer(buffer, 0))
        return error("vkResetCommandBuffer => {}", res);

    const VkCommandBufferBeginInfo command_buffer_begin_info
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    if (auto res = vkBeginCommandBuffer(buffer, &command_buffer_begin_info))
        return error("vkBeginCommandBuffer => {}", res);

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

    const std::array clear_values
    {
        VkClearValue{ .color = { .float32 = { 0.0f, 0.0f, 0.0f, 0.0f } } },
        VkClearValue{ .depthStencil = { .depth = 1.0f } },
    };

    const VkRenderPassBeginInfo render_pass_begin_info
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = m_RenderPass,
        .framebuffer = m_SwapchainViews[view_index].Framebuffers[image_index],
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

    const VkSubpassBeginInfo subpass_begin_info
    {
        .sType = VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO,
        .contents = VK_SUBPASS_CONTENTS_INLINE,
    };

    vkCmdBeginRenderPass2(buffer, &render_pass_begin_info, &subpass_begin_info);

    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    vkCmdSetViewportWithCount(buffer, 1, &viewport);
    vkCmdSetScissorWithCount(buffer, 1, &scissor);

    const std::array buffers
    {
        *m_VertexBuffer,
    };

    const std::array offsets
    {
        0LU,
    };

    vkCmdBindVertexBuffers(buffer, 0, buffers.size(), buffers.data(), offsets.data());

    if (!m_DescriptorSets.empty())
    {
        std::vector<VkDescriptorSet> descriptor_sets(m_DescriptorSets.size());
        for (uint32_t i = 0; i < m_DescriptorSets.size(); ++i)
            descriptor_sets[i] = m_DescriptorSets[i];

        const VkBindDescriptorSetsInfo bind_descriptor_sets_info
        {
            .sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .layout = m_PipelineLayout,
            .firstSet = 0,
            .descriptorSetCount = static_cast<uint32_t>(descriptor_sets.size()),
            .pDescriptorSets = descriptor_sets.data(),
        };

        vkCmdBindDescriptorSets2(buffer, &bind_descriptor_sets_info);
    }

    const VkPushConstantsInfo push_constants_info
    {
        .sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO,
        .layout = m_PipelineLayout,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(CameraData),
        .pValues = &camera_data,
    };

    vkCmdPushConstants2(buffer, &push_constants_info);

    vkCmdDraw(buffer, m_Mesh.Vertices.size(), 1, 0, 0);

    const VkSubpassEndInfo subpass_end_info
    {
        .sType = VK_STRUCTURE_TYPE_SUBPASS_END_INFO,
    };

    vkCmdEndRenderPass2(buffer, &subpass_end_info);

    if (auto res = vkEndCommandBuffer(buffer))
        return error("vkEndCommandBuffer => {}", res);

    return ok();
}
