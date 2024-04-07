#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <LiteMath.h>
#include <Image2d.h>

// Load shaders
#include <string>
#include <fstream>
#include <iostream>

// SSBOs
#include <cstring>

// Parallel processing
#include <omp.h>

#include "constants.h"
#include "body.h"
#include "scene.h"
#include "render.h"

using namespace LiteMath;
using namespace LiteImage;

namespace render {

    /// CPU ///
    static float3 raymarch(float3 ray);
    static void pixel(Image2D<float4> &image, int2 coord);

    /// GPU ///
    GLFWwindow* window;

    GLuint texture;
    GLuint bodySSBO;
    GLuint treeSSBO;
    static void gentexture(void);
    static uint type(Body::Type type);
    static uint mode(Body::Mode mode);
    static void genssbo(const char *name, GLuint &ssbo, uint binding);
    static void pushssbo(GLuint ssbo, void *data, size_t size);
    static void pushuniforms(void);

    namespace shader {
        GLuint program;
        static GLuint load(const char *path, GLenum type);
        static GLuint link(GLuint compute);
        static void log(GLuint shader, GLenum status, GLenum type = 0);

        struct Body {
            float data[4 * constants::gpu::bodyElements];
        };

        // List Node
        struct Node {
            uint type[4]; // Mode OR Type
            uint ID[4];   // Total OR ID
        };

        // Stack Item
        struct Item {
            uint ID[4];
            uint offset[4];
        };

        static void packbody(::Body::Base *in, Body *out);
        static void genscene(
            Body bodies[constants::gpu::bodyTypes * constants::gpu::bodyMax],
            Node tree[constants::gpu::listEntries * constants::gpu::listMax]);
    };
}

///////////////////////////////////////////
///                 CPU                 ///
///////////////////////////////////////////

// Calculate the color produced by ray
float3 render::raymarch(float3 ray) {
    float3 position(0.0f);
    Body::Surface surface {};
    bool hit = true;
    for (int _ = 0; _ < constants::iterations; _++) {
        surface = scene::SDF(position);
        position += surface.SD * ray;
        hit = scene::inside(position);
        if (!hit || surface.SD < constants::precision) break;
    }

    float3 color = float3(0.0f);
    if (hit) {
        float3 normal = normalize(scene::grad(position));
        float light = scene::lighting(position, normal);
        color = light * surface.color;
    }

    return color;
}

// Calculate pixel at the given image coord
void render::pixel(Image2D<float4> &image, int2 coord) {
    static const float AR = float(constants::width) / constants::height;

    float2 s1 = float2( -AR/2, (float) 1/2 ); // screen top left corner
    float2 s2 = float2(  AR/2, (float)-1/2 ); // screen bottom right corner

    float2 psize = float2( (float) 1 / constants::width, (float) 1 / constants::height ); // pixel size

    // screen space UV
    float2 uv1      = float2(coord) * psize;
    int2 offset     = int2(1, 1);
    float2 uv2      = float2(coord + offset) * psize;

    float2 p1       = float2( lerp( s1.x, s2.x, uv1.x), lerp( s1.y, s2.y, uv1.y) ); // pixel top left corner
    float2 p2       = float2( lerp( s1.x, s2.x, uv2.x), lerp( s1.y, s2.y, uv2.y) ); // pixel bottom right corner

    float3 total = float3(0.0f);
    for (int i = 0; i < constants::SSAA::kernel; i++) {
        for (int j = 0; j < constants::SSAA::kernel; j++) {
            float2 uv = float2( i + 1, j + 1 ) / constants::SSAA::kernel;
            float x = lerp( p1.x, p2.x, uv.x);
            float y = lerp( p1.y, p2.y, uv.y);
            float z = 1.0f;
            float3 ray = normalize( float3(x, y, z) );
            float3 color = raymarch(ray);
            total += color;
        }
    }

    float3 color = total / (constants::SSAA::kernel * constants::SSAA::kernel);
    image[coord] = float4(color.x, color.y, color.z, 1.0f);
}

void render::CPU(Image2D<float4> &image) {
    for (int pi = 0; pi < constants::height; pi++) {
        for (int pj = 0; pj < constants::width; pj++) {
            int2 coord(pj, pi);
            pixel(image, coord);
        }
    }
}

void render::OMP(Image2D<float4> &image) {
    #pragma omp parallel for
    for (int pi = 0; pi < constants::height; pi++) {
        for (int pj = 0; pj < constants::width; pj++) {
            int2 coord(pj, pi);
            pixel(image, coord);
        }
    }
}

///////////////////////////////////////////
///                 GPU                 ///
///////////////////////////////////////////

