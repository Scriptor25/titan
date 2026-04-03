#include <titan/core.hxx>
#include <titan/utils.hxx>

core::result<> core::Application::AllocateBufferMemory()
{
    const VkBufferMemoryRequirementsInfo2 requirements_info
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
        .buffer = m_VertexBuffer,
    };

    const auto requirements = vk::GetBufferMemoryRequirements2(m_Device, requirements_info).memoryRequirements;

    return ok()
           & [&]
           {
               return FindMemoryType(
                   m_PhysicalDevice,
                   requirements.memoryTypeBits,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                   | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
           }
           & [&](uint32_t &&memory_type_index)
           {
               return vk::DeviceMemory::create(
                          m_Device,
                          {
                              .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                              .allocationSize = requirements.size,
                              .memoryTypeIndex = memory_type_index,
                          })
                      >> m_VertexMemory;
           }
           & [&]
           {
               const VkBindBufferMemoryInfo bind_info
               {
                   .sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
                   .buffer = m_VertexBuffer,
                   .memory = m_VertexMemory,
                   .memoryOffset = 0,
               };

               return vk::BindBufferMemory2(m_Device, bind_info);
           };
}
