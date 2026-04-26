#include <titan/core.hxx>
#include <titan/utils.hxx>

#include <pkg/mesh.hxx>

toolkit::result<> titan::Application::CreateBuffers()
{
    m_ModelReferences.resize(m_ModelData.size());
    for (uint32_t i = 0; i < m_ModelData.size(); ++i)
    {
        auto data = m_Resources.Get<pkg::mesh::Data>(m_ModelData[i].Mesh);
        auto &reference = m_ModelReferences[i];

        reference.BoxMin = data.BoxMin;
        reference.BoxMax = data.BoxMax;

        reference.BoxCen = data.BoxMin + 0.5f * (data.BoxMax - data.BoxMin);

        {
            const VkBufferCreateInfo create_info
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = data.Vertices.size() * sizeof(pkg::mesh::Vertex),
                .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            };

            if (auto res = vk::Buffer::create(m_Device, create_info) >> reference.VertexBuffer; !res)
                return res;

            const VkBufferMemoryRequirementsInfo2 requirements_info
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
                .buffer = reference.VertexBuffer,
            };

            const auto requirements = vk::GetBufferMemoryRequirements2(
                        m_Device,
                        requirements_info)
                    .memoryRequirements;

            uint32_t memory_type_index;
            if (auto res = FindMemoryType(
                               m_PhysicalDevice,
                               requirements.memoryTypeBits,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                               | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
                           >> memory_type_index; !res)
                return res;

            const VkMemoryAllocateInfo allocate_info
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = requirements.size,
                .memoryTypeIndex = memory_type_index,
            };

            if (auto res = vk::DeviceMemory::create(m_Device, allocate_info) >> reference.VertexMemory; !res)
                return res;

            const VkBindBufferMemoryInfo bind_info
            {
                .sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
                .buffer = reference.VertexBuffer,
                .memory = reference.VertexMemory,
                .memoryOffset = 0,
            };

            if (auto res = vk::BindBufferMemory2(m_Device, bind_info); !res)
                return res;
        }

        {
            const VkBufferCreateInfo create_info
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = data.Indices.size() * sizeof(uint32_t),
                .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            };

            if (auto res = vk::Buffer::create(m_Device, create_info) >> reference.IndexBuffer; !res)
                return res;

            const VkBufferMemoryRequirementsInfo2 requirements_info
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
                .buffer = reference.IndexBuffer,
            };

            const auto requirements = vk::GetBufferMemoryRequirements2(
                        m_Device,
                        requirements_info)
                    .memoryRequirements;

            uint32_t memory_type_index;
            if (auto res = FindMemoryType(
                               m_PhysicalDevice,
                               requirements.memoryTypeBits,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                               | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
                           >> memory_type_index; !res)
                return res;

            const VkMemoryAllocateInfo allocate_info
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = requirements.size,
                .memoryTypeIndex = memory_type_index,
            };

            if (auto res = vk::DeviceMemory::create(m_Device, allocate_info) >> reference.IndexMemory; !res)
                return res;

            const VkBindBufferMemoryInfo bind_info
            {
                .sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
                .buffer = reference.IndexBuffer,
                .memory = reference.IndexMemory,
                .memoryOffset = 0,
            };

            if (auto res = vk::BindBufferMemory2(m_Device, bind_info); !res)
                return res;
        }
    }

    return ok();
}
