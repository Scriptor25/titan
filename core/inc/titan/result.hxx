#pragma once

#include <expected>
#include <format>
#include <functional>

#define WRAP(FN) ([&]{ return (FN)(); })

namespace core
{
    template<typename T = void>
    class result;

    template<typename>
    struct is_result_t : std::false_type
    {
    };

    template<typename T>
    struct is_result_t<result<T>> : std::true_type
    {
    };

    template<typename T>
    concept result_type = is_result_t<T>::value;

    template<>
    class result<void>
    {
        template<typename>
        friend class result;

    public:
        using value_type = std::expected<void, std::string>;

        result() = default;

        result(value_type &&value)
            : value(std::move(value))
        {
        }

        result(const value_type &value)
            : value(value)
        {
        }

        result(result &&other) noexcept
            : value(std::move(other.value))
        {
        }

        result(const result &other) = default;

        template<result_type R>
        result(R &&other) noexcept
            : value(std::unexpected(std::forward<R>(other).value.error()))
        {
        }

        template<result_type R>
        result(const R &other)
            : value(std::unexpected(other.value.error()))
        {
        }

        result &operator=(result &&other) noexcept
        {
            std::swap(value, other.value);
            return *this;
        }

        result &operator=(const result &other) = default;

        template<result_type R>
        result &operator=(R &&other) noexcept
        {
            value = std::unexpected(std::forward<R>(other).value.error());
            return *this;
        }

        template<result_type R>
        result &operator=(const R &other)
        {
            value = std::unexpected(other.value.error());
            return *this;
        }

        explicit operator bool() const
        {
            return !value;
        }

        std::string &error()
        {
            return value.error();
        }

        [[nodiscard]] const std::string &error() const
        {
            return value.error();
        }

        template<typename F>
        constexpr auto and_then(F &&f)
        {
            using R = std::invoke_result_t<F>;

            return R
            {
                value.and_then(
                    [&f]
                    {
                        return std::invoke(std::forward<F>(f)).value;
                    })
            };
        }

        template<typename F>
        constexpr auto or_else(F &&f)
        {
            using R = std::invoke_result_t<F>;

            return R
            {
                value.or_else(
                    [&f]
                    {
                        return std::invoke(std::forward<F>(f)).value;
                    })
            };
        }

        value_type value;
    };

    template<typename T>
    class result
    {
        template<typename>
        friend class result;

    public:
        using value_type = std::expected<T, std::string>;

        result() = default;

        result(T &&value)
            : value(std::forward<T>(value))
        {
        }

        result(const T &value)
            : value(value)
        {
        }

        result(value_type &&value)
            : value(std::move(value))
        {
        }

        result(const value_type &value)
            : value(value)
        {
        }

        result(result &&other) noexcept
            : value(std::move(other.value))
        {
        }

        result(const result &other) = default;

        template<result_type R>
        result(R &&other) noexcept
            : value(std::unexpected(std::forward<R>(other).value.error()))
        {
        }

        template<result_type R>
        result(const R &other)
            : value(std::unexpected(other.value.error()))
        {
        }

        result &operator=(result &&other) noexcept
        {
            std::swap(value, other.value);
            return *this;
        }

        result &operator=(const result &other) = default;

        template<result_type R>
        result &operator=(R &&other) noexcept
        {
            value = std::unexpected(std::forward<R>(other).value.error());
            return *this;
        }

        template<result_type R>
        result &operator=(const R &other)
        {
            value = std::unexpected(other.value.error());
            return *this;
        }

        explicit operator bool() const
        {
            return !value;
        }

        bool operator!() const
        {
            return !!value;
        }

        T &operator*()
        {
            return *value;
        }

        const T &operator*() const
        {
            return *value;
        }

        std::string &error()
        {
            return value.error();
        }

        [[nodiscard]] const std::string &error() const
        {
            return value.error();
        }

        template<typename F>
        constexpr auto and_then(F &&f)
        {
            using R = std::invoke_result_t<F, T>;
            static_assert(result_type<R>);

            return R
            {
                std::move(value).and_then(
                    [&f]<typename V>(V &&v)
                    {
                        return std::invoke(std::forward<F>(f), std::forward<V>(v)).value;
                    })
            };
        }

        template<typename F>
        constexpr auto or_else(F &&f)
        {
            using R = std::invoke_result_t<F, T>;
            static_assert(result_type<R>);

            return R
            {
                value.or_else(
                    [&f]<typename V>(V &&v)
                    {
                        return std::invoke(std::forward<F>(f), std::forward<V>(v)).value;
                    })
            };
        }

    private:
        value_type value;
    };

    inline result<> ok()
    {
        return {};
    }

    template<typename... A>
    result<> error(std::format_string<A...> &&fmt, A &&... args)
    {
        return { std::unexpected(std::format(std::move(fmt), std::forward<A>(args)...)) };
    }

    template<typename T, typename... A>
    result<T> error(std::format_string<A...> &&fmt, A &&... args)
    {
        return { std::unexpected(std::format(std::move(fmt), std::forward<A>(args)...)) };
    }

    template<typename T>
    auto operator>>(result<T> &&r, T &t)
    {
        auto f = [&t](T &&v) -> result<>
        {
            t = std::move(v);
            return {};
        };

        return std::move(r).and_then(f);
    }

    template<typename T, typename F>
    auto operator&(result<T> &&r, F &&f)
    {
        return std::move(r).and_then(std::forward<F>(f));
    }

    template<typename T, typename F>
    auto operator|(result<T> &&r, F &&f)
    {
        return std::move(r).or_else(std::forward<F>(f));
    }
}
