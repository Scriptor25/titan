#pragma once

#include <functional>
#include <span>
#include <unordered_map>
#include <unordered_set>

namespace titan
{
    class Heap
    {
        using Destructor = std::function<void()>;

        struct Entry
        {
            size_t Count;
            Destructor Destroy;

            std::unordered_set<const void *> Handles;
        };

    public:
        void Insert(const void *handle, Destructor destroy, std::unordered_set<const void *> handles);
        void Erase(const void *handle);

        template<typename... P>
        void Insert(const void *handle, const Destructor &destroy, const P *... handles)
        {
            Insert(handle, destroy, { static_cast<const void *>(handles)... });
        }

        void Use(const void *user, const void *handle);

        template<typename P>
        void Use(const void *user, std::span<P *const> handles)
        {
            auto &user_entry = m_Handles[user];

            for (auto handle : handles)
            {
                user_entry.Handles.insert(handle);

                auto &entry = m_Handles[handle];
                ++entry.Count;
            }
        }

        void Drop(const void *user, const void *handle);

        [[nodiscard]] bool Uses(const void *handle) const;

    private:
        std::unordered_map<const void *, Entry> m_Handles;
    };
}
