#pragma once

#include <pkg.hxx>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace pkg::pipeline
{
    enum class StageName : uint8_t
    {
        Vertex,
        Fragment,
        Geometry,
        TesselationControl,
        TesselationEvaluation,
        Compute,
    };

    struct Stage
    {
        std::string Module;
        std::string Name;
    };

    enum class VertexReference : uint8_t
    {
        Position,
        Normal,
        Texture,
    };

    struct VertexAttribute
    {
        uint32_t Location;
        uint32_t Binding;
        VertexReference Reference;
    };

    struct Data
    {
        std::unordered_map<StageName, std::vector<Stage>> Stages;
        std::vector<VertexAttribute> Vertex;
    };
}

namespace pkg
{
    template<>
    struct Serializer<pipeline::StageName> : std::true_type
    {
        static void Serialize(std::ostream &stream, const pipeline::StageName &value);
        static void Deserialize(std::istream &stream, pipeline::StageName &value);
    };

    template<>
    struct Serializer<pipeline::Stage> : std::true_type
    {
        static void Serialize(std::ostream &stream, const pipeline::Stage &value);
        static void Deserialize(std::istream &stream, pipeline::Stage &value);
    };

    template<>
    struct Serializer<pipeline::VertexReference> : std::true_type
    {
        static void Serialize(std::ostream &stream, const pipeline::VertexReference &value);
        static void Deserialize(std::istream &stream, pipeline::VertexReference &value);
    };

    template<>
    struct Serializer<pipeline::VertexAttribute> : std::true_type
    {
        static void Serialize(std::ostream &stream, const pipeline::VertexAttribute &value);
        static void Deserialize(std::istream &stream, pipeline::VertexAttribute &value);
    };

    template<>
    struct Serializer<pipeline::Data> : std::true_type
    {
        static void Serialize(std::ostream &stream, const pipeline::Data &value);
        static void Deserialize(std::istream &stream, pipeline::Data &value);
    };

    template<>
    class View<pipeline::Stage> : public std::true_type
    {
    public:
        static constexpr auto size(const void *data)
        {
            const auto module_size = View<std::string_view>::size(data);
            const auto name_size = View<std::string_view>::size(static_cast<const char *>(data) + module_size);
            return module_size + name_size;
        }

        View(const void *data);

        [[nodiscard]] std::string_view GetModule() const;
        [[nodiscard]] std::string_view GetName() const;

    private:
        const void *m_Data;
    };

    template<>
    class View<pipeline::Data> : public std::true_type
    {
    public:
        View(const void *data);

        [[nodiscard]] size_t GetStageCount(pipeline::StageName key) const;
        [[nodiscard]] View<pipeline::Stage> GetStage(pipeline::StageName key, size_t index) const;

        [[nodiscard]] size_t GetVertexAttributeCount() const;
        [[nodiscard]] const pipeline::VertexAttribute *GetVertexAttributeData() const;

    private:
        const void *m_Data;
    };
}
