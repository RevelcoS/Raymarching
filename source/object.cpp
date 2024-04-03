#include <LiteMath.h>
#include <limits>

#include "constants.h"
#include "object.h"

using namespace LiteMath;

namespace Object {
    /// Base ///
    Base::Base(Type type) : type(type) {}

    /// Light ///
    Light::Light(float3 position, float3 color) :
        Base(Type::LIGHT), position(position), color(color) {}
}
