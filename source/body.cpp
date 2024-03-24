#include <LiteMath.h>
#include <limits>
#include <iostream>

#include "object.h"
#include "body.h"

using namespace LiteMath;

namespace Body {
    /// Surface ///
    bool operator<(const Surface &lsurface, const Surface &rsurface) {
        return lsurface.SD < rsurface.SD;
    }

    Surface operator-(const Surface &surface) {
        return { .SD = -surface.SD, .color = surface.color };
    }

    /// Base ///
    Base::Base(Type type, Mode mode, float3 color) :
        Object::Base(Object::Type::BODY), type(type), mode(mode), color(color) {}

    Surface Base::SDF(float3 position) {
        float distance = std::numeric_limits<float>::infinity();
        return { .SD = distance, .color = this->color };
    }

    float Base::complement(float distance) {
        if (this->mode == Mode::COMPLEMENT)
            return -distance;
        return distance;
    }

    /// Sphere ///
    Sphere::Sphere(float3 position, float radius, float3 color, Mode mode) :
        Base(Type::SPHERE, mode, color), position(position), radius(radius) {}

    Surface Sphere::SDF(float3 position) {
        float distance = length(this->position - position) - this->radius;
        return { .SD = this->complement(distance), .color = this->color };
    }

    /// Box ///
    Box::Box(float3 position, float3 size, float3 color, Mode mode) :
        Base(Type::BOX, mode, color), position(position), size(size) {}

    Surface Box::SDF(float3 position) {
        float3 distances = abs(position - this->position) - this->size / 2;
        float distance = max(max(distances.x, distances.y), distances.z);
        return { .SD = this->complement(distance), .color = this->color };
    }

    /// Cross ///
    Cross::Cross(float3 position, float3 size, float3 color, Mode mode) :
        Base(Type::CROSS, mode, color), position(position), size(size) {}

    Surface Cross::SDF(float3 position) {
        float3 distances = abs(position - this->position) - this->size / 2;
        float dmin = min(min(distances.x, distances.y), distances.z);
        float dmax = max(max(distances.x, distances.y), distances.z);
        float distance = distances.x + distances.y + distances.z - dmin - dmax;
        return { .SD = this->complement(distance), .color = this->color };
    }

    /// Empty ///
    Empty::Empty() : Base(Type::EMPTY, Mode::DEFAULT, float3(0.0f)) {}

