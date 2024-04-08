#pragma once

#include <LiteMath.h>

#include <vector>
#include "object.h"
#include "body.h"

using namespace LiteMath;

namespace scene {
    extern Body::List *tree;
    extern std::vector<Object::Light*> lights;
    Body::Surface surface(float3 &position, float3 ray);
    float3 raymarch(float3 ray);
    bool shadow(Object::Light *light, float3 position, float3 normal);
    float lighting(float3 position, float3 normal);
    Body::Surface SDF(float3 position);
    float3 grad(float3 position);
    void load(const char *path);
};
