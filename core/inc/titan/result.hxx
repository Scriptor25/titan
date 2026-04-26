#pragma once

#include <toolkit/result.hxx>

#define WRAP(FN) ([&]{ return (FN)(); })

namespace titan
{
    inline toolkit::result<> ok()
    {
        return {};
    }
}
