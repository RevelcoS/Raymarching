#pragma once

#include <LiteMath.h>
#include <vector>
#include "object.h"

using namespace LiteMath;

namespace Body {
    enum class Type : uint {
        EMPTY       = 0,
        LIST        = 1,
        SPHERE      = 2,
        BOX         = 3,
        CROSS       = 4,
    };

    enum class Mode : uint {
        UNION           = 0,
        COMPLEMENT      = 1,
        INTERSECTION    = 2,
        DIFFERENCE      = 3,
    };

    struct Surface {
        float SD;
        float3 color;
    };

    struct Base : Object::Base {
        Type type;
        Base(Type type);
        virtual Surface SDF(float3 position);
    };

    struct List : Base {
        std::vector<Base*> bodies;
        Mode mode;
        List(Mode mode = Mode::UNION);
        void append(Base *body);
        Surface SDF(float3 position);
    };

    struct Sphere : Base {
        float3 position;
        float radius;
        float3 color;
        Sphere(float3 position,
               float radius,
               float3 color = float3(1.0f));
        Surface SDF(float3 position);
    };

    struct Box : Base {
        float3 position;
        float3 size;
        float3 color;
        Box(float3 position,
            float3 size,
            float3 color = float3(1.0f));
        Surface SDF(float3 position);
    };

    struct Cross: Base {
        float3 position;
        float3 size;
        float3 color;
        Cross(float3 position,
              float3 size,
              float3 color = float3(1.0f));
        Surface SDF(float3 position);
    };

    // Generators
    List* MengerSponge(
            float3 position,
            float size,
            int iterations = 3,
            float3 color = float3(1.0f));

    List* DeathStar(
            float3 position,
            float radius,
            float3 color = float3(1.0f));
}
