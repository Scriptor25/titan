#include <titan/heap.hxx>

void titan::Heap::Insert(const void *handle, Destructor destroy, std::unordered_set<const void *> handles)
{
    for (auto h : handles)
    {
        auto &entry = m_Handles[h];
        ++entry.Count;
    }

    m_Handles.insert(
        {
            handle,
            Entry
            {
                .Count = {},
                .Destroy = std::move(destroy),
                .Handles = std::move(handles),
            },
        });
}

void titan::Heap::Erase(const void *handle)
{
    const auto &user_entry = m_Handles[handle];

    for (auto h : user_entry.Handles)
    {
        auto &entry = m_Handles[h];
        --entry.Count;

        if (!entry.Count && entry.Destroy)
            entry.Destroy();
    }

    m_Handles.erase(handle);
}

void titan::Heap::Use(const void *user, const void *handle)
{
    auto &user_entry = m_Handles[user];
    user_entry.Handles.insert(handle);

    auto &entry = m_Handles[handle];
    ++entry.Count;
}

void titan::Heap::Drop(const void *user, const void *handle)
{
    auto &user_entry = m_Handles[user];
    user_entry.Handles.erase(handle);

    auto &entry = m_Handles[handle];
    --entry.Count;

    if (!entry.Count && entry.Destroy)
        entry.Destroy();
}

bool titan::Heap::Uses(const void *handle) const
{
    if (const auto it = m_Handles.find(handle); it != m_Handles.end())
    {
        auto &entry = it->second;
        return entry.Count;
    }

    return false;
}
