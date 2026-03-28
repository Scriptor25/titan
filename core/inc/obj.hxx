#pragma once

#include <filesystem>
#include <istream>
#include <vector>

#include <glm/glm.hpp>

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
