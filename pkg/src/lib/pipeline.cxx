#include <pkg/pipeline.hxx>

void pkg::Serializer<pkg::pipeline::StageName>::Serialize(std::ostream &stream, const pipeline::StageName &value)
{
    serialize(stream, reinterpret_cast<const uint8_t &>(value));
}

void pkg::Serializer<pkg::pipeline::StageName>::Deserialize(std::istream &stream, pipeline::StageName &value)
{
    deserialize(stream, reinterpret_cast<uint8_t &>(value));
}

void pkg::Serializer<pkg::pipeline::Stage>::Serialize(std::ostream &stream, const pipeline::Stage &value)
{
    serialize(stream, value.Module);
    serialize(stream, value.Name);
}

void pkg::Serializer<pkg::pipeline::Stage>::Deserialize(std::istream &stream, pipeline::Stage &value)
{
    deserialize(stream, value.Module);
    deserialize(stream, value.Name);
}

void pkg::Serializer<pkg::pipeline::VertexReference>::Serialize(
    std::ostream &stream,
    const pipeline::VertexReference &value)
{
    serialize(stream, reinterpret_cast<const uint8_t &>(value));
}

void pkg::Serializer<pkg::pipeline::VertexReference>::Deserialize(
    std::istream &stream,
    pipeline::VertexReference &value)
{
    deserialize(stream, reinterpret_cast<uint8_t &>(value));
}

void pkg::Serializer<pkg::pipeline::VertexAttribute>::Serialize(
    std::ostream &stream,
    const pipeline::VertexAttribute &value)
{
    serialize(stream, value.Location);
    serialize(stream, value.Binding);
    serialize(stream, value.Reference);
}

void pkg::Serializer<pkg::pipeline::VertexAttribute>::Deserialize(
    std::istream &stream,
    pipeline::VertexAttribute &value)
{
    deserialize(stream, value.Location);
    deserialize(stream, value.Binding);
    deserialize(stream, value.Reference);
}

void pkg::Serializer<pkg::pipeline::Data>::Serialize(std::ostream &stream, const pipeline::Data &value)
{
    serialize(stream, value.Stages);
    serialize(stream, value.Vertex);
}

void pkg::Serializer<pkg::pipeline::Data>::Deserialize(std::istream &stream, pipeline::Data &value)
{
    deserialize(stream, value.Stages);
    deserialize(stream, value.Vertex);
}

pkg::View<pkg::pipeline::Stage>::View(const void *data)
    : m_Data(data)
{
}

std::string_view pkg::View<pkg::pipeline::Stage>::GetModule() const
{
    return *View<std::string_view>(m_Data);
}

std::string_view pkg::View<pkg::pipeline::Stage>::GetName() const
{
    return *View<std::string_view>(static_cast<const char *>(m_Data) + View<std::string_view>::size(m_Data));
}

pkg::View<pkg::pipeline::Data>::View(const void *data)
    : m_Data(data)
{
}

size_t pkg::View<pkg::pipeline::Data>::GetStageCount(const pipeline::StageName key) const
{
    const View<std::unordered_map<pipeline::StageName, std::vector<pipeline::Stage>>> stages{ m_Data };

    return stages[key].GetCount();
}

pkg::View<pkg::pipeline::Stage> pkg::View<pkg::pipeline::Data>::GetStage(
    const pipeline::StageName key,
    const size_t index) const
{
    const View<std::unordered_map<pipeline::StageName, std::vector<pipeline::Stage>>> stages{ m_Data };

    return stages[key][index];
}

size_t pkg::View<pkg::pipeline::Data>::GetVertexAttributeCount() const
{
    const View<std::vector<pipeline::VertexAttribute>> vertex
    {
        static_cast<const char *>(m_Data)
        + View<std::unordered_map<pipeline::StageName, std::vector<pipeline::Stage>>>::size(m_Data)
    };

    return vertex.GetCount();
}

const pkg::pipeline::VertexAttribute *pkg::View<pkg::pipeline::Data>::GetVertexAttributeData() const
{
    const View<std::vector<pipeline::VertexAttribute>> vertex
    {
        static_cast<const char *>(m_Data)
        + View<std::unordered_map<pipeline::StageName, std::vector<pipeline::Stage>>>::size(m_Data)
    };

    return vertex.GetData();
}
