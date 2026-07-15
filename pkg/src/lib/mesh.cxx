#include <pkg/mesh.hxx>

void pkg::Serializer<glm::vec2>::Serialize(std::ostream &stream, const glm::vec2 &value)
{
    serialize(stream, value.x);
    serialize(stream, value.y);
}

void pkg::Serializer<glm::vec2>::Deserialize(std::istream &stream, glm::vec2 &value)
{
    deserialize(stream, value.x);
    deserialize(stream, value.y);
}

void pkg::Serializer<glm::vec3>::Serialize(std::ostream &stream, const glm::vec3 &value)
{
    serialize(stream, value.x);
    serialize(stream, value.y);
    serialize(stream, value.z);
}

void pkg::Serializer<glm::vec3>::Deserialize(std::istream &stream, glm::vec3 &value)
{
    deserialize(stream, value.x);
    deserialize(stream, value.y);
    deserialize(stream, value.z);
}

void pkg::Serializer<pkg::mesh::Vertex>::Serialize(std::ostream &stream, const mesh::Vertex &value)
{
    serialize(stream, value.Position);
    serialize(stream, value.Normal);
    serialize(stream, value.Texture);
}

void pkg::Serializer<pkg::mesh::Vertex>::Deserialize(std::istream &stream, mesh::Vertex &value)
{
    deserialize(stream, value.Position);
    deserialize(stream, value.Normal);
    deserialize(stream, value.Texture);
}

void pkg::Serializer<pkg::mesh::Data>::Serialize(std::ostream &stream, const mesh::Data &value)
{
    serialize(stream, value.BoxMin);
    serialize(stream, value.BoxMax);
    serialize(stream, value.Vertices);
    serialize(stream, value.Indices);
}

void pkg::Serializer<pkg::mesh::Data>::Deserialize(std::istream &stream, mesh::Data &value)
{
    deserialize(stream, value.BoxMin);
    deserialize(stream, value.BoxMax);
    deserialize(stream, value.Vertices);
    deserialize(stream, value.Indices);
}

pkg::View<pkg::mesh::Data>::View(const void *data)
    : m_Data(data)
{
    m_VertexCount = *reinterpret_cast<const size_t *>(
        static_cast<const char *>(m_Data)
        + 2 * sizeof(glm::vec3)
    );

    m_IndexCount = *reinterpret_cast<const size_t *>(
        static_cast<const char *>(m_Data)
        + 2 * sizeof(glm::vec3)
        + sizeof(size_t)
        + m_VertexCount * sizeof(mesh::Vertex)
    );
}

const glm::vec3 &pkg::View<pkg::mesh::Data>::GetBoxMin() const
{
    return *(static_cast<const glm::vec3 *>(m_Data) + 0);
}

const glm::vec3 &pkg::View<pkg::mesh::Data>::GetBoxMax() const
{
    return *(static_cast<const glm::vec3 *>(m_Data) + 1);
}

size_t pkg::View<pkg::mesh::Data>::GetVertexCount() const
{
    return m_VertexCount;
}

const pkg::mesh::Vertex *pkg::View<pkg::mesh::Data>::GetVertexData() const
{
    return reinterpret_cast<const mesh::Vertex *>(
        static_cast<const char *>(m_Data)
        + 2 * sizeof(glm::vec3)
        + sizeof(size_t)
    );
}

size_t pkg::View<pkg::mesh::Data>::GetIndexCount() const
{
    return m_IndexCount;
}

const uint32_t *pkg::View<pkg::mesh::Data>::GetIndexData() const
{
    return reinterpret_cast<const uint32_t *>(
        static_cast<const char *>(m_Data)
        + 2 * sizeof(glm::vec3)
        + sizeof(size_t)
        + m_VertexCount * sizeof(mesh::Vertex)
        + sizeof(size_t)
    );
}
