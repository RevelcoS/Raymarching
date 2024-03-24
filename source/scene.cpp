#include <LiteMath.h>

#include <limits>
#include <vector>

// scene::load
#include <string>
#include <fstream>
#include <sstream>

#include "constants.h"
#include "object.h"
#include "body.h"
#include "scene.h"

using namespace LiteMath;

// TODO: free objects when process finished
namespace scene {
    Body::Tree *tree;
    Object::Light *light;
    Body::Box *bounds;
}

// Check if the position is inside of scene boundaries
bool scene::inside(float3 position) {
    return all_of(abs(position - scene::bounds->position) < scene::bounds->size / 2);
}

// Calculate the lighting at the surface
float scene::lighting(float3 position, float3 normal) {
    return max(constants::saturation, dot(normal, normalize(scene::light->position - position)));
}

// Calculate SDF from scene tree
Body::Surface scene::SDF(float3 position) {
    return tree->SDF(position);
}

// Calculate gradient of scene SDF
float3 scene::grad(float3 p) {
    static const float h = 1e-3f;
    float3 dx = float3(h, 0.0f, 0.0f);
    float3 dy = float3(0.0f, h, 0.0f);
    float3 dz = float3(0.0f, 0.0f, h);

    Body::Surface dxl = scene::SDF(p + dx);
    Body::Surface dxr = scene::SDF(p - dx);
    float dfdx = dxl.SD - dxr.SD;

    Body::Surface dyl = scene::SDF(p + dy);
    Body::Surface dyr = scene::SDF(p - dy);
    float dfdy = dyl.SD - dyr.SD;

    Body::Surface dzl = scene::SDF(p + dz);
    Body::Surface dzr = scene::SDF(p - dz);
    float dfdz = dzl.SD - dzr.SD;

    return float3(dfdx, dfdy, dfdz) / (2 * h);
}

// Load scene objects from path
void scene::load(const char *path) {
    scene::tree = new Body::Tree();

    std::ifstream file(path);
    std::string line;

    float3 color = float3(1.0f);
    while (std::getline(file, line)) {
        std::istringstream input(line);
        std::string cmd;
        input >> cmd;

        Body::Base *obj;
        bool isBody = true;
        if (cmd == "Sphere") {
            float3 position;
            float radius;
            input >> position.x >> position.y >> position.z >> radius;
            obj = new Body::Sphere(position, radius, color);
        } else if (cmd == "Box") {
            float3 position, size;
            input >> position.x >> position.y >> position.z;
            input >> size.x >> size.y >> size.z;
            obj = new Body::Box(position, size, color);
        } else if (cmd == "Cross") {
            float3 position, size;
            input >> position.x >> position.y >> position.z;
            input >> size.x >> size.y >> size.z;
            obj = new Body::Cross(position, size, color);
        } else if (cmd == "DeathStar") {
            float3 position;
            float radius;
            input >> position.x >> position.y >> position.z >> radius;
            obj = Body::DeathStar(position, radius, color);
        } else if (cmd == "MengerSponge") {
            float3 position;
            float size;
            int iterations;
            input >> position.x >> position.y >> position.z >> size >> iterations;
            obj = Body::MengerSponge(position, size, iterations);
        } else isBody = false;

        if (isBody) {
            scene::tree->add(obj);
            continue;
        }

        if (cmd == "Bounds") {
            float size;
            input >> size;
            scene::bounds = new Body::Box(float3(0.0f), float3(size));
        } else if (cmd == "Light") {
            float3 position;
            input >> position.x >> position.y >> position.z;
            scene::light = new Object::Light(position);
        } else if (cmd == "Color") {;
            input >> color.x >> color.y >> color.z;
        }
    }
}
