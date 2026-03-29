#pragma once

namespace core
{
    template<typename T, typename W>
    class wrapper_base
    {
    public:
        using value_type = T;

        wrapper_base() = default;

        explicit wrapper_base(T value)
            : value(value)
        {
        }

        virtual ~wrapper_base()
        {
            if constexpr (std::is_convertible_v<T, bool>)
            {
                if (value)
                {
                    wrapper_base::destroy();
                    value = {};
                }
            }
            else
            {
                wrapper_base::destroy();
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
