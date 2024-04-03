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
    Base::Base(Type type) : Object::Base(Object::Type::BODY), type(type) {}

    Surface Base::SDF(float3 position) {
        float distance = std::numeric_limits<float>::infinity();
        return { .SD = distance, .color = float3(0.0f) };
    }

    /// Sphere ///
    Sphere::Sphere(float3 position, float radius, float3 color) :
        Base(Type::SPHERE), position(position), radius(radius), color(color) {}

    Surface Sphere::SDF(float3 position) {
        float distance = length(this->position - position) - this->radius;
        return { .SD = distance, .color = this->color };
    }

    /// Box ///
    Box::Box(float3 position, float3 size, float3 color) :
        Base(Type::BOX), position(position), size(size), color(color) {}

    Surface Box::SDF(float3 position) {
        float3 distances = abs(position - this->position) - this->size / 2;
        float distance = max(max(distances.x, distances.y), distances.z);
        return { .SD = distance, .color = this->color };
    }

    /// Cross ///
    Cross::Cross(float3 position, float3 size, float3 color) :
        Base(Type::CROSS), position(position), size(size), color(color) {}

    Surface Cross::SDF(float3 position) {
        float3 distances = abs(position - this->position) - this->size / 2;
        float dmin = min(min(distances.x, distances.y), distances.z);
        float dmax = max(max(distances.x, distances.y), distances.z);
        float distance = distances.x + distances.y + distances.z - dmin - dmax;
        return { .SD = distance, .color = this->color };
    }

    /// List ///
    List::List(Mode mode) : Base(Type::LIST), mode(mode) {}

    void List::append(Base *body) {
        this->bodies.push_back(body);
    }

    Surface List::SDF(float3 position) {
        if (this->bodies.empty()) {
            float distance = std::numeric_limits<float>::infinity();
            return { .SD = distance, .color = float3(0.0f) };
        }

        Base *body = this->bodies[0];
        Surface surface = body->SDF(position);
        if (this->mode == Mode::COMPLEMENT)
            surface = -surface;

        for (size_t idx = 1; idx < this->bodies.size(); idx++) {
            body = this->bodies[idx];
            Surface current = body->SDF(position);

            switch (this->mode) {
                case Mode::UNION:
                {
                    surface = min(surface, current);
                    break;
                }

                case Mode::COMPLEMENT:
                {
                    surface = min(surface, -current);
                    break;
                }

                case Mode::INTERSECTION:
                {
                    surface = max(surface, current);
                    break;
                }

                case Mode::DIFFERENCE:
                {
                    surface = max(surface, -current);
                    break;
                }

                defaut: break;
            }
        }

        return surface;
    }

    /// Menger Sponge /// 
    static void generateMengerSponge(List* result, float3 position, float size, int iterations, float3 color) {
        float d = size / 3;
        Cross *cross = new Cross(position, float3(d), color);
        result->append(cross);

        if (iterations >= 2) {

            // Front
            generateMengerSponge(result, position + float3(d, -d, -d), d, iterations - 1, color);
            generateMengerSponge(result, position + float3(0, -d, -d), d, iterations - 1, color);
            generateMengerSponge(result, position + float3(-d, -d, -d), d, iterations - 1, color);
            generateMengerSponge(result, position + float3(d, d, -d), d, iterations - 1, color);
            generateMengerSponge(result, position + float3(0, d, -d), d, iterations - 1, color);
            generateMengerSponge(result, position + float3(-d, d, -d), d, iterations - 1, color);
            generateMengerSponge(result, position + float3(-d, 0, -d), d, iterations - 1, color);
            generateMengerSponge(result, position + float3(d, 0, -d), d, iterations - 1, color);

            // Back
            generateMengerSponge(result, position + float3(d, -d, d), d, iterations - 1, color);
            generateMengerSponge(result, position + float3(0, -d, d), d, iterations - 1, color);
            generateMengerSponge(result, position + float3(-d, -d, d), d, iterations - 1, color);
            generateMengerSponge(result, position + float3(d, d, d), d, iterations - 1, color);
            generateMengerSponge(result, position + float3(0, d, d), d, iterations - 1, color);
            generateMengerSponge(result, position + float3(-d, d, d), d, iterations - 1, color);
            generateMengerSponge(result, position + float3(-d, 0, d), d, iterations - 1, color);
            generateMengerSponge(result, position + float3(d, 0, d), d, iterations - 1, color);

            // Middle
            generateMengerSponge(result, position + float3(-d, -d, 0), d, iterations - 1, color);
            generateMengerSponge(result, position + float3(d, -d, 0), d, iterations - 1, color);
            generateMengerSponge(result, position + float3(-d, d, 0), d, iterations - 1, color);
            generateMengerSponge(result, position + float3(d, d, 0), d, iterations - 1, color);
        }
    }

    List* MengerSponge(float3 position, float size, int iterations, float3 color) {
        List *result = new List(Mode::DIFFERENCE);
        Box *box = new Box(position, float3(size), color);
        result->append(box);

        generateMengerSponge(result, position, size, iterations, color);
        return result;
    }

    /// Death Star ///
    List* DeathStar(float3 position, float radius, float3 color) {
        List *result = new List(Mode::DIFFERENCE);
        Sphere *sphere = new Sphere(position, radius, color);
        result->append(sphere);

        float3 diffposition = float3(position.x, position.y, position.z - 1.5f * radius);
        Sphere *diff = new Sphere(diffposition, radius, color);
        result->append(diff);

        return result;
    }
}
