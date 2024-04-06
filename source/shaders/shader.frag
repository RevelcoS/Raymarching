#version 430 core

#define BODY_ELEMENTS   4               // Length of Body struct vec4 array
#define BODY_TYPES      20              // Limit number of Body Types
#define BODY_MAX        (1 << 10)       // Amount of Bodies array contains
#define LIST_ENTRIES    (1 << 6)        // Number of Lists
#define LIST_MAX        (1 << 10)       // Amount of Nodes List contains
#define STACK_MAX       (1 << 6)        // Amount of Items in Stack

out vec4 outColor;

in vec2 uv;

struct Light {
    vec3 position;
    vec3 color;
};

/// Scene Bodies ///
struct Surface {
    float SD;
    vec3 color;
};

struct Sphere {
    vec3 position;
    float radius;
    vec3 color;
};

struct Box {
    vec3 position;
    vec3 size;
    vec3 color;
};

struct Cross {
    vec3 position;
    vec3 size;
    vec3 color;
};

uniform int iterations;
uniform float saturation;
uniform float hitPrecision;

uniform Light light;
uniform Box bounds;

/// SSBO elements ///
struct Body {
    vec4 data[BODY_ELEMENTS];
};

// List Node
struct Node {
    uvec4 type; // Mode OR Type
    uvec4 ID;   // Total OR ID
};

/// SSBOs ///
layout (std430, binding = 0) buffer Bodies {
    Body bodies[BODY_TYPES * BODY_MAX];
};

layout (std430, binding = 1) buffer Tree {
    Node tree[LIST_ENTRIES * LIST_MAX];
};

/// Stack ///
struct Item {
    uint ID;
    uint offset;
    Surface surface;
};

Item stack[STACK_MAX];
uint size = 0;

void stackClear() {
    size = 0;
}

bool stackEmpty() {
    return size == 0;
}

void stackPush(Item item) {
    stack[size++] = item;    
}

Item stackPop() {
    return stack[--size];
}

/// List & Body operations ///
Node listPull(uint ID, uint offset) {
    return tree[ID * LIST_MAX + offset];
}

Node listMeta(uint ID) {
    return tree[ID * LIST_MAX];
}

bool listIsBase(uint offset) {
    return offset == 1;
}

Body bodyPull(uint type, uint ID) {
    return bodies[type * BODY_MAX + ID];
}

/// Surface operations ///
Surface opUnion(Surface s1, Surface s2) {
    if (s1.SD < s2.SD) {
        return s1;
    }
    return s2;
}

Surface opComplement(Surface s) {
    return Surface(-s.SD, s.color);
}

Surface opUComplement(Surface s1, Surface s2) {
    return opUnion(s1, opComplement(s2));
}

Surface opIntersection(Surface s1, Surface s2) {
    if (s1.SD > s2.SD) {
        return s1;
    }
    return s2;
}

Surface opDifference(Surface s1, Surface s2) {
    return opIntersection(s1, opComplement(s2));
}

/// Body SDFs ///
Surface sphereSDF(Sphere obj, vec3 position) {
    float sd = length(obj.position - position) - obj.radius;
    return Surface(sd, obj.color);
}

Surface boxSDF(Box obj, vec3 position) {
    vec3 distances = abs(position - obj.position) - obj.size / 2;
    float sd = max(max(distances.x, distances.y), distances.z);
    return Surface(sd, obj.color);
}

Surface crossSDF(Cross obj, vec3 position) {
    vec3 distances = abs(position - obj.position) - obj.size / 2;
    float dmin = min(min(distances.x, distances.y), distances.z);
    float dmax = max(max(distances.x, distances.y), distances.z);
    float sd = distances.x + distances.y + distances.z - dmin - dmax;
    return Surface(sd, obj.color);
}

Surface emptySDF() {
    return Surface( 1.0f / 0.0f, vec3(1.0f) );
}

Surface bodySDF(uint type, Body body, vec3 position) {
    if (type == 1) {
        Sphere obj = Sphere(body.data[0].xyz, body.data[1].x, body.data[2].xyz);
        return sphereSDF(obj, position);
    } else if (type == 2) {
        Box obj = Box(body.data[0].xyz, body.data[1].xyz, body.data[2].xyz);
        return boxSDF(obj, position);
    } else if (type == 3) {
        Cross obj = Cross(body.data[0].xyz, body.data[1].xyz, body.data[2].xyz);
        return crossSDF(obj, position);
    }
    return emptySDF();
}

Surface listApply(uint mode, Surface left, Surface right, bool base) {
    if (base) {
        if (mode == 1)
            right = opComplement(right);
        return right;
    }

    if (mode == 0) return opUnion(left, right);
    else if (mode == 1) return opUComplement(left, right);
    else if (mode == 2) return opIntersection(left, right);
    else if (mode == 3) return opDifference(left, right);
    return left;
}

/// Scene ///
bool inside(vec3 position) {
    return all(lessThan(abs(position - bounds.position), bounds.size / 2));
}

float lighting(vec3 position, vec3 normal) {
    return max(saturation, dot(normal, normalize(light.position - position)));
}

Surface SDF(vec3 position) {
    stackClear();
    Item top = Item(0, 0, emptySDF());
    stackPush(top);

    uint total = 0;

    while (!stackEmpty()) {
        Node meta = listMeta(top.ID);
        bool base = listIsBase(++top.offset);
        if (top.offset > meta.ID.x) {
            // List end
            Item pop = stackPop();
            meta = listMeta(pop.ID);
            base = listIsBase(pop.offset);

            top.ID = pop.ID;
            top.offset = pop.offset;
            top.surface = listApply(meta.type.x, pop.surface, top.surface, base);
            continue;
        }

        Node node = listPull(top.ID, top.offset);
        if (node.type.x == 0) {
            // List node
            stackPush(top);
            top.ID = node.ID.x;
            top.offset = 0;
            top.surface = emptySDF();

        } else {
            // Body node
            Body body = bodyPull(node.type.x, node.ID.x);
            Surface surface = bodySDF(node.type.x, body, position);
            top.surface = listApply(meta.type.x, top.surface, surface, base);
        }
    }

    return top.surface;
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
        if (!hit || result.SD < hitPrecision)
            break;
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
