#pragma once

#include <LiteMath.h>

using namespace LiteMath;

namespace constants {
    static const char *title    = "Raymarching";    // Project title
    const uint width            = 768;              // Window width
    const uint height           = 768;              // Window height
    const int iterations        = 1000;             // Raymarcing iterations
    const float gamma           = 1.0f;             // gamma correction
    const float saturation      = 0.05f;            // lighting saturation
    const int capacity          = 1 << 10;          // Container::Array capacity
    const int logsize           = 1 << 10;          // Shader log buffer size
    const float precision       = 1e-3f;            // Object hit precision

    namespace SSAA {
        const int kernel        = 3;                // Kernel size
    }

    namespace stb {
        const int quality       = 100;              // Image quality
        const int channels      = 4;                // Color channels
    }

    // Constants must be in sync with glsl fragment shader defines
    namespace gpu {
        constexpr size_t bodyElements   = 4;        // Length of Body struct float4 array
        constexpr size_t bodyTypes      = 20;       // Limit number of Body Types
        constexpr size_t bodyMax        = 1 << 10;  // Amount of Bodies array contains
        constexpr size_t listEntries    = 1 << 6;   // Number of Lists
        constexpr size_t listMax        = 1 << 10;  // Amount of Nodes List contains
        constexpr size_t stackMax       = 1 << 6;   // Amount of Items in Stack
    }
}
