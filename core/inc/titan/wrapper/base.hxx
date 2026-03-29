#pragma once

#include <titan/log.hxx>

namespace core
{
    template<typename T, typename W>
        requires std::is_convertible_v<T, bool> && std::is_convertible_v<T, const void *>
    class wrapper_base
    {
    public:
        using value_type = T;

        wrapper_base() = default;

        explicit wrapper_base(T value)
            : value(value)
        {
            info("create {} {}", typename_string<T>(), reinterpret_cast<const void *>(value));
        }

        virtual ~wrapper_base()
        {
            if (value)
            {
                info("destroy {} {}", typename_string<T>(), reinterpret_cast<const void *>(value));

                wrapper_base::destroy();
                value = {};
            }
        }

        wrapper_base(const wrapper_base &) = delete;
        wrapper_base &operator=(const wrapper_base &) = delete;

        wrapper_base(wrapper_base &&other) noexcept
        {
            std::swap(value, other.value);
            wrapper_base::swap(static_cast<W &&>(other));
        }

        wrapper_base &operator=(wrapper_base &&other) noexcept
        {
            std::swap(value, other.value);
            swap(static_cast<W &&>(other));
            return *this;
        }

        wrapper_base(W &&other) noexcept
        {
            std::swap(value, other.value);
            swap(other);
        }

        wrapper_base &operator=(W &&other) noexcept
        {
            std::swap(value, other.value);
            swap(other);
            return *this;
        }

        [[nodiscard]] T get() const
        {
            return value;
        }

        operator T &()
        {
            return value;
        }

        operator const T &() const
        {
            return value;
        }

        T &operator *()
        {
            return value;
        }

        const T &operator *() const
        {
            return value;
        }

        T *operator->()
        {
            return &value;
        }

        const T *operator->() const
        {
            return &value;
        }

        explicit operator bool() const
        {
            return !!value;
        }

        bool operator!() const
        {
            return !value;
        }

    protected:
        virtual void destroy()
        {
        }

        virtual void swap(W &&) noexcept
        {
        }

        T value{};
    };
}
