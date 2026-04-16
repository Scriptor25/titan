#pragma once

#include <expected>
#include <format>
#include <functional>

#define WRAP(FN) ([&]{ return (FN)(); })

namespace titan
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
    concept result_type = is_result_t<std::decay_t<T>>::value;

    template<>
    class result<void>
    {
        template<typename>
        friend class result;

    public:
        using error_type = std::string;
        using expected_type = std::expected<void, error_type>;

        result() = default;

        result(expected_type &&value)
            : value(std::move(value))
        {
        }

        result(const expected_type &value)
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
            static_assert(result_type<R>);

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
            using R = std::conditional_t<
                std::invocable<F>,
                std::invoke_result<F>,
                std::invoke_result<F, error_type &&>
            >::type;

            static_assert(result_type<R>);

            return R
            {
                value.or_else(
                    [&f]<typename E>(E &&error)
                    {
                        if constexpr (std::invocable<F>)
                            return std::invoke(std::forward<F>(f)).value;
                        else
                            return std::invoke(std::forward<F>(f), std::forward<E>(error)).value;
                    })
            };
        }

        expected_type value;
    };

    template<typename T>
    class result
    {
        template<typename>
        friend class result;

    public:
        using value_type = T;
        using error_type = std::string;
        using expected_type = std::expected<value_type, error_type>;

        result() = default;

        result(T &&value)
            : value(std::forward<T>(value))
        {
        }

        result(const T &value)
            : value(value)
        {
        }

        result(expected_type &&value)
            : value(std::move(value))
        {
        }

        result(const expected_type &value)
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
            using R = std::conditional_t<
                std::invocable<F>,
                std::invoke_result<F>,
                std::invoke_result<F, error_type &&>
            >::type;

            static_assert(result_type<R>);

            return R
            {
                value.or_else(
                    [&f]<typename E>(E &&error)
                    {
                        if constexpr (std::invocable<F>)
                            return std::invoke(std::forward<F>(f)).value;
                        else
                            return std::invoke(std::forward<F>(f), std::forward<E>(error)).value;
                    })
            };
        }

    private:
        expected_type value;
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

    template<result_type R>
    auto operator>>(R &&r, typename R::value_type &t)
    {
        auto f = [&t]<typename T>(T &&v) -> result<>
        {
            t = std::forward<T>(v);
            return {};
        };

        return std::forward<R>(r).and_then(f);
    }

    template<result_type R, typename F>
    auto operator&(R &&r, F &&f)
    {
        return std::forward<R>(r).and_then(std::forward<F>(f));
    }

    template<result_type R, typename F>
    auto operator|(R &&r, F &&f)
    {
        return std::forward<R>(r).or_else(std::forward<F>(f));
    }
}
