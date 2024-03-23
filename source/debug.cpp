#include <LiteMath.h>
#include <iostream>

#include "debug.h"

using namespace LiteMath;

void debug::print(float3 vector) {
    std::cout << vector.x << " " << vector.y << " " << vector.z << std::endl;
}
