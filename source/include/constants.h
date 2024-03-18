#pragma once

#include <LiteMath.h>

using namespace LiteMath;

namespace constants {
    namespace iterations {
        const int raymarch        = 1000;   // raymarcing iterations
    }
    const float saturation  = 0.05f;    // lighting saturation
    const int capacity      = 1 << 10;  // Container::Array capacity
    const float precision   = 1e-3f;    // Object hit precision
}
