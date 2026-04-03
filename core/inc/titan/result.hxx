#pragma once

#include <expected>
#include <format>

#define TRY(EXP) if (auto res = (EXP); !res) return res;
#define TRY_CAST(EXP, TY) if (auto res = (EXP); !res) return error<TY>(std::move(res));

namespace core
{
    template<typename T = void>
    using result = std::expected<T, std::string>;

    inline result<> ok()
    {
        return {};
    }

    template<typename... A>
    result<> error(std::format_string<A...> &&fmt, A &&... args)
    {
        return std::unexpected(std::format(std::forward<std::format_string<A...>>(fmt), std::forward<A>(args)...));
    }

    template<typename T, typename... A>
    result<T> error(std::format_string<A...> &&fmt, A &&... args)
    {
        return std::unexpected(std::format(std::forward<std::format_string<A...>>(fmt), std::forward<A>(args)...));
    }

    template<typename T, typename O>
    result<T> error(result<O> other)
    {
        return std::unexpected(std::move(other).error());
    }

    template<typename F>
    struct if_error_t
    {
        F f;
    };

    template<typename F>
    if_error_t<F> if_error(F &&f)
    {
        return { std::forward<F>(f) };
    }
}

template<typename T>
auto operator>>(core::result<T> &&r, T &t)
{
    return std::move(r).and_then(
        [&t]<typename V>(V &&v)
        {
            t = std::forward<V>(v);
            return core::ok();
        });
}

template<typename T, typename F>
auto operator|(core::result<T> &&r, F &&f)
{
    return std::move(r).and_then(std::forward<F>(f));
}

template<typename T, typename F>
auto operator|(core::result<T> &&r, core::if_error_t<F> &&e)
{
    return std::move(r).or_else(std::forward<F>(e.f));
}
