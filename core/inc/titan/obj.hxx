#pragma once

#include <titan/model.hxx>

#include <filesystem>

namespace core::obj
{
    MeshData Open(std::istream &stream);
    MeshData Open(const std::filesystem::path &path);
}
