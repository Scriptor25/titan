#pragma once

#include <algorithm>
#include <ranges>
#include <titan/result.hxx>

#include <tuple>
#include <vector>

namespace titan
{
    using EntityID = size_t;
    using ComponentID = size_t;
    using component_mask_t = size_t;

    template<typename T>
    struct component_traits_t;

    template<typename... C>
    constexpr component_mask_t get_component_mask()
    {
        return (0ull | ... | (1ull << component_traits_t<C>::id));
    }

    template<typename T>
    struct converter_t
    {
        struct iterator
        {
            bool operator!=(const iterator &it) const
            {
                return offset != it.offset;
            }

            T &operator*() const
            {
                return *reinterpret_cast<T *>(&data[offset]);
            }

            iterator &operator++()
            {
                offset += stride;
                return *this;
            }

            size_t stride;
            std::vector<uint8_t> &data;
            size_t offset;
        };

        iterator begin()
        {
            return { stride, data, 0 };
        }

        iterator end()
        {
            return { stride, data, data.size() };
        }

        size_t stride;
        std::vector<uint8_t> &data;
    };

    template<typename T>
    struct const_converter_t
    {
        struct const_iterator
        {
            bool operator!=(const const_iterator &it) const
            {
                return stride != it.stride || &data != &it.data || offset != it.offset;
            }

            const T &operator*() const
            {
                return *reinterpret_cast<const T *>(&data[offset]);
            }

            const_iterator &operator++()
            {
                offset += stride;
                return *this;
            }

            size_t stride;
            const std::vector<uint8_t> &data;
            size_t offset;
        };

        const_iterator begin() const
        {
            return { stride, data, 0 };
        }

        const_iterator end() const
        {
            return { stride, data, data.size() };
        }

        size_t stride;
        const std::vector<uint8_t> &data;
    };

    class component_storage_t
    {
    public:
        component_storage_t()
            : m_Stride()
        {
        }

        explicit component_storage_t(const size_t stride)
            : m_Stride(stride)
        {
        }

        [[nodiscard]] size_t stride() const
        {
            return m_Stride;
        }

        void allocate(const size_t count = 1)
        {
            m_Data.resize(m_Data.size() + count * m_Stride);
        }

        void release()
        {
            m_Data.resize(m_Data.size() - m_Stride);
        }

        void push_back(uint8_t *data, const size_t size)
        {
            if (size % m_Stride)
                throw std::runtime_error("data size is not a multiple of storage stride");

            allocate(size / m_Stride);
            std::move(data, data + size, m_Data.end() - m_Stride);
        }

        void push_back(std::vector<uint8_t> &&data)
        {
            if (data.size() % m_Stride)
                throw std::runtime_error("data size is not a multiple of storage stride");

            allocate(data.size() / m_Stride);
            std::ranges::move(std::forward<decltype(data)>(data), m_Data.end() - m_Stride);
        }

        void set(size_t index, uint8_t *data, const size_t size)
        {
            if (size % m_Stride)
                throw std::runtime_error("data size is not a multiple of storage stride");

            std::move(data, data + size, m_Data.begin() + index * m_Stride);
        }

        void erase(const size_t index)
        {
            const auto offset = index * m_Stride;
            m_Data.erase(m_Data.begin() + offset, m_Data.begin() + offset + m_Stride);
        }

        uint8_t *get(const size_t index)
        {
            return &m_Data[index * m_Stride];
        }

        template<typename T>
            requires std::is_trivially_copyable_v<T>
        converter_t<T> cast()
        {
            return { m_Stride, m_Data };
        }

        template<typename T>
            requires std::is_trivially_copyable_v<T>
        const_converter_t<T> cast() const
        {
            return { m_Stride, m_Data };
        }

    private:
        size_t m_Stride;
        std::vector<uint8_t> m_Data;
    };

    class archetype_t
    {
    public:
        archetype_t()
            : m_Mask()
        {
        }

        archetype_t(const component_mask_t mask, const std::unordered_map<ComponentID, size_t> &strides)
            : m_Mask(mask)
        {
            for (auto &[id, stride] : strides)
                m_Storage[id] = component_storage_t(stride);
        }

        [[nodiscard]] component_mask_t mask() const
        {
            return m_Mask;
        }

        std::unordered_map<ComponentID, size_t> strides() const
        {
            std::unordered_map<ComponentID, size_t> strides;
            for (auto &[id, storage] : m_Storage)
                strides[id] = storage.stride();
            return strides;
        }

        [[nodiscard]] bool matches(const component_mask_t mask) const
        {
            return (mask & m_Mask) == mask;
        }

        [[nodiscard]] size_t size() const
        {
            return m_Index.size();
        }

        const component_storage_t &get_column(const ComponentID id) const
        {
            return m_Storage.at(id);
        }

        void get(const EntityID entity, const ComponentID id, uint8_t * &data, size_t &size)
        {
            auto &storage = m_Storage.at(id);
            data = storage.get(m_Index.at(entity));
            size = storage.stride();
        }

