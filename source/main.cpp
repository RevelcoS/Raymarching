#include <LiteMath.h>
#include <Image2d.h>
#include <limits>
#include <vector>
#include <iostream>

#include "constants.h"
#include "body.h"
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
    Body::Box *box = static_cast<Body::Box*>(scene::objects.get(scene::boundsID));
    return all_of(position - box->position < box->size / 2);
}

// Calculate SDF from scene objects and return <active> hit object
float SDF(float3 position, Body::Base** active = nullptr) {
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

float3 grad(Body::Base* &obj, float3 p) {
    static const float h = 1e-3f;
    float3 dx = float3(h, 0.0f, 0.0f);
    float3 dy = float3(0.0f, h, 0.0f);
    float3 dz = float3(0.0f, 0.0f, h);
    float dfdx = obj->SDF(p + dx) - obj->SDF(p - dx);
    float dfdy = obj->SDF(p + dy) - obj->SDF(p - dy);
    float dfdz = obj->SDF(p + dz) - obj->SDF(p - dz);
    return float3(dfdx, dfdy, dfdz) / (2 * h);
}

void print(float3 vector) {
    std::cout << vector.x << " " << vector.y << " " << vector.z << std::endl;
}

float lighting(float3 position, float3 normal) {
    Object::Light *light = static_cast<Object::Light*>(scene::objects.get(scene::lightID));
    return max(constants::saturation, dot(normal, normalize(light->position - position)));
}

float4 raymarch(float3 ray) {
    float3 position(0.0f);
    Body::Base *obj;
    bool hit = true;
    for (int _ = 0; _ < constants::iterations; _++) {
        float distance = SDF(position, &obj);
        position += distance * ray;
        hit = inside(position);
        if (!hit || distance < constants::precision) break;
    }

    float3 color = float3(0.0f);
    if (hit) {
        float3 normal = normalize(grad(obj, position));
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
    Body::Box *bounds = new Body::Box ( float3(0.0f), float3(100.0f) );
    scene::boundsID = scene::objects.add(bounds);

    Object::Light *light = new Object::Light ( float3(50.0f, 10.0f, -20.0f) );
    scene::lightID = scene::objects.add(light);

    int ID;

    // Red
    Body::Sphere *sphere = new Body::Sphere ( float3(2.0f, 0.0f, 25.0f), 5.0f, float3(1.0f, 0.0f, 0.0f) );
    ID = scene::objects.add(sphere);
    scene::bodyIDs.push_back(ID);

    // Compound blue
    Body::Sphere *sphere1 = new Body::Sphere ( float3(-4.0f, -3.0f, 15.0f), 5.0f, float3(0.0f) );
    Body::Sphere *sphere2 = new Body::Sphere ( float3(-4.0f, -3.0f, 8.0f), 5.0f, float3(0.0f) );
    Body::Compound *compound = new Body::Compound( sphere1, sphere2, float3(0.0f, 0.0f, 1.0f),
            Body::Mode::DIFFERENCE );
    ID = scene::objects.add(compound);
    scene::bodyIDs.push_back(ID);

    // Green
    sphere = new Body::Sphere ( float3(15.0f, 5.0f, 45.0f), 5.0f, float3(0.0f, 1.0f, 0.0f) );
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
