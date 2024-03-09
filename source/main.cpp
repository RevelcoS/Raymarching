#include <LiteMath.h>
#include <Image2d.h>
#include <functional>
#include <iostream>

using namespace LiteMath;
using namespace LiteImage;

struct Box {
    float3 position;
    float3 size;
};

struct Sphere {
    float3 position;
    float radius;
    float3 color;
};

struct Light {
    float3 position;
};

namespace constants {
    const int iterations = 100;
    const float saturation = 0.05f;
}

namespace scene {
    Box bounds  = Box { float3(0.0f), float3(100.0f) };
    Light light = Light { float3(-15.0f, 15.0f, 15.0f) };
    Sphere obj  = Sphere { float3(0.0f, 0.0f, 25.0f), 5.0f, float3(1.0f, 0.0f, 0.0f) };
}

bool inside(const Box &obj, float3 position) {
    return all_of(position - obj.position < obj.size / 2);
}

float SDF(Sphere &obj, float3 position) {
    return length(obj.position - position) - obj.radius;
}

float3 grad(std::function<float(Sphere&, float3)> f, Sphere &obj, float3 p) {
    static const float h = 1e-3f;
    float3 dx = float3(h, 0.0f, 0.0f);
    float3 dy = float3(0.0f, h, 0.0f);
    float3 dz = float3(0.0f, 0.0f, h);
    float dfdx = f(obj, p + dx) - f(obj, p - dx);
    float dfdy = f(obj, p + dy) - f(obj, p - dy);
    float dfdz = f(obj, p + dz) - f(obj, p - dz);
    return float3(dfdx, dfdy, dfdz) / (2 * h);
}

void print(float3 vector) {
    std::cout << vector.x << " " << vector.y << " " << vector.z << std::endl;
}

float lighting(Light &light, float3 position, float3 normal) {
    return max(constants::saturation, dot(normal, normalize(light.position - position)));
}

float4 raymarch(Sphere &obj, float3 ray) {
    float3 position(0.0f);
    bool hit = true;
    for (int _ = 0; _ < constants::iterations; _++) {
        float distance = SDF(obj, position);
        position += distance * ray;
        hit = inside(scene::bounds, position);
        if (!hit) break;
    }

    float3 color = float3(0.0f);
    if (hit) {
        float3 normal = normalize(grad(SDF, obj, position));
        float light = lighting(scene::light, position, normal);
        color = light * obj.color;
    }

    return float4(color.x, color.y, color.z, 1.0f);
}

int main() {
    const uint width = 768, height = 768;
    const float AR = float(width) / height;
    Image2D<float4> image(width, height);

    for (int pi = 0; pi < height; pi++) {
        for (int pj = 0; pj < width; pj++) {
            int2 coord(pj, pi);
            float u = (pj + 0.5) / width;
            float v = (pi + 0.5) / height;

            float x = lerp(-AR/2, AR/2, u);
            float y = lerp( (float)1/2, (float)-1/2, v);
            float z = 1.0f;
            float3 ray = normalize( float3(x, y, z) );

            float4 color = raymarch(scene::obj, ray);
            image[coord] = color;
        }
    }

    SaveImage("images/output.png", image);

    return 0;
}
