#include <titan/core.hxx>
#include <titan/format.hxx>

#include <fstream>

toolkit::result<std::vector<char>> titan::Application::LoadBinary(const std::filesystem::path &path)
{
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream)
        return toolkit::make_error("failed to open file '{}'", path.string());

    const auto count = stream.tellg();
    stream.seekg(0, std::ios::beg);

    std::vector<char> binary(count);
    stream.read(binary.data(), static_cast<std::streamsize>(binary.size()));

    return binary;
}

toolkit::result<> titan::Application::StoreBinary(const std::filesystem::path &path, const std::vector<char> &data)
{
    std::ofstream stream(path, std::ios::binary);
    if (!stream)
        return toolkit::make_error("failed to open file '{}'", path.string());

    stream.write(data.data(), static_cast<std::streamsize>(data.size()));
    return {};
}
