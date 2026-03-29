#pragma once

#include <glm/glm.hpp>

#include <filesystem>
#include <vector>

namespace core::obj
{
    struct Vertex
    {
        glm::vec3 Position;
        glm::vec2 Texture;
        glm::vec3 Normal;
    };

    struct Mesh
    {
        std::vector<Vertex> Vertices;
    };

    Mesh Open(std::istream &stream);
    Mesh Open(const std::filesystem::path &path);
}
