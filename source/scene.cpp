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
    Body::List *tree;
    std::vector<Object::Light*> lights;
    Object::Camera *camera;
}

// Calculate the color produced by ray
float3 scene::raymarch(float3 position, float3 ray) {
    Body::Surface surface = scene::surface(position, ray);
    float3 normal = normalize(scene::grad(position));
    float light = scene::lighting(position, normal);
    float3 color = light * surface.color;
    return color;
}

Body::Surface scene::surface(float3 &position, float3 ray) {
    Body::Surface surface {};
    for (int _ = 0; _ < constants::iterations; _++) {
        surface = scene::SDF(position);
        position += surface.SD * ray;
        if (surface.SD < constants::precision::surface) break;
    }
    return surface;
}

// Calculate shadow ray
bool scene::shadow(Object::Light *light, float3 position, float3 normal) {
    float3 ray = normalize(light->position - position);
    position += normal * (constants::precision::surface + constants::precision::offset);
    scene::surface(position, ray);
    return dot(light->position - position, ray) > 0;
}

// Calculate the lighting at the surface
float scene::lighting(float3 position, float3 normal) {
    float lighting = 0.0f;
    for (uint idx = 0; idx < lights.size(); idx++) {
        Object::Light *light = lights[idx];
        if (!scene::shadow(light, position, normal))
            lighting += dot(normal, normalize(light->position - position));
    }
    lighting = clamp(lighting, constants::saturation, 1.0f);
    return lighting;
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
    scene::tree = new Body::List();
    scene::camera = new Object::Camera();

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
        }
        else if (cmd == "Box") {
            float3 position, size;
            input >> position.x >> position.y >> position.z;
            input >> size.x >> size.y >> size.z;
            obj = new Body::Box(position, size, color);
        }
        else if (cmd == "Bounds") {
            float size;
            input >> size;
            Body::Box *box = new Body::Box(float3(0.0f), float3(size), float3(0.0f));
            Body::List *list = new Body::List(Body::Mode::COMPLEMENT);
            list->append(box);
            obj = list;
        }
        else if (cmd == "Cross") {
            float3 position, size;
            input >> position.x >> position.y >> position.z;
            input >> size.x >> size.y >> size.z;
            obj = new Body::Cross(position, size, color);
        }
        else if (cmd == "DeathStar") {
            float3 position;
            float radius;
            input >> position.x >> position.y >> position.z >> radius;
            obj = Body::DeathStar(position, radius, color);
        }
        else if (cmd == "MengerSponge") {
            float3 position;
            float size;
            int iterations;
            input >> position.x >> position.y >> position.z >> size >> iterations;
            obj = Body::MengerSponge(position, size, iterations, color);
        }
        else isBody = false;

        if (isBody) {
            scene::tree->append(obj);
            continue;
        }

        if (cmd == "Light") {
            float3 position;
            input >> position.x >> position.y >> position.z;
            Object::Light *light = new Object::Light(position);
            lights.push_back(light);
        }
        else if (cmd == "Camera") {
            std::string cameraCmd;
            input >> cameraCmd;

            float3 vector;
            float scalar;
            if (cameraCmd == "Position") {
                input >> vector.x >> vector.y >> vector.z;
                scene::camera->position = vector;
            }
            else if (cameraCmd == "Direction") {
                input >> vector.x >> vector.y >> vector.z;
                scene::camera->direction = vector;
            }
            else if (cameraCmd == "Up") {
                input >> vector.x >> vector.y >> vector.z;
                scene::camera->up = vector;
            }
            else if (cameraCmd == "FOV") {
                input >> scalar;
                scene::camera->FOV = scalar;
            }
        }
        else if (cmd == "Color") {
            input >> color.x >> color.y >> color.z;
        }
    }

    // Update camera transform
    scene::camera->update();
}