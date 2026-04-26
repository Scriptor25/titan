#include <titan/core.hxx>
#include <titan/utils.hxx>

#include <pkg/mesh.hxx>

#include <cstring>

toolkit::result<> titan::Application::FillBuffers()
{
    for (uint32_t i = 0; i < m_ModelData.size(); ++i)
    {
        auto &data = m_ModelData[i];
        auto &reference = m_ModelReferences[i];

        auto mesh = m_Resources.Get<pkg::mesh::Data>(data.Mesh);

        reference.VertexBufferOffset = 0;
        reference.VertexBufferSize = mesh.Vertices.size() * sizeof(pkg::mesh::Vertex);
        reference.VertexBufferStride = sizeof(pkg::mesh::Vertex);

        reference.IndexBufferOffset = 0;
        reference.IndexBufferSize = mesh.Indices.size() * sizeof(uint32_t);
        reference.IndexType = VK_INDEX_TYPE_UINT32;
        reference.IndexCount = mesh.Indices.size();

        reference.Instances.resize(data.InstanceCount);
        reference.Active.resize(data.InstanceCount);

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
            if (auto res = vk::MapMemory2(m_Device, map_info) >> ptr; !res)
                return res;

            std::memcpy(ptr, mesh.Vertices.data(), reference.VertexBufferSize);

            if (auto res = vk::UnmapMemory2(m_Device, unmap_info); !res)
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
            if (auto res = vk::MapMemory2(m_Device, map_info) >> ptr; !res)
                return res;

            std::memcpy(ptr, mesh.Indices.data(), reference.IndexBufferSize);

            if (auto res = vk::UnmapMemory2(m_Device, unmap_info); !res)
                return res;
        }
    }

    return ok();
}
