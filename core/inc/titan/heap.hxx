#pragma once

#include <unordered_map>

namespace titan
{
    class Heap
    {
        struct Entry
        {
            size_t Count;
            void (*Destroy)();
        };

    public:
        void Insert(const void *handle, void (*destroy)());
        void Erase(const void *handle);

        void Use(const void *handle);
        void Drop(const void *handle);

        bool Uses(const void *handle);

    private:
        std::unordered_map<const void *, Entry> m_Handles;
    };
}
