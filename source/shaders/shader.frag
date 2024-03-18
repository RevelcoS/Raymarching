#version 330 core

out vec4 outColor;

in vec2 uv;

int iterations = 1000;
float saturation = 0.05f;

vec3 spherePos = vec3(2.0f, 0.0f, 25.0f);
float sphereRadius = 5.0f;
vec3 sphereColor = vec3(1.0f, 0.0f, 0.0f);

vec3 boundsPosition = vec3(0.0f);
vec3 bounds = vec3(200.0f);

vec3 lightPosition = vec3(50.0f, 10.0f, -20.0f);

bool inside(vec3 position) {
    return all(lessThan(abs(position - boundsPosition), bounds));
}

float lighting(vec3 position, vec3 normal) {
    return max(saturation, dot(normal, normalize(lightPosition - position)));
}

float SDF(vec3 position) {
    return length(spherePos - position) - sphereRadius;
}

vec3 grad(vec3 position) {
    float h = 1e-3f;
    vec3 dx = vec3(h, 0.0f, 0.0f);
    vec3 dy = vec3(0.0f, h, 0.0f);
    vec3 dz = vec3(0.0f, 0.0f, h);
    float dfdx = SDF(position + dx) - SDF(position - dx);
    float dfdy = SDF(position + dy) - SDF(position - dy);
    float dfdz = SDF(position + dz) - SDF(position - dz);
    return vec3(dfdx, dfdy, dfdz) / (2 * h);
}

vec3 raymarch(vec3 ray) {
    vec3 position = vec3(0.0f);
    bool hit = true;
    for (int i = 0; i < iterations; i++) {
        float dist = SDF(position);
        position += dist * ray;
        hit = inside(position);
        if (!hit) {
            break;
        }
    }

    vec3 color = vec3(0.0f);
    if (hit) {
        vec3 normal = normalize(grad(position));
        float light = lighting(position, normal);
        color = sphereColor * light;
    }

    return color;
}

void main() {
    vec3 ray = vec3(0.5 * uv.x, 0.5 * uv.y, 1.0f);
    vec3 color = raymarch(ray); 
    outColor = vec4(color, 1.0f);
}
