#include <titan/core.hxx>

core::result<> core::Application::AllocateBufferMemory()
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
