#include <titan/core.hxx>

core::result<> core::Application::CreateBuffers()
{
    const VkBufferCreateInfo create_info
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = m_Mesh.Vertices.size() * sizeof(obj::Vertex),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    return vk::Buffer::create(m_Device, create_info) >> m_VertexBuffer;
}
