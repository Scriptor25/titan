#include <titan/core.hxx>

toolkit::result<> titan::Application::CreatePipelineLayout()
{
    const std::array push_constant_ranges
    {
        VkPushConstantRange
        {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(ShaderData),
        },
    };

    const VkPipelineLayoutCreateInfo create_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pushConstantRangeCount = push_constant_ranges.size(),
        .pPushConstantRanges = push_constant_ranges.data(),
    };

    return vk::PipelineLayout::create(m_Device, create_info) >> m_PipelineLayout;
}
