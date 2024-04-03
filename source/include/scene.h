#pragma once

#include <LiteMath.h>

#include <vector>
#include "object.h"
#include "body.h"

using namespace LiteMath;

namespace scene {
    extern Body::List *tree;
    extern Object::Light *light;
    extern Body::Box *bounds;
    bool inside(float3 position);
    float lighting(float3 position, float3 normal);
    Body::Surface SDF(float3 position);
    float3 grad(float3 position);
    void load(const char *path);
};