GLuint render::shader::load(const char *path, GLenum type) {
    std::string source;
    std::string line;
    std::ifstream file(path);
    if (file.is_open()) {
        while (std::getline(file, line)) {
            source += line + '\n';
        }
        file.close();
    }

    const char *csource = source.c_str();
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &csource, NULL);
    glCompileShader(shader);

    return shader;
}

GLuint render::shader::link(GLuint compute) {
    GLuint program = glCreateProgram();
    glAttachShader(program, compute);
    glLinkProgram(program);
    glDeleteShader(compute);
    return program;
}

void render::shader::log(GLuint shader, GLenum status, GLenum type) {
    static char log[constants::logsize];
    int success;
    
    switch (status) {
        case GL_COMPILE_STATUS:
            glGetShaderiv(shader, status, &success); break;
        case GL_LINK_STATUS:
            glGetProgramiv(shader, status, &success); break;
        default: break;
    }

    glGetShaderiv(shader, status, &success);
    if (!success) {
        glGetShaderInfoLog(shader, constants::logsize, NULL, log);
        std::cout << "[Error] ";
        switch (status) {
            case GL_COMPILE_STATUS:
            {
                switch (type) {
                    case GL_VERTEX_SHADER:
                        std::cout << "Vertex "; break;
                    case GL_FRAGMENT_SHADER:
                        std::cout << "Fragment "; break;
                    case GL_COMPUTE_SHADER:
                        std::cout << "Compute "; break;
                    default: break;
                }
                std::cout << "Shader failed to load"; break;
            }
            case GL_LINK_STATUS:
            {
                std::cout << "Program failed to link"; break;
            }
            default: break;
        }
        std::cout << std::endl << log << std::endl;
    }
}

uint render::type(Body::Type type) {
    return static_cast<uint>(type);
}

uint render::mode(Body::Mode mode) {
    return static_cast<uint>(mode);
}

