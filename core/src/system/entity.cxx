#include <titan/system/entity.hxx>

const titan::detail::ComponentInfo &titan::detail::ComponentRegistry::operator[](const ComponentID id) const
{
    return m_Components.at(id);
}

inline size_t component_index{};
inline std::unordered_map<titan::ComponentID, size_t> component_id_to_index;
inline std::unordered_map<size_t, titan::ComponentID> component_index_to_id;

size_t titan::detail::GetComponentIndex(const ComponentID id)
{
    if (const auto it = component_id_to_index.find(id); it != component_id_to_index.end())
        return it->second;

    const auto index = component_index++;
    component_id_to_index[id] = index;
    component_index_to_id[index] = id;

    return index;
}

titan::ComponentID titan::detail::GetComponentID(const size_t index)
{
    return component_index_to_id[index];
}

titan::detail::ComponentStorage::ComponentStorage(const ComponentInfo *component)
    : m_Component(component)
{
}

titan::detail::ComponentStorage::ComponentStorage(ComponentStorage &&other) noexcept
    : m_Component(other.m_Component),
      m_Buffer(std::move(other.m_Buffer))
{
}

titan::detail::ComponentStorage &titan::detail::ComponentStorage::operator=(ComponentStorage &&other) noexcept
{
    std::swap(m_Component, other.m_Component);
    std::swap(m_Buffer, other.m_Buffer);

    return *this;
}

const titan::detail::ComponentInfo *titan::detail::ComponentStorage::GetComponent() const
{
    return m_Component;
}

void *titan::detail::ComponentStorage::Allocate(const size_t count)
{
    const auto begin = m_Buffer.size();
    m_Buffer.resize(m_Buffer.size() + count * m_Component->size);

    for (auto offset = begin; offset < m_Buffer.size(); offset += m_Component->size)
        m_Component->create(m_Buffer.data() + offset);

    return m_Buffer.data() + begin;
}

void titan::detail::ComponentStorage::Release(const size_t count)
{
    const auto begin = m_Buffer.size() - count * m_Component->size;
    for (auto offset = begin; offset < m_Buffer.size(); offset += m_Component->size)
        m_Component->destroy(m_Buffer.data() + offset);

    m_Buffer.resize(begin);
}

void titan::detail::ComponentStorage::Get(const size_t index, void *&data)
{
    data = &m_Buffer[index * m_Component->size];
}

void titan::detail::ComponentStorage::Get(const size_t index, const void *&data) const
{
    data = &m_Buffer[index * m_Component->size];
}

void titan::detail::ComponentStorage::Set(const size_t index, void *data)
{
    const auto offset = index * m_Component->size;

    if (offset >= m_Buffer.size())
        throw std::runtime_error("offset >= buffer size");

    m_Component->move(m_Buffer.data() + offset, data);
}

void titan::detail::ComponentStorage::Set(const size_t index, const void *data)
{
    const auto offset = index * m_Component->size;

    if (offset >= m_Buffer.size())
        throw std::runtime_error("offset >= buffer size");

    m_Component->copy(m_Buffer.data() + offset, data);
}

void titan::detail::ComponentStorage::Erase(const size_t index)
{
    const auto offset = index * m_Component->size;
    m_Component->destroy(m_Buffer.data() + offset);

    m_Buffer.erase(m_Buffer.begin() + offset, m_Buffer.begin() + offset + m_Component->size);
}

titan::detail::Archetype::Archetype()
    : m_Mask()
{
}

titan::detail::Archetype::Archetype(
    const ComponentMask mask,
    const std::unordered_map<ComponentID, const ComponentInfo *> &components)
    : m_Mask(mask)
{
    for (auto &[id, component] : components)
        m_Storage[id] = ComponentStorage(component);
}

titan::detail::Archetype::Archetype(Archetype &&other) noexcept
    : m_Mask(other.m_Mask),
      m_Entities(std::move(other.m_Entities)),
      m_Index(std::move(other.m_Index)),
      m_Storage(std::move(other.m_Storage))
{
}

titan::detail::Archetype &titan::detail::Archetype::operator=(Archetype &&other) noexcept
{
    std::swap(m_Mask, other.m_Mask);
    std::swap(m_Entities, other.m_Entities);
    std::swap(m_Index, other.m_Index);
    std::swap(m_Storage, other.m_Storage);

    return *this;
}

titan::detail::ComponentMask titan::detail::Archetype::GetMask() const
{
    return m_Mask;
}

size_t titan::detail::Archetype::GetCount() const
{
    return m_Index.size();
}

std::unordered_map<
    titan::ComponentID,
    const titan::detail::ComponentInfo *> titan::detail::Archetype::GetComponents() const
{
    std::unordered_map<ComponentID, const ComponentInfo *> components;
    for (auto &[id, storage] : m_Storage)
        components[id] = storage.GetComponent();
    return components;
}

