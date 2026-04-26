#include <titan/system/resource.hxx>

#include <toolkit/result.hxx>

#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <ranges>

titan::ResourceSystem::ResourceSystem()
{
    std::ifstream stream("res/index", std::ios::binary);
    if (!stream)
        return;

    for (std::string key; std::getline(stream, key, '\n');)
    {
        std::string chunk, offset, size;
        std::getline(stream, chunk, '\n');
        std::getline(stream, offset, '\n');
        std::getline(stream, size, '\n');

        m_Location[key] = {
            .Chunk = std::stoull(chunk, nullptr, 0x10),
            .Offset = std::stoull(offset, nullptr, 0x10),
            .Size = std::stoull(size, nullptr, 0x10),
        };
    }
}

titan::ResourceSystem::~ResourceSystem()
{
    for (auto &[size, block] : m_Data | std::views::values)
    {
        if (size && block)
            operator delete(block, size);

        size = {};
        block = {};
    }

    m_Data.clear();
}

toolkit::result<titan::ResourceID> titan::ResourceSystem::Load(const std::string &name)
{
    if (const auto it = m_Index.find(name); it != m_Index.end())
        return it->second;

    // location[name] = { <chunk>, <offset>, <size> }
    // { <chunk>, <offset>, <size> } -> { data... }

    auto it = m_Location.find(name);
    if (it == m_Location.end())
        return toolkit::make_error("undefined resource name '{}'", name);

    auto &[chunk, offset, size] = it->second;

    auto path = std::filesystem::current_path() / "res" / std::format("chunk{:02x}", chunk);

    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        return toolkit::make_error("failed to open '{}'", path.string());

    stream.seekg(static_cast<std::ios::off_type>(offset), std::ios::beg);
    if (!stream)
        return toolkit::make_error("failed to seek offset {} in '{}'", offset, path.string());

    std::vector<char> data(size);
    stream.read(data.data(), static_cast<std::streamsize>(data.size()));
    if (!stream)
        return toolkit::make_error("failed to read {} bytes at {} in '{}'", size, offset, path.string());

    auto block = operator new(size);
    std::memcpy(block, data.data(), data.size());

    auto id = m_Index.size();
    m_Index[name] = id;
    m_Data[id] = {
        .Size = size,
        .Block = block,
    };

    return id;
}

void titan::ResourceSystem::Discard(const ResourceID id)
{
    if (const auto it = m_Data.find(id); it != m_Data.end())
    {
        auto &[size, block] = it->second;

        if (size && block)
            operator delete(block, size);

        size = {};
        block = {};

        m_Data.erase(it);
    }
}

titan::ResourceData titan::ResourceSystem::Get(const ResourceID id) const
{
    return m_Data.at(id);
}
