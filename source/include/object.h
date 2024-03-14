#pragma once

#include <LiteMath.h>
#include "constants.h"

using namespace LiteMath;

namespace Object {
    enum class Type {
        SPHERE,
        BOX,
        LIGHT
    };

    // Base class for all objects
    struct Base {
        float3 position;
        float3 color;
        Type type;
        Base(float3 position, float3 color, Type type);
    };

    struct Sphere : Base {
        float radius;
        Sphere(float3 position, float radius,
               float3 color = float3(1.0f));
    };

    struct Box : Base {
        float3 size;
        bool inverse; // If true, SDF is positive inside, negative outside
        Box(float3 position, float3 size,
            float3 color = float3(1.0f),
            bool inverse = false);
    };

    struct Light : Base {
        Light(float3 position, float3 color = float3(1.0f));
    };

    class Container {
        Base* _objects[constants::capacity];
        int _capacity;
        int _size;

    public:
        Container();
        int add(Base* object);  // Returns ID of stored object
        Base* get(int ID);     // Get object by ID
    };
}