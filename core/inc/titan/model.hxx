#pragma once

#include <glm/glm.hpp>

#include <vector>

namespace titan
{
    struct VertexData
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 Texture;
    };

    struct MeshData
    {
        std::vector<VertexData> Vertices;
        std::vector<uint32_t> Indices;
    };

    struct ModelData
    {
        MeshData Mesh;
        uint32_t InstanceCount{};
        /* MaterialData Material; */
        /* TransformData Transform; */
    };
}
