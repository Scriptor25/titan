#pragma once

#include <istream>
#include <ostream>
#include <unordered_map>
#include <vector>

namespace pkg
{
    template<typename>
    struct Serializer : std::false_type
    {
    };

    template<typename T>
    concept serializable = Serializer<T>::value;

    template<typename>
    class View : public std::false_type
    {
    };

    template<typename T>
    concept viewable = View<T>::value;

    template<serializable T>
    auto serialize(std::ostream &stream, const T &value)
    {
        return Serializer<T>::Serialize(stream, value);
    }

    template<serializable T>
    auto deserialize(std::istream &stream, T &value)
    {
        return Serializer<T>::Deserialize(stream, value);
    }

    template<viewable T>
    View<T> view(const void *data)
    {
        return { data };
    }

    template<std::integral T>
    struct Serializer<T> : std::true_type
    {
        static auto Serialize(std::ostream &stream, const T &value)
        {
            stream.write(reinterpret_cast<const char *>(&value), sizeof(T));
        }

        static auto Deserialize(std::istream &stream, T &value)
        {
            stream.read(reinterpret_cast<char *>(&value), sizeof(T));
        }
    };

    template<std::floating_point T>
    struct Serializer<T> : std::true_type
    {
        static auto Serialize(std::ostream &stream, const T &value)
        {
            stream.write(reinterpret_cast<const char *>(&value), sizeof(T));
        }

        static auto Deserialize(std::istream &stream, T &value)
        {
            stream.read(reinterpret_cast<char *>(&value), sizeof(T));
        }
    };

    template<serializable T>
    struct Serializer<std::vector<T>> : std::true_type
    {
        static auto Serialize(std::ostream &stream, const std::vector<T> &value)
        {
            serialize(stream, value.size());
            for (auto &entry : value)
                serialize(stream, entry);
        }

        static auto Deserialize(std::istream &stream, std::vector<T> &value)
        {
            size_t size;
            deserialize(stream, size);

            value.resize(size);
            for (auto &entry : value)
                deserialize(stream, entry);
        }
    };

    template<typename T>
    class View<std::vector<T>> : public std::true_type
    {
    public:
        static constexpr auto size(const void *data)
        {
            const auto count = *static_cast<const size_t *>(data);

            return sizeof(size_t) + count * sizeof(T);
        }

        View(const void *data)
            : m_Data(data)
        {
            m_Count = *static_cast<const size_t *>(m_Data);
        }

        [[nodiscard]] size_t GetCount() const
        {
            return m_Count;
        }

        [[nodiscard]] const T *GetData() const
        {
            return reinterpret_cast<const T *>(static_cast<const char *>(m_Data) + sizeof(size_t));
        }

        [[nodiscard]] const T &operator[](size_t index) const
        {
            if (index >= m_Count)
                throw std::runtime_error("index out of bounds");

            return reinterpret_cast<const T *>(static_cast<const char *>(m_Data) + sizeof(size_t))[index];
        }

    private:
        const void *m_Data;

        size_t m_Count;
    };

    template<viewable T>
    class View<std::vector<T>> : public std::true_type
    {
    public:
        static constexpr auto size(const void *data)
        {
            const auto count = *static_cast<const size_t *>(data);

            auto offset = sizeof(size_t);

            for (size_t i = 0; i < count; ++i)
            {
                auto *base = static_cast<const char *>(data) + offset;

                offset += View<T>::size(base);
            }

            return offset;
        }

        View(const void *data)
            : m_Data(data)
        {
            m_Count = *static_cast<const size_t *>(m_Data);
        }

        [[nodiscard]] size_t GetCount() const
        {
            return m_Count;
        }

        [[nodiscard]] View<T> operator[](size_t index) const
        {
            if (index >= m_Count)
                throw std::runtime_error("index out of bounds");

            auto offset = sizeof(size_t);

            for (size_t i = 0; i < index; ++i)
                offset += View<T>::size(static_cast<const char *>(m_Data) + offset);

            return { static_cast<const char *>(m_Data) + offset };
        }

    private:
        const void *m_Data;

        size_t m_Count;
    };

    template<serializable K, serializable V>
    struct Serializer<std::unordered_map<K, V>> : std::true_type
    {
        static auto Serialize(std::ostream &stream, const std::unordered_map<K, V> &value)
        {
            serialize(stream, value.size());
            for (auto &[key, val] : value)
            {
                serialize(stream, key);
                serialize(stream, val);
            }
        }

        static auto Deserialize(std::istream &stream, std::unordered_map<K, V> &value)
        {
            size_t size;
            deserialize(stream, size);

            value.reserve(size);
            for (size_t i = 0; i < size; ++i)
            {
                K key;
                deserialize(stream, key);

                V val;
                deserialize(stream, val);

                value[std::move(key)] = std::move(val);
            }
        }
    };