        void set(const EntityID entity, const ComponentID id, uint8_t *data, const size_t size)
        {
            auto &storage = m_Storage.at(id);
            storage.set(m_Index.at(entity), data, size);
        }

        void allocate(const EntityID entity)
        {
            const auto index = m_Entities.size();

            m_Entities.push_back(entity);
            m_Index[entity] = index;

            for (auto &storage : m_Storage | std::views::values)
                storage.allocate();
        }

        void release(const EntityID entity)
        {
            const auto index = m_Index.at(entity);
            const auto last = m_Entities.size() - 1;

            if (index != last)
            {
                auto last_entity = m_Entities[last];

                std::swap(m_Entities[index], m_Entities[last]);

                m_Index[last_entity] = index;

                for (auto &storage : m_Storage | std::views::values)
                {
                    const auto a = storage.get(index);
                    const auto b = storage.get(last);

                    std::swap_ranges(a, a + storage.stride(), b);
                }
            }

            m_Entities.pop_back();
            m_Index.erase(entity);

            for (auto &storage : m_Storage | std::views::values)
                storage.release();
        }

    private:
        component_mask_t m_Mask;

        std::vector<EntityID> m_Entities;
        std::unordered_map<EntityID, size_t> m_Index;

        std::unordered_map<ComponentID, component_storage_t> m_Storage;
    };

    template<typename... C>
    using QueryResult = std::tuple<std::vector<C>...>;

    class ECS
    {
    public:
        template<typename... C>
        archetype_t &GetArchetype()
        {
            constexpr auto mask = get_component_mask<C...>();

            if (const auto it = m_Archetypes.find(mask); it != m_Archetypes.end())
                return it->second;

            std::unordered_map<ComponentID, size_t> strides;
            ([&]
            {
                using traits = component_traits_t<C>;
                strides[traits::id] = sizeof(C);
            }(), ...);

            return m_Archetypes[mask] = archetype_t(mask, strides);
        }

        archetype_t &GetArchetype(const component_mask_t mask, const std::unordered_map<ComponentID, size_t> &strides)
        {
            if (const auto it = m_Archetypes.find(mask); it != m_Archetypes.end())
                return it->second;

            return m_Archetypes[mask] = archetype_t(mask, strides);
        }

        template<typename... C>
        QueryResult<C...> Query()
        {
            constexpr auto mask = get_component_mask<C...>();

            QueryResult<C...> result;

            for (auto &archetype : m_Archetypes | std::views::values)
            {
                if (!archetype.matches(mask))
                    continue;

                ([&]
                {
                    using traits = component_traits_t<C>;

                    auto &dst = std::get<std::vector<C>>(result);
                    auto &src = archetype.get_column(traits::id);

                    for (auto &component : src.template cast<C>())
                        dst.push_back(component);
                }(), ...);
            }

            return result;
        }

        template<typename... C>
        EntityID Create(C... components)
        {
            auto &archetype = GetArchetype<C...>();
            auto entity = m_Entities.size();

            m_Entities[entity] = &archetype;

            archetype.allocate(entity);
            ([&]
            {
                using traits = component_traits_t<C>;
                archetype.set(entity, traits::id, reinterpret_cast<uint8_t *>(&components), sizeof(C));
            }(), ...);

            return entity;
        }

        template<typename C>
        void Add(EntityID entity, C component)
        {
            using traits = component_traits_t<C>;

            auto &src = *m_Entities[entity];

            auto strides = src.strides();
            strides[traits::id] = sizeof(C);

            auto &dst = GetArchetype(src.mask() | 1ull << traits::id, strides);

            dst.allocate(entity);
            dst.set(entity, traits::id, reinterpret_cast<uint8_t *>(&component), sizeof(C));

            auto mask = src.mask();
            for (ComponentID sid{}; mask; ++sid, mask >>= 1)
                if (mask & 1)
                {
                    uint8_t *data;
                    size_t size;
                    src.get(entity, sid, data, size);
                    dst.set(entity, sid, data, size);
                }

            src.release(entity);

            m_Entities[entity] = &dst;
        }

        template<typename C>
        void Remove(EntityID entity)
        {
            using traits = component_traits_t<C>;

            auto &src = *m_Entities[entity];

            auto strides = src.strides();
            strides.erase(traits::id);

            auto &dst = GetArchetype(src.mask() & ~(1ull << traits::id));

            dst.allocate(entity);

            auto mask = dst.mask();
            for (ComponentID sid{}; mask; ++sid, mask >>= 1)
                if (mask & 1)
                {
                    uint8_t *data;
                    size_t size;
                    src.get(entity, sid, data, size);
                    dst.set(entity, sid, data, size);
                }

            src.release(entity);
        }

    private:
        std::unordered_map<component_mask_t, archetype_t> m_Archetypes;
        std::unordered_map<EntityID, archetype_t *> m_Entities;
    };
}
