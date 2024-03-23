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

namespace scene {
    Object::Container objects;
    std::vector<int> bodyIDs;
    int boundsID;
    int lightID;
}

// Check if the position is inside of scene boundaries
bool scene::inside(float3 position) {
    Body::Box *box = static_cast<Body::Box*>(scene::objects.get(scene::boundsID));
    return all_of(abs(position - box->position) < box->size / 2);
}

// Calculate the lighting at the surface
float scene::lighting(float3 position, float3 normal) {
    Object::Light *light = static_cast<Object::Light*>(scene::objects.get(scene::lightID));
    return max(constants::saturation, dot(normal, normalize(light->position - position)));
}

// Calculate SDF from scene objects and return <active> hit object
float scene::SDF(float3 position, Body::Base** active) {
    float result = std::numeric_limits<float>::infinity();

    for (auto bodyID : scene::bodyIDs) {
        Body::Base *obj = static_cast<Body::Base*>(scene::objects.get(bodyID));
        float distance = obj->SDF(position);

        // Choose the closest object
        if (distance < result) {
            result = distance;
            if (active) *active = obj;
        }
    }

    return result;
}

// Calculate gradient of the object SDF
float3 scene::grad(Body::Base* &obj, float3 p) {
    static const float h = 1e-3f;
    float3 dx = float3(h, 0.0f, 0.0f);
    float3 dy = float3(0.0f, h, 0.0f);
    float3 dz = float3(0.0f, 0.0f, h);
    float dfdx = obj->SDF(p + dx) - obj->SDF(p - dx);
    float dfdy = obj->SDF(p + dy) - obj->SDF(p - dy);
    float dfdz = obj->SDF(p + dz) - obj->SDF(p - dz);
    return float3(dfdx, dfdy, dfdz) / (2 * h);
}

// Load scene objects from path
void scene::load(const char *path) {
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
            int ID = scene::objects.add(obj);
            scene::bodyIDs.push_back(ID);
            continue;
        }

        if (cmd == "Bounds") {
            float size;
            input >> size;
            Body::Box *bounds = new Body::Box (float3(0.0f), float3(size));
            scene::boundsID = scene::objects.add(bounds);
        } else if (cmd == "Light") {
            float3 position;
            input >> position.x >> position.y >> position.z;
            Object::Light *light = new Object::Light (position);
            scene::lightID = scene::objects.add(light);
        } else if (cmd == "Color") {;
            input >> color.x >> color.y >> color.z;
        }
    }
}
