#include <LiteMath.h>
#include <limits>
#include <iostream>

#include "object.h"
#include "body.h"

using namespace LiteMath;

namespace Body {
    /// Base ///
    Base::Base(Type type, Mode mode, float3 color) :
        Object::Base(Object::Type::BODY), type(type), mode(mode), color(color) {}

    float Base::SDF(float3 position) {
        return std::numeric_limits<float>::infinity();
    }

    /// Sphere ///
    Sphere::Sphere(float3 position, float radius, float3 color, Mode mode) :
        Base(Type::SPHERE, mode, color), position(position), radius(radius) {}

    float Sphere::SDF(float3 position) {
        float distance = length(this->position - position) - this->radius;
        if (this->mode == Mode::COMPLEMENT)
            distance = -distance;
        return distance;
    }

    /// Box ///
    Box::Box(float3 position, float3 size, float3 color, Mode mode) :
        Base(Type::BOX, mode, color), position(position), size(size) {}

    // TODO: implement Box SDF
    float Box::SDF(float3 position) {
        //float3 distances = abs(position - box->position) - box->size / 2;
        return std::numeric_limits<float>::infinity();
    }

    /// Compound ///
    Compound::Compound(Base *first, Base *second, float3 color, Mode mode) :
        Base(Type::COMPOUND, mode, color), first(first), second(second), mode(mode) {}

    float Compound::SDF(float3 position) {
        float distance = std::numeric_limits<float>::infinity();
        float d1 = first->SDF(position);
        float d2 = second->SDF(position);
        switch (this->mode) {
            case Mode::DEFAULT:
            case Mode::UNION:
            {
                distance = min(d1, d2);
                break;
            }

            case Mode::COMPLEMENT:
            {
                distance = min(-d1, -d2);
            }

            case Mode::INTERSECTION:
            {
                distance = max(d1, d2);
                break;
            }

            case Mode::DIFFERENCE:
            {
                distance = max(d1, -d2);
                break;
            }

            defaut: break;
        }

        return distance;
    }
}