bool titan::detail::Archetype::Matches(const ComponentMask mask) const
{
    return (mask & m_Mask) == mask;
}

titan::detail::ComponentStorage &titan::detail::Archetype::GetColumn(const ComponentID id)
{
    return m_Storage.at(id);
}

const titan::detail::ComponentStorage &titan::detail::Archetype::GetColumn(const ComponentID id) const
{
    return m_Storage.at(id);
}

void titan::detail::Archetype::Get(const EntityID entity, const ComponentID id, void *&data)
{
    m_Storage.at(id).Get(m_Index.at(entity), data);
}

void titan::detail::Archetype::Get(const EntityID entity, const ComponentID id, const void *&data) const
{
    m_Storage.at(id).Get(m_Index.at(entity), data);
}

void titan::detail::Archetype::Set(const EntityID entity, const ComponentID id, void *data)
{
    m_Storage.at(id).Set(m_Index.at(entity), data);
}

void titan::detail::Archetype::Set(const EntityID entity, const ComponentID id, const void *data)
{
    m_Storage.at(id).Set(m_Index.at(entity), data);
}

void titan::detail::Archetype::Allocate(const EntityID entity)
{
    const auto index = m_Entities.size();

    m_Entities.push_back(entity);
    m_Index[entity] = index;

    for (auto &storage : m_Storage | std::views::values)
        storage.Allocate();
}

void titan::detail::Archetype::Release(const EntityID entity)
{
    const auto index = m_Index.at(entity);
    const auto last = m_Entities.size() - 1;

    if (index != last)
    {
        const auto last_entity = m_Entities[last];

        std::swap(m_Entities[index], m_Entities[last]);

        m_Index[last_entity] = index;

        for (auto &storage : m_Storage | std::views::values)
        {
            void *a_data, *b_data;

            storage.Get(index, a_data);
            storage.Get(last, b_data);

            storage.GetComponent()->swap(a_data, b_data);
        }
    }

    m_Entities.pop_back();
    m_Index.erase(entity);

    for (auto &storage : m_Storage | std::views::values)
        storage.Release();
}

titan::detail::Archetype &titan::EntitySystem::GetArchetype(
    const detail::ComponentMask mask,
    const std::unordered_map<ComponentID, const detail::ComponentInfo *> &components)
{
    if (const auto it = m_Archetypes.find(mask); it != m_Archetypes.end())
        return it->second;

    return m_Archetypes[mask] = { mask, components };
}

titan::EntityID titan::EntitySystem::Create(std::unordered_map<ComponentID, void *> component_data)
{
    detail::ComponentMask mask{};
    std::unordered_map<ComponentID, const detail::ComponentInfo *> components;

    for (auto &id : component_data | std::views::keys)
    {
        mask |= 1ull << detail::GetComponentIndex(id);
        components[id] = &m_Registry[id];
    }

    auto &archetype = GetArchetype(mask, components);
    const auto entity = m_Entities.size();

    m_Entities[entity] = &archetype;

    archetype.Allocate(entity);
    for (auto &[id, val] : component_data)
        archetype.Set(entity, id, val);

    return entity;
}

void titan::EntitySystem::Destroy(const EntityID entity)
{
    auto &archetype = *m_Entities.at(entity);

    archetype.Release(entity);
    m_Entities.erase(entity);
}

void titan::EntitySystem::Add(const EntityID entity, const ComponentID id, void *data)
{
    auto &src = *m_Entities.at(entity);

    auto components = src.GetComponents();
    components[id] = &m_Registry[id];

    auto &dst = GetArchetype(src.GetMask() | 1ull << detail::GetComponentIndex(id), components);

    dst.Allocate(entity);
    dst.Set(entity, id, data);

    auto mask = src.GetMask();
    for (size_t index{}; mask; ++index, mask >>= 1)
        if (mask & 1)
        {
            const auto c_id = detail::GetComponentID(index);

            void *c_data;
            src.Get(entity, c_id, c_data);
            dst.Set(entity, c_id, c_data);
        }

    src.Release(entity);

    m_Entities[entity] = &dst;
}

void titan::EntitySystem::Remove(const EntityID entity, const ComponentID id)
{
    auto &src = *m_Entities.at(entity);

    auto components = src.GetComponents();
    components.erase(id);

    auto &dst = GetArchetype(src.GetMask() & ~(1ull << detail::GetComponentIndex(id)), components);

    dst.Allocate(entity);

    auto mask = dst.GetMask();
    for (size_t index{}; mask; ++index, mask >>= 1)
        if (mask & 1)
        {
            const auto c_id = detail::GetComponentID(index);

            void *c_data;
            src.Get(entity, c_id, c_data);
            dst.Set(entity, c_id, c_data);
        }

    src.Release(entity);

    m_Entities[entity] = &dst;
}
