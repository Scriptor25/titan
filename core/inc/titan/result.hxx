#pragma once

#include <toolkit/result.hxx>

namespace titan
{
    template<typename T, typename... F>
    auto sequence(T *self, F &&... f)
    {
        return (toolkit::result() & ... & [self, fn = std::forward<F>(f)]() -> toolkit::result<>
        {
            return (self->*fn)();
        });
    }
}