    /// Menger Sponge /// 
    Compound* generateMengerSponge(float3 position, float size, int iterations,
            float3 color, Mode mode) {
        float d = size / 3;
        Cross *cross = new Cross(position, float3(d), color, mode);
        Empty *empty = new Empty();
        Compound *result = new Compound(cross, empty, color, mode);
        if (iterations >= 2) {
            Compound *recursive;

            // Front
            recursive = generateMengerSponge(position + float3(d, -d, -d), d, iterations - 1, color, mode);
            result = new Compound(result, recursive, color, mode);

            recursive = generateMengerSponge(position + float3(0, -d, -d), d, iterations - 1, color, mode);
            result = new Compound(result, recursive, color, mode);

            recursive = generateMengerSponge(position + float3(-d, -d, -d), d, iterations - 1, color, mode);
            result = new Compound(result, recursive, color, mode);

            recursive = generateMengerSponge(position + float3(d, d, -d), d, iterations - 1, color, mode);
            result = new Compound(result, recursive, color, mode);

            recursive = generateMengerSponge(position + float3(0, d, -d), d, iterations - 1, color, mode);
            result = new Compound(result, recursive, color, mode);

            recursive = generateMengerSponge(position + float3(-d, d, -d), d, iterations - 1, color, mode);
            result = new Compound(result, recursive, color, mode);

            recursive = generateMengerSponge(position + float3(-d, 0, -d), d, iterations - 1, color, mode);
            result = new Compound(result, recursive, color, mode);

            recursive = generateMengerSponge(position + float3(d, 0, -d), d, iterations - 1, color, mode);
            result = new Compound(result, recursive, color, mode);

            // Back
            recursive = generateMengerSponge(position + float3(d, -d, d), d, iterations - 1, color, mode);
            result = new Compound(result, recursive, color, mode);

            recursive = generateMengerSponge(position + float3(0, -d, d), d, iterations - 1, color, mode);
            result = new Compound(result, recursive, color, mode);

            recursive = generateMengerSponge(position + float3(-d, -d, d), d, iterations - 1, color, mode);
            result = new Compound(result, recursive, color, mode);

            recursive = generateMengerSponge(position + float3(d, d, d), d, iterations - 1, color, mode);
            result = new Compound(result, recursive, color, mode);

            recursive = generateMengerSponge(position + float3(0, d, d), d, iterations - 1, color, mode);
            result = new Compound(result, recursive, color, mode);

            recursive = generateMengerSponge(position + float3(-d, d, d), d, iterations - 1, color, mode);
            result = new Compound(result, recursive, color, mode);

            recursive = generateMengerSponge(position + float3(-d, 0, d), d, iterations - 1, color, mode);
            result = new Compound(result, recursive, color, mode);

            recursive = generateMengerSponge(position + float3(d, 0, d), d, iterations - 1, color, mode);
            result = new Compound(result, recursive, color, mode);

            // Middle
            recursive = generateMengerSponge(position + float3(-d, -d, 0), d, iterations - 1, color, mode);
            result = new Compound(result, recursive, color, mode);

            recursive = generateMengerSponge(position + float3(d, -d, 0), d, iterations - 1, color, mode);
            result = new Compound(result, recursive, color, mode);

            recursive = generateMengerSponge(position + float3(-d, d, 0), d, iterations - 1, color, mode);
            result = new Compound(result, recursive, color, mode);

            recursive = generateMengerSponge(position + float3(d, d, 0), d, iterations - 1, color, mode);
            result = new Compound(result, recursive, color, mode);
        }

        return result;
    }

    Compound* MengerSponge(float3 position, float size, int iterations,
            float3 color, Mode mode) {
        Box *box = new Box(position, float3(size), color, mode);
        Compound *diff = generateMengerSponge(position, size, iterations, color, mode);
        return new Compound(box, diff, color, Mode::DIFFERENCE);
    }

    /// Death Star ///
    Compound* DeathStar(float3 position, float radius, float3 color, Mode mode) {
        Sphere *sphere = new Sphere(position, radius, color, mode);
        float3 diffposition = float3(position.x, position.y, position.z - 1.5f * radius);
        Sphere *diff = new Sphere(diffposition, radius, color, mode);
        return new Compound(sphere, diff, color, Mode::DIFFERENCE);
    }

    /// Tree ///
    Tree::Tree() {
        Empty *left = new Empty();
        Empty *right = new Empty();
        this->root = new Compound(left, right);
    }

    void Tree::add(Base *body) {
        this->root = new Compound(this->root, body);
    }

    Surface Tree::SDF(float3 position) {
        return this->root->SDF(position);
    }

    /// Compound ///
    Compound::Compound(Base *first, Base *second, float3 color, Mode mode) :
        Base(Type::COMPOUND, mode, color), first(first), second(second), mode(mode) {}

    Surface Compound::SDF(float3 position) {
        float distance = std::numeric_limits<float>::infinity();
        Surface surface { .SD = distance, .color = this->color };
        Surface s1 = first->SDF(position);
        Surface s2 = second->SDF(position);
        switch (this->mode) {
            case Mode::DEFAULT:
            case Mode::UNION:
            {
                surface = min(s1, s2);
                break;
            }

            case Mode::COMPLEMENT:
            {
                surface = min(-s1, -s2);
                break;
            }

            case Mode::INTERSECTION:
            {
                surface = max(s1, s2);
                break;
            }

            case Mode::DIFFERENCE:
            {
                surface = max(s1, -s2);
                break;
            }

            defaut: break;
        }

        return surface;
    }
}
