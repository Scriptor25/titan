#pragma once

#include <titan/result.hxx>

#include <ranges>
#include <tuple>
#include <vector>

namespace titan
{
    using EntityID = size_t;
    using ComponentID = size_t;
    using ComponentMask = __uint128_t;

    size_t GetComponentIndex(ComponentID id);
    ComponentID GetComponentID(size_t index);

    template<typename... C>
    ComponentMask GetComponentMask()
    {
        return (0ull | ... | (1ull << GetComponentIndex(C::id)));
    }

    template<bool C, typename T>
    class TypeView
    {
        using element_type = std::conditional_t<C, const T, T>;
        using value_type = std::conditional_t<C, const std::vector<uint8_t>, std::vector<uint8_t>>;

        struct Iterator
        {
            bool operator!=(const Iterator &other) const
            {
                return offset != other.offset;
            }

            element_type &operator*() const
            {
                return reinterpret_cast<element_type &>(buffer[offset]);
            }

            Iterator &operator++()
            {
                offset += stride;
                return *this;
            }

            size_t stride;
            value_type &buffer;
            size_t offset;
        };

    public:
        TypeView(const size_t stride, value_type &buffer)
            : m_Stride(stride),
              m_Buffer(buffer)
        {
        }

        Iterator begin() const
        {
            return { m_Stride, m_Buffer, 0ull };
        }

        Iterator end() const
        {
            return { m_Stride, m_Buffer, m_Buffer.size() };
        }

        [[nodiscard]] size_t size() const
        {
            return m_Buffer.size() / m_Stride;
        }

        element_type &operator[](const size_t index) const
        {
            return reinterpret_cast<element_type &>(m_Buffer[index * m_Stride]);
        }

        void push_back(T &&value)
        {
            m_Buffer.resize(m_Buffer.size() + m_Stride);

            auto &p = reinterpret_cast<element_type &>(m_Buffer[m_Buffer.size() - m_Stride]);
            std::construct_at(&p, std::forward<T>(value));
        }

    private:
        size_t m_Stride;
        value_type &m_Buffer;
    };

    class ComponentStorage
    {
    public:
        ComponentStorage();
        explicit ComponentStorage(size_t stride);

        ComponentStorage(const ComponentStorage &) = delete;
        ComponentStorage &operator=(const ComponentStorage &) = delete;

        ComponentStorage(ComponentStorage &&other) noexcept;
        ComponentStorage &operator=(ComponentStorage &&other) noexcept;

        [[nodiscard]] size_t GetStride() const;

        void Allocate(size_t count = 1ull);

        template<typename T>
        void Allocate(T &&value)
        {
            Cast<T>().push_back(std::forward<T>(value));
        }

        void Release(size_t count = 1ull);

        void Get(size_t index, void * &data, size_t &size);
        void Get(size_t index, const void * &data, size_t &size) const;

        template<typename T>
        T &Get(const size_t index)
        {
            return Cast<T>()[index];
        }

        template<typename T>
        const T &Get(const size_t index) const
        {
            return Cast<T>()[index];
        }

        void Set(size_t index, void *data, size_t size);

        template<typename T>
        void Set(const size_t index, T &&value)
        {
            auto &p = Cast<T>()[index];
            std::construct_at(&p, std::forward<T>(value));
        }

        void Erase(size_t index);

        template<typename T>
        TypeView<false, T> Cast()
        {
            if (sizeof(T) != m_Stride)
                throw std::runtime_error("size != stride");

            return { m_Stride, m_Buffer };
        }

        template<typename T>
        TypeView<true, T> Cast() const
        {
            if (sizeof(T) != m_Stride)
                throw std::runtime_error("size != stride");

            return { m_Stride, m_Buffer };
        }

    private:
        size_t m_Stride;
        std::vector<uint8_t> m_Buffer;
    };

    class Archetype
    {
    public:
        Archetype();
        Archetype(ComponentMask mask, const std::unordered_map<ComponentID, size_t> &strides);

        Archetype(const Archetype &) = delete;
        Archetype &operator=(const Archetype &) = delete;

        Archetype(Archetype &&other) noexcept;
        Archetype &operator=(Archetype &&other) noexcept;

        [[nodiscard]] ComponentMask GetMask() const;
        [[nodiscard]] size_t GetCount() const;

        [[nodiscard]] std::unordered_map<ComponentID, size_t> GetStrides() const;

        [[nodiscard]] bool Matches(ComponentMask mask) const;

        ComponentStorage &GetColumn(ComponentID id);
        const ComponentStorage &GetColumn(ComponentID id) const;

        template<typename C>
        auto GetColumn()
        {
            return GetColumn(C::id).template Cast<C>();
        }

        template<typename C>
        auto GetColumn() const
        {
            return GetColumn(C::id).template Cast<C>();
        }

        void Get(EntityID entity, ComponentID id, void * &data, size_t &size);
        void Get(EntityID entity, ComponentID id, const void * &data, size_t &size) const;

        template<typename C>
        C &Get(const EntityID entity, const ComponentID id)
        {
            return m_Storage.at(id).Get<C>(m_Index.at(entity));
        }

        template<typename C>
        const C &Get(const EntityID entity, const ComponentID id) const
        {
            return m_Storage.at(id).Get<C>(m_Index.at(entity));
        }

