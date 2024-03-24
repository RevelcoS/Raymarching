#pragma once

#include <LiteMath.h>

#include <vector>
#include "object.h"
#include "body.h"

using namespace LiteMath;

namespace scene {
    extern Object::Container objects;
    extern std::vector<int> bodyIDs;
    extern int boundsID;
    extern int lightID;
    bool inside(float3 position);
    float lighting(float3 position, float3 normal);
    Body::Surface SDF(float3 position);
    float3 grad(float3 position);
    void load(const char *path);
};
