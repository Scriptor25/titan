#include <titan/core.hxx>

#include <cstring>

core::result<> core::Application::FillVertexBuffer()
{
    const VkMemoryMapInfo memory_map_info
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO,
        .memory = m_VertexMemory,
        .offset = 0,
        .size = m_Mesh.Vertices.size() * sizeof(obj::Vertex),
    };

    void *data;
    if (auto res = vkMapMemory2(m_Device, &memory_map_info, &data))
        return error("vkMapMemory2 => {}", res);

    std::memcpy(data, m_Mesh.Vertices.data(), m_Mesh.Vertices.size() * sizeof(obj::Vertex));

    const VkMemoryUnmapInfo memory_unmap_info
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO,
        .memory = m_VertexMemory,
    };

    if (auto res = vkUnmapMemory2(m_Device, &memory_unmap_info))
        return error("vkUnmapMemory2 => {}", res);

    return ok();
}
