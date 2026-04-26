#pragma once

#include <string>
#include <type_traits>
#include <vector>

namespace pkg
{
    template<typename T>
    struct Serializer;

    template<typename T>
    concept primitive = std::integral<T> || std::floating_point<T>;

    template<typename T>
    concept complex = !primitive<T>;

    struct Writer
    {
        template<primitive T>
        auto operator()(const T value) const
        {
            Buffer.insert(
                Buffer.end(),
                reinterpret_cast<const char *>(&value),
                reinterpret_cast<const char *>(&value) + sizeof(T));
        }

        template<complex T>
        auto operator()(const T &value) const
        {
            Serializer<T>::Serialize(*this, value);
        }

        auto operator()(const std::string &value) const
        {
            operator()(value.size());
            for (auto c : value)
                Buffer.push_back(c);
        };

        auto operator()(const void *data, const size_t count) const
        {
            Buffer.insert(
                Buffer.end(),
                static_cast<const char *>(data),
                static_cast<const char *>(data) + count);
        }

        std::vector<char> &Buffer;
    };

    struct Reader
    {
        template<typename T>
            requires std::is_floating_point_v<T> || std::is_integral_v<T>
        auto operator()(T &value) const
        {
            value = *reinterpret_cast<const T *>(&Buffer[Offset]);
            Offset += sizeof(T);
        }

        template<complex T>
        auto operator()(T &value) const
        {
            Serializer<T>::Deserialize(*this, value);
        }

        auto operator()(std::string &value) const
        {
            size_t size;
            operator()(size);

            value.resize(size);
            for (auto &c : value)
                c = Buffer[Offset++];
        };

        auto operator()(void *data, const size_t count) const
        {
            for (size_t i = 0; i < count; ++i)
                static_cast<char *>(data)[i] = Buffer[Offset++];
        }

        const char *Buffer;
        size_t &Offset;
    };
}
