#pragma once

#include <toolkit/result.hxx>

#include <pkg.hxx>

#include <string>
#include <unordered_map>

namespace titan
{
    using ResourceID = size_t;

    struct ResourceLocation
    {
        size_t Chunk;
        size_t Offset;
        size_t Size;
    };

    struct ResourceData
    {
        size_t Size;
        void *Block;
    };

    class ResourceSystem
    {
    public:
        ResourceSystem();
        ~ResourceSystem();

        toolkit::result<ResourceID> Load(const std::string &name);
        void Discard(ResourceID id);

        ResourceData Get(ResourceID id) const;

        template<pkg::viewable T>
        auto Get(ResourceID id) const
        {
            auto [size, block] = Get(id);

            return pkg::view<T>(block);
        }

    private:
        std::unordered_map<std::string, ResourceLocation> m_Location;

        std::unordered_map<std::string, ResourceID> m_Index;
        std::unordered_map<ResourceID, ResourceData> m_Data;
    };
}
