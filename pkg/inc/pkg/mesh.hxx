#pragma once

#include <pkg.hxx>

#include <glm/glm.hpp>

#include <vector>

namespace pkg::mesh
{
    struct Vertex
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 Texture;
    };

    struct Data
    {
        glm::vec3 BoxMin, BoxMax;

        std::vector<Vertex> Vertices;
        std::vector<uint32_t> Indices;
    };
}

namespace pkg
{
    template<>
    struct Serializer<glm::vec2> : std::true_type
    {
        static void Serialize(std::ostream &stream, const glm::vec2 &value);
        static void Deserialize(std::istream &stream, glm::vec2 &value);
    };

    template<>
    struct Serializer<glm::vec3> : std::true_type
    {
        static void Serialize(std::ostream &stream, const glm::vec3 &value);
        static void Deserialize(std::istream &stream, glm::vec3 &value);
    };

    template<>
    struct Serializer<mesh::Vertex> : std::true_type
    {
        static void Serialize(std::ostream &stream, const mesh::Vertex &value);
        static void Deserialize(std::istream &stream, mesh::Vertex &value);
    };

    template<>
    struct Serializer<mesh::Data> : std::true_type
    {
        static void Serialize(std::ostream &stream, const mesh::Data &value);
        static void Deserialize(std::istream &stream, mesh::Data &value);
    };

    template<>
    class View<mesh::Data> : public std::true_type
    {
    public:
        View(const void *data);

        [[nodiscard]] const glm::vec3 &GetBoxMin() const;
        [[nodiscard]] const glm::vec3 &GetBoxMax() const;

        [[nodiscard]] size_t GetVertexCount() const;
        [[nodiscard]] const mesh::Vertex *GetVertexData() const;

        [[nodiscard]] size_t GetIndexCount() const;
        [[nodiscard]] const uint32_t *GetIndexData() const;

    private:
        const void *m_Data;

        size_t m_VertexCount;
        size_t m_IndexCount;
    };
}
