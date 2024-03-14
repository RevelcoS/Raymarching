#include <LiteMath.h>
#include "constants.h"
#include "object.h"

using namespace LiteMath;

namespace Object {
    /// Base ///
    Base::Base(float3 position, float3 color, Type type) :
        position(position), color(color), type(type) {}

    /// Sphere ///
    Sphere::Sphere(float3 position, float radius, float3 color) :
        Base(position, color, Type::SPHERE), radius(radius) {}

    /// Box ///
    Box::Box(float3 position, float3 size, float3 color, bool inverse) :
        Base(position, color, Type::BOX), size(size), inverse(inverse) {}

    /// Light ///
    Light::Light(float3 position, float3 color) :
        Base(position, color, Type::LIGHT) {}

    /// Container ///
    Container::Container() :
        _capacity(constants::capacity), _size(0) {}

    int Container::add(Base* object) {
        if (_size == _capacity)
            return -1;

        _objects[_size] = object;
        return _size++;
    }

    Base* Container::get(int ID) {
        return _objects[ID];
    }
}
