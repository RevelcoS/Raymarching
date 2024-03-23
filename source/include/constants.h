#pragma once

#include <LiteMath.h>

using namespace LiteMath;

namespace constants {
    static const char *title    = "Raymarching";    // Project title
    const uint width            = 768;              // Window width
    const uint height           = 768;              // Window height
    const int iterations        = 1000;             // Raymarcing iterations
    const float gamma           = 1.35f;            // gamma correction
    const float saturation      = 0.05f;            // lighting saturation
    const int capacity          = 1 << 10;          // Container::Array capacity
    const int logsize           = 1 << 10;          // Shader log buffer size
    const float precision       = 1e-3f;            // Object hit precision
    namespace stb {
        const int quality       = 100;              // Image quality
        const int channels      = 4;                // Color channels
    }
}
