#include <titan/core.hxx>

titan::result<> titan::Application::CreatePipeline()
{
    vk::ShaderModule shader_module_vertex, shader_module_fragment;

    auto shader_module_vertex_binary = LoadBinary("res/shader/vert.spv");
    auto shader_module_fragment_binary = LoadBinary("res/shader/frag.spv");

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

    if (auto res = vk::ShaderModule::create(m_Device, shader_module_vertex_create_info) >> shader_module_vertex)
        return res;
    if (auto res = vk::ShaderModule::create(m_Device, shader_module_fragment_create_info) >> shader_module_fragment)
        return res;

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
            .stride = sizeof(VertexData),
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
            .offset = offsetof(VertexData, Position),
        },
        VkVertexInputAttributeDescription
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(VertexData, Normal),
        },
        VkVertexInputAttributeDescription
        {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(VertexData, Texture),
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

    return vk::GraphicsPipeline::create(m_Device, m_PipelineCache, create_info) >> m_Pipeline;
}
