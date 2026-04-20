#pragma once

#include <cstdint>
#include <memory>
#include <ranges>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace titan
{
    using EntityID = size_t;
    using ComponentID = size_t;

    namespace detail
    {
        struct ComponentInfo
        {
            size_t size;
            size_t alignment;

            void (*create)(void *dst);
            void (*destroy)(void *ptr);
            void (*move)(void *dst, void *src);
            void (*copy)(void *dst, const void *src);
            void (*swap)(void *a, void *b);
        };

        template<typename T>
        ComponentInfo GetComponentInfo()
        {
            return {
                sizeof(T),
                alignof(T),

                [](void *dst)
                {
                    new(dst) T();
                },
                [](void *ptr)
                {
                    static_cast<T *>(ptr)->~T();
                },
                [](void *dst, void *src)
                {
                    new(dst) T(std::move(*static_cast<T *>(src)));
                    static_cast<T *>(src)->~T();
                },
                [](void *dst, const void *src)
                {
                    new(dst) T(*static_cast<const T *>(src));
                },
                [](void *a, void *b)
                {
                    std::swap(*static_cast<T *>(a), *static_cast<T *>(b));
                }
            };
        }

        class ComponentRegistry
        {
        public:
            const ComponentInfo &operator[](ComponentID id) const;

            template<typename T>
            const ComponentInfo &Register()
            {
                if (auto it = m_Components.find(T::id); it != m_Components.end())
                    return it->second;
                return m_Components[T::id] = GetComponentInfo<T>();
            }

        private:
            std::unordered_map<ComponentID, ComponentInfo> m_Components;
        };

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
            static constexpr auto stride = sizeof(T);

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

                value_type &buffer;
                size_t offset;
            };

        public:
            TypeView(value_type &buffer)
                : m_Buffer(buffer)
            {
            }

            Iterator begin() const
            {
                return { m_Buffer, 0ull };
            }

            Iterator end() const
            {
                return { m_Buffer, m_Buffer.size() };
            }

            [[nodiscard]] size_t size() const
            {
                return m_Buffer.size() / stride;
            }

            element_type &operator[](size_t index) const
            {
                return reinterpret_cast<element_type &>(m_Buffer[index * stride]);
            }

        private:
            value_type &m_Buffer;
        };

        class ComponentStorage
        {
        public:
            explicit ComponentStorage(const ComponentInfo *component = {});

            ComponentStorage(const ComponentStorage &) = delete;
            ComponentStorage &operator=(const ComponentStorage &) = delete;

            ComponentStorage(ComponentStorage &&other) noexcept;
            ComponentStorage &operator=(ComponentStorage &&other) noexcept;

            [[nodiscard]] const ComponentInfo *GetComponent() const;

            void *Allocate(size_t count = 1ull);

            template<typename T>
            void Allocate(T &&value)
            {
                *static_cast<T *>(Allocate()) = std::forward<T>(value);
            }

            void Release(size_t count = 1ull);

            void Get(size_t index, void * &data);
            void Get(size_t index, const void * &data) const;

            void Set(size_t index, void *data);
            void Set(size_t index, const void *data);

            template<typename T>
            T &At(size_t index)
            {
                return Cast<T>()[index];
            }

            template<typename T>
            const T &At(size_t index) const
            {
                return Cast<T>()[index];
            }

            void Erase(size_t index);

            template<typename T>
            TypeView<false, T> Cast()
            {
                if (sizeof(T) != m_Component->size)
                    throw std::runtime_error("type size != component size");

                return { m_Buffer };
            }

            template<typename T>
            TypeView<true, T> Cast() const
            {
                if (sizeof(T) != m_Component->size)
                    throw std::runtime_error("type size != component size");

                return { m_Buffer };
            }

        private:
            const ComponentInfo *m_Component;

            std::vector<uint8_t> m_Buffer;
        };

        class Archetype
        {
        public:
            Archetype();
            Archetype(ComponentMask mask, const std::unordered_map<ComponentID, const ComponentInfo *> &components);

            Archetype(const Archetype &) = delete;
            Archetype &operator=(const Archetype &) = delete;

            Archetype(Archetype &&other) noexcept;
            Archetype &operator=(Archetype &&other) noexcept;

            [[nodiscard]] ComponentMask GetMask() const;
            [[nodiscard]] size_t GetCount() const;

            std::unordered_map<ComponentID, const ComponentInfo *> GetComponents() const;

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

            void Get(EntityID entity, ComponentID id, void * &data);
            void Get(EntityID entity, ComponentID id, const void * &data) const;

            void Set(EntityID entity, ComponentID id, void *data);
            void Set(EntityID entity, ComponentID id, const void *data);

            template<typename C>
            C &At(EntityID entity, ComponentID id)
            {
                return m_Storage.at(id).At<C>(m_Index.at(entity));
            }

            template<typename C>
            const C &At(EntityID entity, ComponentID id) const
            {
                return m_Storage.at(id).At<C>(m_Index.at(entity));
            }

            void Allocate(EntityID entity);

            template<typename... C>
            void Allocate(EntityID entity, C &&... components)
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
    }

    class EntitySystem
    {
    public:
        detail::Archetype &GetArchetype(
            detail::ComponentMask mask,
            const std::unordered_map<ComponentID, const detail::ComponentInfo *> &components);

        template<typename... C>
        detail::Archetype &GetArchetype()
        {
            const auto mask = detail::GetComponentMask<C...>();

            if (const auto it = m_Archetypes.find(mask); it != m_Archetypes.end())
                return it->second;

            return m_Archetypes[mask] = { mask, { { C::id, &m_Registry.Register<C>() }... } };
        }

        template<typename... C>
        detail::QueryResult<C...> Query()
        {
            const auto mask = detail::GetComponentMask<C...>();

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

        EntityID Create(std::unordered_map<ComponentID, void *> component_data);

        template<typename... C>
        EntityID Create(C &&... components)
        {
            auto &archetype = GetArchetype<C...>();
            auto entity = m_Entities.size();

            m_Entities[entity] = &archetype;

            archetype.Allocate(entity, std::forward<C>(components)...);

            return entity;
        }

        void Destroy(EntityID entity);

        void Add(EntityID entity, ComponentID id, void *data);

        template<typename C>
        void Add(EntityID entity, C &&component)
        {
            auto &src = *m_Entities.at(entity);

            auto components = src.GetComponents();
            components[C::id] = &m_Registry.Register<C>();

            auto &dst = GetArchetype(src.GetMask() | detail::GetComponentMask<C>(), components);

            dst.Allocate(entity, std::forward<C>(component));

            auto mask = src.GetMask();
            for (size_t index{}; mask; ++index, mask >>= 1)
                if (mask & 1)
                {
                    const auto id = detail::GetComponentID(index);

                    void *data;
                    src.Get(entity, id, data);
                    dst.Set(entity, id, data);
                }

            src.Release(entity);

            m_Entities[entity] = &dst;
        }

        void Remove(EntityID entity, ComponentID id);

        template<typename C>
        void Remove(EntityID entity)
        {
            auto &src = *m_Entities.at(entity);

            auto components = src.GetComponents();
            components.erase(C::id);

            auto &dst = GetArchetype(src.GetMask() & ~detail::GetComponentMask<C>(), components);

            dst.Allocate(entity);

            auto mask = dst.GetMask();
            for (size_t index{}; mask; ++index, mask >>= 1)
                if (mask & 1)
                {
                    const auto id = detail::GetComponentID(index);

                    void *data;
                    src.Get(entity, id, data);
                    dst.Set(entity, id, data);
                }

            src.Release(entity);

            m_Entities[entity] = &dst;
        }

    private:
        detail::ComponentRegistry m_Registry;

        std::unordered_map<detail::ComponentMask, detail::Archetype> m_Archetypes;
        std::unordered_map<EntityID, detail::Archetype *> m_Entities;
    };
}
