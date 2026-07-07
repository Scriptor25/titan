#pragma once

#include <toolkit/result.hxx>

#define WRAP(FN) ([&]{ return (FN)(); })
#define HANDLE(X) do { if (auto res = (X); !res) return res; } while (false)
