#include <LiteMath.h>
#include <Image2d.h>
#include <limits>
#include <functional>
#include <vector>
#include <iostream>

#include "constants.h"
#include "object.h"

using namespace LiteMath;
using namespace LiteImage;

namespace scene {
    Object::Container objects;
    std::vector<int> bodyIDs;
    int boundsID;
    int lightID;
}


bool inside(float3 position) {
    const Object::Box *box = reinterpret_cast<Object::Box*>(scene::objects.get(scene::boundsID));
    return all_of(position - box->position < box->size / 2);
}

// Calculate SDF of object depending on obj->type
float SDFobj(float3 position, Object::Base *obj) {
    float distance = std::numeric_limits<float>::infinity();
    switch (obj->type) {
        case Object::Type::SPHERE:
        {
            Object::Sphere *sphere = reinterpret_cast<Object::Sphere*>(obj);
            distance = length(sphere->position - position) - sphere->radius;
            break;
        }
        case Object::Type::BOX:
        {
            /*
            Object::Box *box = reinterpret_cast<Object::Box*>(obj);
            float3 distances = abs(position - box->position) - box->size / 2;
            if (box->inverse) {
                distance = min(distances); // not always
            } else {
                // ...
            }
            */
            break;
        }
        default: break;
    }
    return distance;
}

// Calculate SDF from scene objects and return <active> hit object
float SDF(float3 position, Object::Base** active = nullptr) {
    float result = std::numeric_limits<float>::infinity();

    for (auto bodyID : scene::bodyIDs) {
        Object::Base *obj = scene::objects.get(bodyID);
        float distance = SDFobj(position, obj);

        // Choose the closest object
        if (distance < result) {
            result = distance;
            if (active) *active = obj;
        }
    }

    return result;
}

float3 grad(std::function<float(float3, Object::Base*)> f, Object::Base* &obj, float3 p) {
    static const float h = 1e-3f;
    float3 dx = float3(h, 0.0f, 0.0f);
    float3 dy = float3(0.0f, h, 0.0f);
    float3 dz = float3(0.0f, 0.0f, h);
    float dfdx = f(p + dx, obj) - f(p - dx, obj);
    float dfdy = f(p + dy, obj) - f(p - dy, obj);
    float dfdz = f(p + dz, obj) - f(p - dz, obj);
    return float3(dfdx, dfdy, dfdz) / (2 * h);
}

void print(float3 vector) {
    std::cout << vector.x << " " << vector.y << " " << vector.z << std::endl;
}

float lighting(float3 position, float3 normal) {
    Object::Light *light = reinterpret_cast<Object::Light*>(scene::objects.get(scene::lightID));
    return max(constants::saturation, dot(normal, normalize(light->position - position)));
}

float4 raymarch(float3 ray) {
    float3 position(0.0f);
    Object::Base *obj;
    bool hit = true;
    for (int _ = 0; _ < constants::iterations; _++) {
        float distance = SDF(position, &obj);
        position += distance * ray;
        hit = inside(position);
        if (!hit || distance < constants::precision) break;
    }

    float3 color = float3(0.0f);
    if (hit) {
        float3 normal = normalize(grad(SDFobj, obj, position));
        float light = lighting(position, normal);
        color = light * obj->color;
    }

    return float4(color.x, color.y, color.z, 1.0f);
}

int main() {
    const uint width = 768, height = 768;
    const float AR = float(width) / height;
    Image2D<float4> image(width, height);

    /// Init scene objects ///
    Object::Box *bounds = new Object::Box ( float3(0.0f), float3(100.0f) );
    scene::boundsID = scene::objects.add(bounds);

    Object::Light *light = new Object::Light ( float3(-15.0f, 15.0f, 15.0f) );
    scene::lightID = scene::objects.add(light);

    int ID;

    // Red
    Object::Sphere *sphere = new Object::Sphere ( float3(2.0f, 0.0f, 25.0f), 5.0f, float3(1.0f, 0.0f, 0.0f) );
    ID = scene::objects.add(sphere);
    scene::bodyIDs.push_back(ID);

    // Blue
    sphere = new Object::Sphere ( float3(-4.0f, -3.0f, 18.0f), 5.0f, float3(0.0f, 0.0f, 1.0f) );
    ID = scene::objects.add(sphere);
    scene::bodyIDs.push_back(ID);

    // Green
    sphere = new Object::Sphere ( float3(15.0f, 5.0f, 45.0f), 5.0f, float3(0.0f, 1.0f, 0.0f) );
    ID = scene::objects.add(sphere);
    scene::bodyIDs.push_back(ID);

    /// Render scene objects ///
    for (int pi = 0; pi < height; pi++) {
        for (int pj = 0; pj < width; pj++) {
            int2 coord(pj, pi);
            float u = (pj + 0.5) / width;
            float v = (pi + 0.5) / height;

            float x = lerp(-AR/2, AR/2, u);
            float y = lerp( (float)1/2, (float)-1/2, v);
            float z = 1.0f;
            float3 ray = normalize( float3(x, y, z) );

            float4 color = raymarch(ray);
            image[coord] = color;
        }
    }

    SaveImage("images/output.png", image);

    return 0;
}
