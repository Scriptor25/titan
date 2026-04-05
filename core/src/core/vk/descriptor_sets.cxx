#include <titan/core.hxx>

core::result<> core::Application::AllocateDescriptorSets()
{
    if (m_DescriptorSetLayouts.empty())
        return ok();

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

    return vk::DescriptorSet::create_collection(m_Device, allocate_info) >> m_DescriptorSets;
}
