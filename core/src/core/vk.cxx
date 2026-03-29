#include <titan/core.hxx>
#include <titan/format/vk.hxx>
#include <titan/format/xr.hxx>

#include <cstring>

core::result<> core::Instance::CreateVkInstance()
{
    std::vector<const char *> extensions;
    GetInstanceExtensions(extensions);

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
        .enabledLayerCount = static_cast<uint32_t>(VK_LAYERS.size()),
        .ppEnabledLayerNames = VK_LAYERS.data(),
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
        .pfnUserCallback = VkDebugCallback,
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
    GetDeviceExtensions(extensions);

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
        FindMemoryType(
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
        FindMemoryType(
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
        VkClearValue{ .color = { .float32 = { 0.0f, 0.0f, 0.0f, 0.0f } } },
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
