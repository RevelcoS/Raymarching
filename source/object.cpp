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