    template<typename K, typename V>
    class View<std::unordered_map<K, V>> : public std::true_type
    {
    public:
        static constexpr auto size(const void *data)
        {
            const auto count = *static_cast<const size_t *>(data);

            if constexpr (viewable<K>)
            {
                auto offset = sizeof(size_t);

                for (size_t i = 0; i < count; ++i)
                {
                    auto *base = static_cast<const char *>(data) + offset;

                    offset += View<K>::size(base) + sizeof(V);
                }

                return offset;
            }
            else
            {
                return sizeof(size_t) + count * (sizeof(K) + sizeof(V));
            }
        }

        View(const void *data)
            : m_Data(data)
        {
            m_Count = *static_cast<const size_t *>(m_Data);
        }

        [[nodiscard]] const V &operator[](const K &key) const
        {
            auto offset = sizeof(size_t);

            for (size_t i = 0; i < m_Count; ++i)
            {
                auto *base = static_cast<const char *>(m_Data) + offset;

                size_t key_size;
                if constexpr (viewable<K>)
                {
                    key_size = View<K>::size(base);
                }
                else
                {
                    key_size = sizeof(K);
                }

                offset += key_size + sizeof(V);

                bool match;
                if constexpr (viewable<K>)
                {
                    View<K> ikey{ base };
                    match = ikey == key;
                }
                else
                {
                    auto &ikey = *reinterpret_cast<const K *>(base);
                    match = ikey == key;
                }

                if (match)
                    return *reinterpret_cast<const V *>(base + key_size);
            }

            throw std::runtime_error("key not defined");
        }

    private:
        const void *m_Data;

        size_t m_Count;
    };

    template<typename K, viewable V>
    class View<std::unordered_map<K, V>> : public std::true_type
    {
    public:
        static constexpr auto size(const void *data)
        {
            const auto count = *static_cast<const size_t *>(data);

            auto offset = sizeof(size_t);

            for (size_t i = 0; i < count; ++i)
            {
                auto *base = static_cast<const char *>(data) + offset;

                size_t key_size;
                if constexpr (viewable<K>)
                {
                    key_size = View<K>::size(base);
                }
                else
                {
                    key_size = sizeof(K);
                }

                offset += key_size + View<V>::size(base + key_size);
            }

            return offset;
        }

        View(const void *data)
            : m_Data(data)
        {
            m_Count = *static_cast<const size_t *>(m_Data);
        }

        [[nodiscard]] View<V> operator[](const K &key) const
        {
            auto offset = sizeof(size_t);

            for (size_t i = 0; i < m_Count; ++i)
            {
                auto *base = static_cast<const char *>(m_Data) + offset;

                size_t key_size;
                if constexpr (viewable<K>)
                {
                    key_size = View<K>::size(base);
                }
                else
                {
                    key_size = sizeof(K);
                }

                offset += key_size + View<V>::size(base + key_size);

                bool match;
                if constexpr (viewable<K>)
                {
                    View<K> ikey{ base };
                    match = ikey == key;
                }
                else
                {
                    auto &ikey = *reinterpret_cast<const K *>(base);
                    match = ikey == key;
                }

                if (match)
                    return { base + key_size };
            }

            throw std::runtime_error("key not defined");
        }

    private:
        const void *m_Data;

        size_t m_Count;
    };

    template<typename C>
    struct Serializer<std::basic_string<C>> : std::true_type
    {
        static auto Serialize(std::ostream &stream, const std::basic_string<C> &value)
        {
            serialize(stream, value.size());
            for (auto &entry : value)
                serialize(stream, entry);
        }

        static auto Deserialize(std::istream &stream, std::basic_string<C> &value)
        {
            size_t size;
            deserialize(stream, size);

            value.resize(size);
            for (auto &entry : value)
                deserialize(stream, entry);
        }
    };

    template<typename C>
    class View<std::basic_string_view<C>> : std::true_type
    {
    public:
        static constexpr auto size(const void *data)
        {
            return sizeof(size_t) + *static_cast<const size_t *>(data) * sizeof(C);
        }

        View(const void *data)
            : m_Data(data)
        {
            m_Count = *static_cast<const size_t *>(m_Data);
        }

        [[nodiscard]] std::basic_string_view<C> operator*() const
        {
            auto *begin = static_cast<const C *>(static_cast<const char *>(m_Data) + sizeof(size_t));
            auto *end = begin + m_Count;

            return { begin, end };
        }

    private:
        const void *m_Data;

        size_t m_Count;
    };
}
