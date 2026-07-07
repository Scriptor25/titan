#include <titan/core.hxx>
#include <titan/utils.hxx>

#include <pkg/mesh.hxx>

#include <cstring>

toolkit::result<> titan::Application::FillBuffers()
{
    for (auto &[id, info] : m_Meshes)
    {
        auto data = m_Resources.Get<pkg::mesh::Data>(id);

        info.VertexBufferOffset = 0;
        info.VertexBufferSize = data.GetVertexCount() * sizeof(pkg::mesh::Vertex);
        info.VertexBufferStride = sizeof(pkg::mesh::Vertex);

        info.IndexBufferOffset = 0;
        info.IndexBufferSize = data.GetIndexCount() * sizeof(uint32_t);
        info.IndexType = VK_INDEX_TYPE_UINT32;
        info.IndexCount = data.GetIndexCount();

        {
            const VkMemoryMapInfo map_info
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO,
                .memory = info.VertexMemory,
                .offset = info.VertexBufferOffset,
                .size = info.VertexBufferSize,
            };

            const VkMemoryUnmapInfo unmap_info
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO,
                .memory = info.VertexMemory,
            };

            void *ptr;
            HANDLE(vk::MapMemory2(m_Device, map_info) >> ptr);

            std::memcpy(ptr, data.GetVertexData(), info.VertexBufferSize);

            HANDLE(vk::UnmapMemory2(m_Device, unmap_info));
        }

        {
            const VkMemoryMapInfo map_info
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO,
                .memory = info.IndexMemory,
                .offset = info.IndexBufferOffset,
                .size = info.IndexBufferSize,
            };

            const VkMemoryUnmapInfo unmap_info
            {
                .sType = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO,
                .memory = info.IndexMemory,
            };

            void *ptr;
            HANDLE(vk::MapMemory2(m_Device, map_info) >> ptr);

            std::memcpy(ptr, data.GetIndexData(), info.IndexBufferSize);

            HANDLE(vk::UnmapMemory2(m_Device, unmap_info));
        }
    }

    return {};
}
