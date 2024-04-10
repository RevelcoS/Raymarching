#pragma once

#include <LiteMath.h>
#include "constants.h"

using namespace LiteMath;

namespace Object {
    enum class Type {
        BODY,
        LIGHT,
        CAMERA
    };

    // Base class for all objects
    struct Base {
        Type type;
        Base(Type type);
    };

    struct Light : Base {
        float3 position;
        float3 color;
        Light(float3 position, float3 color = float3(1.0f));
    };

    struct Camera : Base {
        float3 position;
        float3 direction;
        float3 up;
        float FOV;

        float4x4 transform;
        float focal;
        Camera(float3 position = float3(0.0f),
            float3 direction = float3(0.0f, 0.0f, -1.0f),
            float3 up = float3(0.0f, 1.0f, 0.0f),
            float FOV = 90);
        void update(void);
        float3 view(float3 vector, bool offset = true);
    };
}
