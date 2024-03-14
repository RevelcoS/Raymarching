#pragma once

#include <LiteMath.h>
#include "object.h"

using namespace LiteMath;

namespace Body {
    enum class Type {
        SPHERE,
        BOX,
        COMPOUND,
    };

    enum class Mode {
        DEFAULT,
        COMPLEMENT,
        UNION,
        INTERSECTION,
        DIFFERENCE,
    };

    struct Base : Object::Base {
        Type type;
        Mode mode;
        float3 color;
        Base(Type type,
             Mode mode = Mode::DEFAULT,
             float3 color = float3(1.0f));
        virtual float SDF(float3 position);
    };

    struct Sphere : Base {
        float3 position;
        float radius;
        Sphere(float3 position,
               float radius,
               float3 color = float3(1.0f),
               Mode mode = Mode::DEFAULT);
        float SDF(float3 position);
    };

    struct Box : Base {
        float3 position;
        float3 size;
        Box(float3 position,
            float3 size,
            float3 color = float3(1.0f),
            Mode mode = Mode::DEFAULT);
        float SDF(float3 position);
    };

    struct Compound : Base {
        Mode mode;
        Base *first;
        Base *second;
        Compound(Base *first,
                 Base *second,
                 float3 color = float3(1.0f),
                 Mode mode = Mode::UNION);
        float SDF(float3 position);
    };
}
