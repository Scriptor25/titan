#include <titan/core.hxx>
#include <titan/utils.hxx>

#include <cstring>

titan::result<> titan::Application::FillBuffers()
{
    for (uint32_t i = 0; i < m_ModelData.size(); ++i)
    {
        auto &data = m_ModelData[i];
        auto &reference = m_ModelReferences[i];

        reference.VertexBufferOffset = 0;
        reference.VertexBufferSize = data.Mesh.Vertices.size() * sizeof(VertexData);
        reference.VertexBufferStride = sizeof(VertexData);

        reference.IndexBufferOffset = 0;
        reference.IndexBufferSize = data.Mesh.Indices.size() * sizeof(uint32_t);
        reference.IndexType = VK_INDEX_TYPE_UINT32;
        reference.IndexCount = data.Mesh.Indices.size();

        reference.Instances.resize(data.InstanceCount);

        {
            const VkMemoryMapInfo map_info
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO,
                .memory = reference.VertexMemory,
                .offset = reference.VertexBufferOffset,
                .size = reference.VertexBufferSize,
            };

            const VkMemoryUnmapInfo unmap_info
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO,
                .memory = reference.VertexMemory,
            };

            void *ptr;
            if (auto res = vk::MapMemory2(m_Device, map_info) >> ptr)
                return res;

            std::memcpy(ptr, data.Mesh.Vertices.data(), reference.VertexBufferSize);

            if (auto res = vk::UnmapMemory2(m_Device, unmap_info))
                return res;
        }

        {
            const VkMemoryMapInfo map_info
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO,
                .memory = reference.IndexMemory,
                .offset = reference.IndexBufferOffset,
                .size = reference.IndexBufferSize,
            };

            const VkMemoryUnmapInfo unmap_info
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO,
                .memory = reference.IndexMemory,
            };

            void *ptr;
            if (auto res = vk::MapMemory2(m_Device, map_info) >> ptr)
                return res;

            std::memcpy(ptr, data.Mesh.Indices.data(), reference.IndexBufferSize);

            if (auto res = vk::UnmapMemory2(m_Device, unmap_info))
                return res;
        }
    }

    return ok();
}
