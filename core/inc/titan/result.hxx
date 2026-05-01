#pragma once

#include <toolkit/result.hxx>

#define WRAP(FN) ([&]{ return (FN)(); })
