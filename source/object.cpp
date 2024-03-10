#include <LiteMath.h>
#include "object.h"

using namespace LiteMath;

namespace Object {
    Base::Base(float3 position, float3 color, Type type) :
        position(position), color(color), type(type) {}

    Sphere::Sphere(float3 position, float radius, float3 color) :
        Base(position, color, Type::SPHERE), radius(radius) {}
    
    Box::Box(float3 position, float3 size, float3 color, bool inverse) :
        Base(position, color, Type::BOX), size(size), inverse(inverse) {}
    
    Light::Light(float3 position, float3 color) :
        Base(position, color, Type::LIGHT) {}
}
