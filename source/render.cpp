#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <LiteMath.h>
#include <Image2d.h>

// Load shaders
#include <string>
#include <fstream>
#include <iostream>

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
    static float4 raymarch(float3 ray);
    static void pixel(Image2D<float4> &image,
            const uint width, const uint height, int2 coord);

    /// GPU ///
    GLFWwindow* window;
    static GLFWwindow* context(const uint width, const uint height);

    GLuint VBO, VAO;
    GLuint texture;
    GLuint RBO, FBO;
    static void genbuffers(const uint width, const uint height);

    namespace shader {
        GLuint program;
        static GLuint load(const char *path, GLenum type);
        static GLuint link(GLuint vertex, GLuint fragment);
        static void log(GLuint shader, GLenum status, GLenum type = 0);
    };
}

///////////////////////////////////////////
///                 CPU                 ///
///////////////////////////////////////////

// Calculate the color produced by ray
float4 render::raymarch(float3 ray) {
    float3 position(0.0f);
    Body::Base *obj;
    bool hit = true;
    for (int _ = 0; _ < constants::iterations; _++) {
        float distance = scene::SDF(position, &obj);
        position += distance * ray;
        hit = scene::inside(position);
        if (!hit || distance < constants::precision) break;
    }

    float3 color = float3(0.0f);
    if (hit) {
        float3 normal = normalize(scene::grad(obj, position));
        float light = scene::lighting(position, normal);
        color = light * obj->color;
    }

    return float4(color.x, color.y, color.z, 1.0f);
}

// Calculate pixel at the given image coord
void render::pixel(Image2D<float4> &image, const uint width, const uint height, int2 coord) {
    static const float AR = float(width) / height;

    float u = (coord.x + 0.5) / width;
    float v = (coord.y + 0.5) / height;

    float x = lerp(-AR/2, AR/2, u);
    float y = lerp( (float)1/2, (float)-1/2, v);
    float z = 1.0f;
    float3 ray = normalize( float3(x, y, z) );

    float4 color = raymarch(ray);
    image[coord] = color;
}

void render::CPU(Image2D<float4> &image, const uint width, const uint height) {
    for (int pi = 0; pi < height; pi++) {
        for (int pj = 0; pj < width; pj++) {
            int2 coord(pj, pi);
            pixel(image, width, height, coord);
        }
    }
}

void render::OMP(Image2D<float4> &image, const uint width, const uint height) {
    #pragma omp parallel for
    for (int pi = 0; pi < height; pi++) {
        for (int pj = 0; pj < width; pj++) {
            int2 coord(pj, pi);
            pixel(image, width, height, coord);
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

GLuint render::shader::link(GLuint vertex, GLuint fragment) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

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

// Setup GLFW and GLAD context
GLFWwindow* render::context(const uint width, const uint height) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(width, height, constants::title, NULL, NULL);
    if (!window) {
        std::cout << "[Error] Failed to create window" << std::endl;
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "[Error] Failed to init GLAD" << std::endl;
    }
    
    return window;
}

void render::genbuffers(const uint width, const uint height) {
    /// Generate VAO and VBO ///
    glGenBuffers(1, &render::VBO);
    glBindBuffer(GL_ARRAY_BUFFER, render::VBO);

    glGenVertexArrays(1, &render::VAO);
    glBindVertexArray(render::VAO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0); 

    /// Generate texture ///
    glGenTextures(1, &render::texture);
    glBindTexture(GL_TEXTURE_2D, render::texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    /// Generate RBO and FBO ///
    glGenRenderbuffers(1, &render::RBO);
    glBindRenderbuffer(GL_RENDERBUFFER, render::RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH32F_STENCIL8, width, height);

    glGenFramebuffers(1, &render::FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, render::FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, render::texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
            GL_RENDERBUFFER, render::RBO);

    // Render to buffer
    glBindFramebuffer(GL_FRAMEBUFFER, render::FBO);
    glViewport(0, 0, width, height);
}

void render::setup(const uint width, const uint height) {
    /// Setup context ///
    render::window = render::context(width, height);
 
    /// Compile shaders ///
    GLuint vertex = render::shader::load("source/shaders/shader.vert", GL_VERTEX_SHADER);
    render::shader::log(vertex, GL_COMPILE_STATUS, GL_VERTEX_SHADER); 

    GLuint fragment = render::shader::load("source/shaders/shader.frag", GL_FRAGMENT_SHADER);
    render::shader::log(fragment, GL_COMPILE_STATUS, GL_FRAGMENT_SHADER);

    /// Link program ///
    render::shader::program = render::shader::link(vertex, fragment);
    render::shader::log(render::shader::program, GL_LINK_STATUS);

    /// Generate buffers ///
    render::genbuffers(width, height);
}

void render::push(void) {
    /// Screen space triangles ///
    float vertices[] = {
        -1.0f,  1.0f,
         1.0f,  1.0f,
        -1.0f, -1.0f,

        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
}

void render::GPU(unsigned char *image, const uint width, const uint height) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(render::shader::program);
    glBindVertexArray(render::VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
}

void render::destroy() {
    glfwTerminate();
}
