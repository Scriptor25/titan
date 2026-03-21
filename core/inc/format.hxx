#pragma once

#include <format>
#include <string>
#include <vector>

template<typename T>
struct std::formatter<std::vector<T>> : std::formatter<std::string>
{
    template<class C>
    C::iterator format(const std::vector<T> &v, C &context) const
    {
        std::string s;
        for (auto it = v.begin(); it != v.end(); ++it)
        {
            if (it != v.begin())
            {
                if (it != v.end() - 1)
                    s += ", ";
                else
                    s += " and ";
            }
            s += std::format("{}", *it);
        }
        return std::formatter<std::string>::format(s, context);
    }
};

template<>
struct std::formatter<std::vector<std::basic_string_view<char>>> : std::formatter<std::string>
{
    template<class C>
    C::iterator format(const std::vector<std::basic_string_view<char>> &v, C &context) const
    {
        std::string s;
        for (auto it = v.begin(); it != v.end(); ++it)
        {
            if (it != v.begin())
            {
                if (it != v.end() - 1)
                    s += ", ";
                else
                    s += " and ";
            }
            s += *it;
        }
        return std::formatter<std::string>::format(s, context);
    }
};
