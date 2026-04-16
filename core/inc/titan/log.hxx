#pragma once

#include <format>
#include <iostream>

namespace titan
{
    template<typename... A>
    void info(std::format_string<A...> &&format, A &&... args)
    {
        std::cerr << "[INFO:APPLICATION] " << std::format(
            std::forward<std::format_string<A...>>(format),
            std::forward<A>(args)...) << std::endl;
    }
}
