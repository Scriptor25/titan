#include <titan/core.hxx>

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
