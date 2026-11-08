#include <titan/heap.hxx>

void titan::Heap::Insert(const void *user, Destructor destroy, std::unordered_set<const void *> handles)
{
    auto &user_entry = m_Handles[user];

    user_entry.Destroy = std::move(destroy);
    user_entry.Handles.insert(handles.begin(), handles.end());

    for (const auto *handle : handles)
    {
        auto &handle_entry = m_Handles[handle];

        ++handle_entry.Count;
    }
}

void titan::Heap::Erase(const void *user)
{
    const auto &user_entry = m_Handles[user];

    for (const auto *handle : user_entry.Handles)
    {
        auto &handle_entry = m_Handles[handle];

        --handle_entry.Count;

        if (!handle_entry.Count && handle_entry.Destroy)
            handle_entry.Destroy();
    }

    m_Handles.erase(user);
}

void titan::Heap::Use(const void *user, const void *handle)
{
    auto &user_entry = m_Handles[user];

    user_entry.Handles.insert(handle);

    auto &handle_entry = m_Handles[handle];

    ++handle_entry.Count;
}

void titan::Heap::Drop(const void *user, const void *handle)
{
    auto &user_entry = m_Handles[user];

    user_entry.Handles.erase(handle);

    auto &handle_entry = m_Handles[handle];

    --handle_entry.Count;

    if (!handle_entry.Count && handle_entry.Destroy)
        handle_entry.Destroy();
}

bool titan::Heap::Uses(const void *handle) const
{
    if (const auto it = m_Handles.find(handle); it != m_Handles.end())
    {
        auto &entry = it->second;
        return entry.Count;
    }

    return true;
}
