#pragma once

namespace core
{
    template<typename T>
    constexpr const char *typename_string() = delete;

    template<>
    constexpr const char *typename_string<char>()
    {
        return "char";
    }

    template<>
    constexpr const char *typename_string<short>()
    {
        return "short";
    }

    template<>
    constexpr const char *typename_string<int>()
    {
        return "int";
    }

    template<>
    constexpr const char *typename_string<long>()
    {
        return "long";
    }

    template<>
    constexpr const char *typename_string<long long>()
    {
        return "long long";
    }
}
