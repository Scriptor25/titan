#pragma once

#include <pkg/mesh.hxx>

#include <filesystem>

namespace pkg::mesh::obj
{
    void Open(std::istream &stream, Data &data);
    bool Open(const std::filesystem::path &path, Data &data);
}
