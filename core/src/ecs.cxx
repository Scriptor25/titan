#include <titan/ecs.hxx>

#include <unordered_map>

inline size_t component_index{};
inline std::unordered_map<titan::ComponentID, size_t> component_id_to_index;
inline std::unordered_map<size_t, titan::ComponentID> component_index_to_id;

size_t titan::GetComponentIndex(const ComponentID id)
{
    if (const auto it = component_id_to_index.find(id); it != component_id_to_index.end())
        return it->second;

    const auto index = component_index++;
    component_id_to_index[id] = index;
    component_index_to_id[index] = id;

    return index;
}

titan::ComponentID titan::GetComponentID(const size_t index)
{
    return component_index_to_id[index];
}

titan::ComponentStorage::ComponentStorage()
    : m_Stride()
{
}

titan::ComponentStorage::ComponentStorage(const size_t stride)
    : m_Stride(stride)
{
}

titan::ComponentStorage::ComponentStorage(ComponentStorage &&other) noexcept
    : m_Stride(other.m_Stride),
      m_Buffer(std::move(other.m_Buffer))
{
}

titan::ComponentStorage &titan::ComponentStorage::operator=(ComponentStorage &&other) noexcept
{
    std::swap(m_Stride, other.m_Stride);
    std::swap(m_Buffer, other.m_Buffer);

    return *this;
}

size_t titan::ComponentStorage::GetStride() const
{
    return m_Stride;
}

void titan::ComponentStorage::Allocate(const size_t count)
{
    m_Buffer.resize(m_Buffer.size() + count * m_Stride);
}

void titan::ComponentStorage::Release(const size_t count)
{
    m_Buffer.resize(m_Buffer.size() - count * m_Stride);
}

void titan::ComponentStorage::Get(const size_t index, void *&data, size_t &size)
{
    data = &m_Buffer[index * m_Stride];
    size = m_Stride;
}

void titan::ComponentStorage::Get(const size_t index, const void *&data, size_t &size) const
{
    data = &m_Buffer[index * m_Stride];
    size = m_Stride;
}

void titan::ComponentStorage::Set(const size_t index, void *data, const size_t size)
{
    if (size % m_Stride)
        throw std::runtime_error("data size % storage stride");
    if (index * m_Stride >= m_Buffer.size())
        throw std::runtime_error("data begin >= storage end");
    if (index * m_Stride + size > m_Buffer.size())
        throw std::runtime_error("data end > storage end");

    const auto p = static_cast<uint8_t *>(data);
    std::move(p, p + size, m_Buffer.begin() + index * m_Stride);
}

void titan::ComponentStorage::Erase(const size_t index)
{
    const auto begin = m_Buffer.begin() + index * m_Stride;
    const auto end = begin + m_Stride;

    m_Buffer.erase(begin, end);
}

titan::Archetype::Archetype()
    : m_Mask()
{
}

titan::Archetype::Archetype(const ComponentMask mask, const std::unordered_map<ComponentID, size_t> &strides)
    : m_Mask(mask)
{
    for (auto &[id, stride] : strides)
        m_Storage[id] = ComponentStorage(stride);
}

titan::Archetype::Archetype(Archetype &&other) noexcept
    : m_Mask(other.m_Mask),
      m_Entities(std::move(other.m_Entities)),
      m_Index(std::move(other.m_Index)),
      m_Storage(std::move(other.m_Storage))
{
}

titan::Archetype &titan::Archetype::operator=(Archetype &&other) noexcept
{
    std::swap(m_Mask, other.m_Mask);
    std::swap(m_Entities, other.m_Entities);
    std::swap(m_Index, other.m_Index);
    std::swap(m_Storage, other.m_Storage);

    return *this;
}

titan::ComponentMask titan::Archetype::GetMask() const
{
    return m_Mask;
}

size_t titan::Archetype::GetCount() const
{
    return m_Index.size();
}

std::unordered_map<titan::ComponentID, size_t> titan::Archetype::GetStrides() const
{
    std::unordered_map<ComponentID, size_t> strides;
    for (auto &[id, storage] : m_Storage)
        strides[id] = storage.GetStride();
    return strides;
}

bool titan::Archetype::Matches(const ComponentMask mask) const
{
    return (mask & m_Mask) == mask;
}

titan::ComponentStorage &titan::Archetype::GetColumn(const ComponentID id)
{
    return m_Storage.at(id);
}

const titan::ComponentStorage &titan::Archetype::GetColumn(const ComponentID id) const
{
    return m_Storage.at(id);
}

void titan::Archetype::Get(const EntityID entity, const ComponentID id, void *&data, size_t &size)
{
    m_Storage.at(id).Get(m_Index.at(entity), data, size);
}

void titan::Archetype::Get(const EntityID entity, const ComponentID id, const void *&data, size_t &size) const
{
    m_Storage.at(id).Get(m_Index.at(entity), data, size);
}

void titan::Archetype::Set(const EntityID entity, const ComponentID id, void *data, const size_t size)
{
    m_Storage.at(id).Set(m_Index.at(entity), data, size);
}

void titan::Archetype::Allocate(const EntityID entity)
{
    const auto index = m_Entities.size();

    m_Entities.push_back(entity);
    m_Index[entity] = index;

    for (auto &storage : m_Storage | std::views::values)
        storage.Allocate();
}

void titan::Archetype::Release(const EntityID entity)
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
            size_t a_size, b_size;

            storage.Get(index, a_data, a_size);
            storage.Get(last, b_data, b_size);

            const auto ap = static_cast<char *>(a_data);
            const auto bp = static_cast<char *>(b_data);

            std::swap_ranges(ap, ap + a_size, bp);
        }
    }

    m_Entities.pop_back();
    m_Index.erase(entity);

    for (auto &storage : m_Storage | std::views::values)
        storage.Release();
}

titan::Archetype &titan::ECS::GetArchetype(
    const ComponentMask mask,
    const std::unordered_map<ComponentID, size_t> &strides)
{
    if (const auto it = m_Archetypes.find(mask); it != m_Archetypes.end())
        return it->second;

    return m_Archetypes[mask] = { mask, strides };
}
