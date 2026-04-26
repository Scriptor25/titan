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
    struct Serializer<mesh::Vertex>
    {
        template<typename W>
        static void Serialize(W &&write, const mesh::Vertex &value)
        {
            write(value.Position.x);
            write(value.Position.y);
            write(value.Position.z);

            write(value.Normal.x);
            write(value.Normal.y);
            write(value.Normal.z);

            write(value.Texture.x);
            write(value.Texture.y);
        }

        template<typename R>
        static void Deserialize(R &&read, mesh::Vertex &value)
        {
            read(value.Position.x);
            read(value.Position.y);
            read(value.Position.z);

            read(value.Normal.x);
            read(value.Normal.y);
            read(value.Normal.z);

            read(value.Texture.x);
            read(value.Texture.y);
        }
    };

    template<>
    struct Serializer<mesh::Data>
    {
        template<typename W>
        static void Serialize(W &&write, const mesh::Data &value)
        {
            write(value.BoxMin.x);
            write(value.BoxMin.y);
            write(value.BoxMin.z);

            write(value.BoxMax.x);
            write(value.BoxMax.y);
            write(value.BoxMax.z);

            write(value.Vertices.size());
            for (auto &vertex : value.Vertices)
                write(vertex);

            write(value.Indices.size());
            for (auto &index : value.Indices)
                write(index);
        }

        template<typename R>
        static void Deserialize(R &&read, mesh::Data &value)
        {
            read(value.BoxMin.x);
            read(value.BoxMin.y);
            read(value.BoxMin.z);

            read(value.BoxMax.x);
            read(value.BoxMax.y);
            read(value.BoxMax.z);

            size_t vertices_size;
            read(vertices_size);

            value.Vertices.resize(vertices_size);
            for (auto &vertex : value.Vertices)
                read(vertex);

            size_t indices_size;
            read(indices_size);

            value.Indices.resize(indices_size);
            for (auto &index : value.Indices)
                read(index);
        }
    };
}
