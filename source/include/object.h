#pragma once

#include <LiteMath.h>
#include "constants.h"

using namespace LiteMath;

namespace Object {
    enum class Type {
        BODY,
        LIGHT
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

    class Container {
        Base* _objects[constants::capacity];
        int _capacity;
        int _size;

    public:
        Container();
        int add(Base* object); // Returns ID of stored object
        Base* get(int ID);     // Get object by ID
    };
}