        void Set(EntityID entity, ComponentID id, void *data, size_t size);

        template<typename C>
        void Set(const EntityID entity, const ComponentID id, C &&value)
        {
            m_Storage.at(id).Set<C>(m_Index.at(entity), std::forward<C>(value));
        }

        void Allocate(EntityID entity);

        template<typename... C>
        void Allocate(const EntityID entity, C &&... components)
        {
            const auto index = m_Entities.size();

            m_Entities.push_back(entity);
            m_Index[entity] = index;

            for (auto &[id, storage] : m_Storage)
            {
                if (([&] -> bool
                {
                    if (C::id != id)
                        return false;

                    storage.Allocate(std::forward<C>(components));
                    return true;
                }() || ...))
                    continue;

                storage.Allocate();
            }
        }

        void Release(EntityID entity);

    private:
        ComponentMask m_Mask;

        std::vector<EntityID> m_Entities;
        std::unordered_map<EntityID, size_t> m_Index;

        std::unordered_map<ComponentID, ComponentStorage> m_Storage;
    };

    template<typename... C>
    class QueryResult
    {
        using value_type = std::tuple<std::vector<C *>...>;

        struct Iterator
        {
            template<size_t ... I>
            std::tuple<C &...> extract(std::index_sequence<I...>) const
            {
                return { *std::get<I>(data)[offset]... };
            }

            bool operator!=(const Iterator &other) const
            {
                return offset != other.offset;
            }

            std::tuple<C &...> operator*() const
            {
                return extract(std::index_sequence_for<C...>());
            }

            Iterator &operator++()
            {
                ++offset;
                return *this;
            }

            const value_type &data;
            size_t offset;
        };

    public:
        QueryResult(const size_t count, value_type data)
            : m_Count(count),
              m_Data(std::move(data))
        {
        }

        Iterator begin() const
        {
            return { m_Data, 0ull };
        }

        Iterator end() const
        {
            return { m_Data, m_Count };
        }

    private:
        size_t m_Count;
        value_type m_Data;
    };

    class ECS
    {
    public:
        template<typename... C>
        Archetype &GetArchetype()
        {
            const auto mask = GetComponentMask<C...>();

            if (const auto it = m_Archetypes.find(mask); it != m_Archetypes.end())
                return it->second;

            return m_Archetypes[mask] = { mask, { { C::id, sizeof(C) }... } };
        }

        Archetype &GetArchetype(ComponentMask mask, const std::unordered_map<ComponentID, size_t> &strides);

        template<typename... C>
        QueryResult<C...> Query()
        {
            const auto mask = GetComponentMask<C...>();

            size_t count{};
            std::tuple<std::vector<C *>...> result;

            for (auto &archetype : m_Archetypes | std::views::values)
            {
                if (!archetype.Matches(mask))
                    continue;

                count += archetype.GetCount();

                auto extract = [&]<size_t ... I>(std::index_sequence<I...>)
                {
                    ([&]
                    {
                        auto &dst = std::get<I>(result);
                        auto &src = archetype.GetColumn(C::id);

                        for (auto cast = src.template Cast<C>(); auto &component : cast)
                            dst.push_back(&component);
                    }(), ...);
                };

                extract(std::index_sequence_for<C...>());
            }

            return { count, std::move(result) };
        }

        template<typename... C>
        EntityID Create(C &&... components)
        {
            auto &archetype = GetArchetype<C...>();
            auto entity = m_Entities.size();

            m_Entities[entity] = &archetype;

            archetype.Allocate(entity, std::forward<C>(components)...);

            return entity;
        }

        void Destroy(const EntityID entity)
        {
            auto &archetype = *m_Entities.at(entity);

            archetype.Release(entity);
            m_Entities.erase(entity);
        }

        template<typename C>
        void Add(EntityID entity, C &&component)
        {
            auto &src = *m_Entities.at(entity);

            auto strides = src.GetStrides();
            strides[C::id] = sizeof(C);

            auto &dst = GetArchetype(src.GetMask() | GetComponentMask<C>(), strides);

            dst.Allocate(entity, std::forward<C>(component));

            auto mask = src.GetMask();
            for (size_t index{}; mask; ++index, mask >>= 1)
                if (mask & 1)
                {
                    const auto id = GetComponentID(index);

                    void *data;
                    size_t size;

                    src.Get(entity, id, data, size);
                    dst.Set(entity, id, data, size);
                }

            src.Release(entity);

            m_Entities[entity] = &dst;
        }

        template<typename C>
        void Remove(EntityID entity)
        {
            auto &src = *m_Entities.at(entity);

            auto strides = src.GetStrides();
            strides.erase(C::id);

            auto &dst = GetArchetype(src.GetMask() & ~GetComponentMask<C>());

            dst.Allocate(entity);

            auto mask = dst.GetMask();
            for (size_t index{}; mask; ++index, mask >>= 1)
                if (mask & 1)
                {
                    const auto id = GetComponentID(index);

                    void *data;
                    size_t size;

                    src.Get(entity, id, data, size);
                    dst.Set(entity, id, data, size);
                }

            src.Release(entity);

            m_Entities[entity] = &dst;
        }

    private:
        std::unordered_map<ComponentMask, Archetype> m_Archetypes;
        std::unordered_map<EntityID, Archetype *> m_Entities;
    };
}
