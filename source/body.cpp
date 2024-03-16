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

    float Base::complement(float distance) {
        if (this->mode == Mode::COMPLEMENT)
            return -distance;
        return distance;
    }

    /// Sphere ///
    Sphere::Sphere(float3 position, float radius, float3 color, Mode mode) :
        Base(Type::SPHERE, mode, color), position(position), radius(radius) {}

    float Sphere::SDF(float3 position) {
        float distance = length(this->position - position) - this->radius;
        return this->complement(distance);
    }

    /// Box ///
    Box::Box(float3 position, float3 size, float3 color, Mode mode) :
        Base(Type::BOX, mode, color), position(position), size(size) {}

    float Box::SDF(float3 position) {
        float3 distances = abs(position - this->position) - this->size / 2;
        float distance = max(max(distances.x, distances.y), distances.z);
        return this->complement(distance);
    }

    /// Cross ///
    Cross::Cross(float3 position, float3 size, float3 color, Mode mode) :
        Base(Type::CROSS, mode, color), position(position), size(size) {}

    float Cross::SDF(float3 position) {
        float3 distances = abs(position - this->position) - this->size / 2;
        float dmin = min(min(distances.x, distances.y), distances.z);
        float dmax = max(max(distances.x, distances.y), distances.z);
        float distance = distances.x + distances.y + distances.z - dmin - dmax;
        return this->complement(distance);
    }

    /// Empty ///
    Empty::Empty() : Base(Type::EMPTY, Mode::DEFAULT, float3(0.0f)) {}

    float Empty::SDF(float3 position) {
        return std::numeric_limits<float>::infinity();
    }

    /// Menger Sponge ///
    MengerSponge::MengerSponge(float3 position, float size, int iterations,
                               float3 color, Mode mode) :
        Base(Type::MENGER_SPONGE, mode, color),
        position(position), size(size), iterations(iterations) {

        Box *box = new Box(position, float3(size), color, mode);
        Compound *diff = this->generate(position, size, iterations);
        this->body = new Compound(box, diff, color, Mode::DIFFERENCE);
    }

    float MengerSponge::SDF(float3 position) {
        return this->body->SDF(position);
    }

    Compound* MengerSponge::generate(float3 position, float size, int iterations) {
        float d = size / 3;
        Cross *cross = new Cross(position, float3(d), this->color, this->mode);
        Empty *empty = new Empty();
        Compound *result = new Compound(cross, empty, this->color, this->mode);
        if (iterations >= 2) {
            Compound *recursive;

            // Front
            recursive = this->generate(position + float3(d, -d, -d), d, iterations - 1);
            result = new Compound(result, recursive, this->color, this->mode);

            recursive = this->generate(position + float3(0, -d, -d), d, iterations - 1);
            result = new Compound(result, recursive, this->color, this->mode);

            recursive = this->generate(position + float3(-d, -d, -d), d, iterations - 1);
            result = new Compound(result, recursive, this->color, this->mode);

            recursive = this->generate(position + float3(d, d, -d), d, iterations - 1);
            result = new Compound(result, recursive, this->color, this->mode);

            recursive = this->generate(position + float3(0, d, -d), d, iterations - 1);
            result = new Compound(result, recursive, this->color, this->mode);

            recursive = this->generate(position + float3(-d, d, -d), d, iterations - 1);
            result = new Compound(result, recursive, this->color, this->mode);

            recursive = this->generate(position + float3(-d, 0, -d), d, iterations - 1);
            result = new Compound(result, recursive, this->color, this->mode);

            recursive = this->generate(position + float3(d, 0, -d), d, iterations - 1);
            result = new Compound(result, recursive, this->color, this->mode);

            // Back
            recursive = this->generate(position + float3(d, -d, d), d, iterations - 1);
            result = new Compound(result, recursive, this->color, this->mode);

            recursive = this->generate(position + float3(0, -d, d), d, iterations - 1);
            result = new Compound(result, recursive, this->color, this->mode);

            recursive = this->generate(position + float3(-d, -d, d), d, iterations - 1);
            result = new Compound(result, recursive, this->color, this->mode);

            recursive = this->generate(position + float3(d, d, d), d, iterations - 1);
            result = new Compound(result, recursive, this->color, this->mode);

            recursive = this->generate(position + float3(0, d, d), d, iterations - 1);
            result = new Compound(result, recursive, this->color, this->mode);

            recursive = this->generate(position + float3(-d, d, d), d, iterations - 1);
            result = new Compound(result, recursive, this->color, this->mode);

            recursive = this->generate(position + float3(-d, 0, d), d, iterations - 1);
            result = new Compound(result, recursive, this->color, this->mode);

            recursive = this->generate(position + float3(d, 0, d), d, iterations - 1);
            result = new Compound(result, recursive, this->color, this->mode);

            // Middle
            recursive = this->generate(position + float3(-d, -d, 0), d, iterations - 1);
            result = new Compound(result, recursive, this->color, this->mode);

            recursive = this->generate(position + float3(d, -d, 0), d, iterations - 1);
            result = new Compound(result, recursive, this->color, this->mode);

            recursive = this->generate(position + float3(-d, d, 0), d, iterations - 1);
            result = new Compound(result, recursive, this->color, this->mode);

            recursive = this->generate(position + float3(d, d, 0), d, iterations - 1);
            result = new Compound(result, recursive, this->color, this->mode);
        }

        return result;
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
