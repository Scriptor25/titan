#include <titan/heap.hxx>

void titan::Heap::Insert(const void *handle, void (*destroy)())
{
    m_Handles.insert(
        {
            handle,
            {
                .Count = {},
                .Destroy = destroy,
            },
        });
}

void titan::Heap::Erase(const void *handle)
{
    m_Handles.erase(handle);
}

void titan::Heap::Use(const void *handle)
{
    auto &[count, destroy] = m_Handles[handle];

    ++count;
}

void titan::Heap::Drop(const void *handle)
{
    auto &[count, destroy] = m_Handles[handle];

    --count;

    if (!count && destroy)
        destroy();
}

bool titan::Heap::Uses(const void *handle)
{
    auto &[count, destroy] = m_Handles[handle];

    return count;
}
