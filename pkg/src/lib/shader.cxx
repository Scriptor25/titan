#include <pkg/shader.hxx>

void pkg::Serializer<pkg::shader::Data>::Serialize(std::ostream &stream, const shader::Data &value)
{
    serialize(stream, value.Binary);
}

void pkg::Serializer<pkg::shader::Data>::Deserialize(std::istream &stream, shader::Data &value)
{
    deserialize(stream, value.Binary);
}

pkg::View<pkg::shader::Data>::View(const void *data)
    : m_Data(data)
{
}

size_t pkg::View<pkg::shader::Data>::GetBinarySize() const
{
    return *static_cast<const size_t *>(m_Data);
}

const void *pkg::View<pkg::shader::Data>::GetBinaryData() const
{
    return static_cast<const char *>(m_Data) + sizeof(size_t);
}
