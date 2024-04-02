#version 430 core

#define MAX 1 << 10

out vec4 outColor;

in vec2 uv;

struct Surface {
    float SD;
    vec3 color;
};

struct Sphere {
    vec4 position;
    vec4 radius;
    vec4 color;
};

layout (std140) uniform Spheres {
    Sphere sphere[MAX];
};
uniform int sphereTotal;

int iterations = 1000;
float saturation = 0.05f;

vec3 boundsPosition = vec3(0.0f);
vec3 bounds = vec3(200.0f);

vec3 lightPosition = vec3(50.0f, 10.0f, -20.0f);

bool inside(vec3 position) {
    return all(lessThan(abs(position - boundsPosition), bounds));
}

float lighting(vec3 position, vec3 normal) {
    return max(saturation, dot(normal, normalize(lightPosition - position)));
}

Surface sphereSurface(Sphere obj, vec3 position) {
    float sd = length(obj.position.xyz - position) - obj.radius.x;
    return Surface(sd, obj.color.xyz);
}

Surface unionSurface(Surface s1, Surface s2) {
    if (s1.SD < s2.SD) {
        return s1;
    }
    return s2;
}

Surface SDF(vec3 position) {
    Surface result = Surface( 1.0f/0.0f, vec3(1.0f) );
    for (int i = 0; i < sphereTotal; i++) {
        Surface surface = sphereSurface(sphere[i], position);
        result = unionSurface(result, surface);
    }
    return result;
}

vec3 grad(vec3 position) {
    float h = 1e-3f;
    vec3 dx = vec3(h, 0.0f, 0.0f);
    vec3 dy = vec3(0.0f, h, 0.0f);
    vec3 dz = vec3(0.0f, 0.0f, h);

    Surface dxl = SDF(position + dx);
    Surface dxr = SDF(position - dx);
    float dfdx = dxl.SD - dxr.SD;

    Surface dyl = SDF(position + dy);
    Surface dyr = SDF(position - dy);
    float dfdy = dyl.SD - dyr.SD;

    Surface dzl = SDF(position + dz);
    Surface dzr = SDF(position - dz);
    float dfdz = dzl.SD - dzr.SD;

    return vec3(dfdx, dfdy, dfdz) / (2 * h);
}

vec3 raymarch(vec3 ray) {
    Surface result;
    result.color = vec3(1.0f);
    vec3 position = vec3(0.0f);
    bool hit = true;
    for (int i = 0; i < iterations; i++) {
        result = SDF(position);
        position += result.SD * ray;
        hit = inside(position);
        if (!hit) {
            break;
        }
    }

    vec3 color = vec3(0.0f);
    if (hit) {
        vec3 normal = normalize(grad(position));
        float light = lighting(position, normal);
        color = result.color * light;
    }

    return color;
}

void main() {
    vec3 ray = vec3(0.5 * uv.x, 0.5 * uv.y, 1.0f);
    vec3 color = raymarch(ray);
    outColor = vec4(color, 1.0f);
}
