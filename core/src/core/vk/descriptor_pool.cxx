#include <titan/core.hxx>

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
