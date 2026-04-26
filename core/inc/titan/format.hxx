#pragma once

#include <format>
#include <string>
#include <vector>

template<>
struct std::formatter<std::vector<std::string>> : std::formatter<std::string>
{
    template<class C>
    C::iterator format(const std::vector<std::string> &v, C &context) const
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

template<typename T>
struct std::formatter<std::vector<T>> : std::formatter<std::vector<std::string>>
{
    template<class C>
    C::iterator format(const std::vector<T> &v, C &context) const
    {
        std::vector<std::string> segments(v.size());
        for (size_t i = 0; i < v.size(); ++i)
            segments[i] = std::format("{}", v[i]);

        return std::formatter<std::vector<std::string>>::format(segments, context);
    }
};

template<typename T>
struct std::formatter<std::set<T>> : std::formatter<std::vector<std::string>>
{
    template<class C>
    C::iterator format(const std::set<T> &v, C &context) const
    {
        std::vector<std::string> segments;
        segments.reserve(v.size());
        for (auto &e : v)
            segments.push_back(std::format("{}", e));

        return std::formatter<std::vector<std::string>>::format(segments, context);
    }
};
