#include <titan/core.hxx>
#include <titan/utils.hxx>

#include <cstring>

core::result<> core::Application::FillVertexBuffer()
{
    const VkMemoryMapInfo map_info
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO,
        .memory = m_VertexMemory,
        .offset = 0,
        .size = m_Mesh.Vertices.size() * sizeof(obj::Vertex),
    };

    const VkMemoryUnmapInfo unmap_info
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO,
        .memory = m_VertexMemory,
    };

    return vk::MapMemory2(m_Device, map_info)
           & [&](void * &&data)
           {
               std::memcpy(data, m_Mesh.Vertices.data(), m_Mesh.Vertices.size() * sizeof(obj::Vertex));

               return vk::UnmapMemory2(m_Device, unmap_info);
           };
}