void render::genssbo(const char *name, GLuint &ssbo, uint binding) {
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void render::pushssbo(GLuint ssbo, void *data, size_t size) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void render::gentexture() {
    /// Generate texture ///
    glGenTextures(1, &render::texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, render::texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, constants::width, constants::height, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindImageTexture(0, render::texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
}

void render::shader::packbody(::Body::Base *in, render::shader::Body *out) {
    switch (in->type) {
        case ::Body::Type::SPHERE:
        {
            ::Body::Sphere *obj = static_cast<::Body::Sphere*>(in);
            std::memcpy(out->data, obj->position.M, sizeof(obj->position.M));
            std::memcpy(out->data + 4, &obj->radius, sizeof(obj->radius));
            std::memcpy(out->data + 8, obj->color.M, sizeof(obj->color.M));
            break;
        }
        case ::Body::Type::BOX:
        {
            ::Body::Box *obj = static_cast<::Body::Box*>(in);
            std::memcpy(out->data, obj->position.M, sizeof(obj->position.M));
            std::memcpy(out->data + 4, obj->size.M, sizeof(obj->size.M));
            std::memcpy(out->data + 8, obj->color.M, sizeof(obj->color.M));
            break;
        }
        case ::Body::Type::CROSS:
        {
            ::Body::Cross *obj = static_cast<::Body::Cross*>(in);
            std::memcpy(out->data, obj->position.M, sizeof(obj->position.M));
            std::memcpy(out->data + 4, obj->size.M, sizeof(obj->size.M));
            std::memcpy(out->data + 8, obj->color.M, sizeof(obj->color.M));
            break;
        }
        default: break;
    }
}

void render::shader::genscene(
    render::shader::Body bodies[constants::gpu::bodyTypes * constants::gpu::bodyMax],
    render::shader::Node tree[constants::gpu::listEntries * constants::gpu::listMax]) {

    render::shader::Item stack[constants::gpu::stackMax];
    ::Body::List* liststack[constants::gpu::stackMax];

    size_t bodySize[constants::gpu::bodyTypes];
    std::memset(bodySize, 0, sizeof(bodySize));
    size_t treeSize = 1;
    size_t stackSize = 1;

    render::shader::Node node { .type = { render::mode(scene::tree->mode) }, .ID = { 0U } }; // Metadata: Mode, Size
    tree[0] = node;
    render::shader::Node *entry = tree;

    render::shader::Item item { .ID = { 0U }, .offset = { 0U } };
    stack[0] = item;
    render::shader::Item *top = stack;

    ::Body::List *list = scene::tree;
    liststack[0] = list;

    ::Body::Base *body;
    render::shader::Body gpuBody;

    while (stackSize > 0) {
        top = &stack[stackSize - 1];
        list = liststack[stackSize - 1];
        uint listOffset = ++(top->offset[0]);
        uint listID = top->ID[0];
        if (listOffset > list->bodies.size()) {
            stackSize--; continue;
        }

        entry = tree + listID * constants::gpu::listMax;
        body = list->bodies[listOffset - 1];
        uint type = render::type(body->type);
        entry[0].ID[0] = listOffset;
        if (body->type == ::Body::Type::LIST) {
            // Update the stacks
            liststack[stackSize] = static_cast<::Body::List*>(body);
            item.ID[0] = treeSize;
            item.offset[0] = 0U;
            stack[stackSize] = item;

            // Push body node to the list
            node.type[0] = type;
            node.ID[0] = treeSize;
            entry[listOffset] = node;

            // Update new list metadata
            list = liststack[stackSize];
            uint mode = render::mode(list->mode);

            entry = tree + treeSize * constants::gpu::listMax;
            node.type[0] = mode;
            node.ID[0] = 0U;
            entry[0] = node;

            stackSize++; treeSize++;
        } else {
            // Update the bodies
            render::shader::packbody(body, &gpuBody);
            bodies[type * constants::gpu::bodyMax + bodySize[type]] = gpuBody;

            // Push body node to the list
            node.type[0] = type;
            node.ID[0] = bodySize[type]++;
            entry[listOffset] = node;
        }
    }
}

void render::pushuniforms(void) {
    GLuint uniform;

    // Constants
    uniform = glGetUniformLocation(render::shader::program, "width");
    glUniform1ui(uniform, constants::width);

    uniform = glGetUniformLocation(render::shader::program, "height");
    glUniform1ui(uniform, constants::height);

    uniform = glGetUniformLocation(render::shader::program, "iterations");
    glUniform1i(uniform, constants::iterations);

    uniform = glGetUniformLocation(render::shader::program, "saturation");
    glUniform1f(uniform, constants::saturation);

    uniform = glGetUniformLocation(render::shader::program, "hitPrecision");
    glUniform1f(uniform, constants::precision);

    uniform = glGetUniformLocation(render::shader::program, "kernelSize");
    glUniform1i(uniform, constants::SSAA::kernel);

    // Light
    uniform = glGetUniformLocation(render::shader::program, "light.position");
    glUniform3fv(uniform, 1, scene::light->position.M);

    uniform = glGetUniformLocation(render::shader::program, "light.color");
    glUniform3fv(uniform, 1, scene::light->color.M);

    // Bounds
    uniform = glGetUniformLocation(render::shader::program, "bounds.position");
    glUniform3fv(uniform, 1, scene::bounds->position.M);

    uniform = glGetUniformLocation(render::shader::program, "bounds.size");
    glUniform3fv(uniform, 1, scene::bounds->size.M);
}

// Setup GLFW and GLAD context
void render::setup::context() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    render::window = glfwCreateWindow(constants::width, constants::height, constants::title, NULL, NULL);
    if (!render::window) {
        std::cout << "[Error] Failed to create window" << std::endl;
        glfwTerminate();
    }
    glfwMakeContextCurrent(render::window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "[Error] Failed to init GLAD" << std::endl;
    }
}

void render::setup::shaders() {
    /// Compile shader ///
    GLuint compute = render::shader::load("source/shaders/shader.comp", GL_COMPUTE_SHADER);
    render::shader::log(compute, GL_COMPILE_STATUS, GL_COMPUTE_SHADER);

    /// Link program ///
    render::shader::program = render::shader::link(compute);
    render::shader::log(render::shader::program, GL_LINK_STATUS);
    glUseProgram(render::shader::program);
}

void render::setup::buffers() {
    /// Generate buffers ///
    render::gentexture();
    render::genssbo("Bodies", render::bodySSBO, 0);
    render::genssbo("Tree", render::treeSSBO, 1);
}

void render::push(void) {
    /// Fill uniforms ///
    render::pushuniforms();

    /// Fill SSBOs ///
    auto bodies = new render::shader::Body[constants::gpu::bodyTypes * constants::gpu::bodyMax];
    auto tree = new render::shader::Node[constants::gpu::listEntries * constants::gpu::listMax];

    render::shader::genscene(bodies, tree);

    render::pushssbo(render::bodySSBO, bodies, constants::gpu::bodyTypes * constants::gpu::bodyMax * sizeof(*bodies));
    render::pushssbo(render::treeSSBO, tree, constants::gpu::listEntries * constants::gpu::listMax * sizeof(*tree));

    delete[] bodies;
    delete[] tree;
}

void render::GPU(unsigned char *image) {
    glUseProgram(render::shader::program);
    glDispatchCompute(
        constants::width / constants::gpu::groupUnits,
        constants::height / constants::gpu::groupUnits, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
}

void render::destroy() {
    glfwTerminate();
}
