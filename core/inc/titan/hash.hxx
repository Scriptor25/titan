#pragma once

#include <cstdint>
#include <string_view>

namespace titan
{
    constexpr uint64_t mix64(uint64_t x)
    {
        x ^= x >> 32;
        x *= 0xd6e8feb86659fd93ull;
        x ^= x >> 32;
        x *= 0xd6e8feb86659fd93ull;
        x ^= x >> 32;
        return x;
    }

    constexpr uint64_t read64(const char *data, const size_t len)
    {
        uint64_t v = 0;
        for (size_t i = 0; i < len; ++i)
            v |= static_cast<uint64_t>(static_cast<unsigned char>(data[i])) << (i * 8);
        return v;
    }

    constexpr uint64_t hash64(const std::string_view str)
    {
        uint64_t h = 0x9e3779b97f4a7c15ull;

        size_t i = 0;
        while (i + 8 <= str.size())
        {
            const auto chunk = read64(str.data() + i, 8);
            h ^= mix64(chunk);
            h *= 0x9e3779b97f4a7c15ull;
            i += 8;
        }

        if (i < str.size())
        {
            const auto tail = read64(str.data() + i, str.size() - i);
            h ^= mix64(tail);
        }

        return mix64(h ^ str.size());
    }
}

constexpr uint64_t operator""_hash64(const char *str, size_t len)
{
    return titan::hash64({ str, len });
}
