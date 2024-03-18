#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "stb_image_write.h"

#include <LiteMath.h>
#include <Image2d.h>

#include <limits>
#include <vector>

// Test runtime
#include <chrono>
#include <iostream>
#include <omp.h>

// Load shaders and scene
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>

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

namespace gpu {
    GLFWwindow* window;
    GLuint shader;
}

void loadScene(const char *path) {
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

// Check if the position is inside of scene boundaries
bool inside(float3 position) {
    Body::Box *box = static_cast<Body::Box*>(scene::objects.get(scene::boundsID));
    return all_of(abs(position - box->position) < box->size / 2);
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
    for (int _ = 0; _ < constants::iterations::raymarch; _++) {
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

void calculatePixel(Image2D<float4> &image, const uint width, const uint height, int2 coord) {
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

/// Render scene objects ///
void renderCPU(Image2D<float4> &image, const uint width, const uint height) {
    for (int pi = 0; pi < height; pi++) {
        for (int pj = 0; pj < width; pj++) {
            int2 coord(pj, pi);
            calculatePixel(image, width, height, coord);
        }
    }
}

void renderOMP(Image2D<float4> &image, const uint width, const uint height) {
    #pragma omp parallel for
    for (int pi = 0; pi < height; pi++) {
        for (int pj = 0; pj < width; pj++) {
            int2 coord(pj, pi);
            calculatePixel(image, width, height, coord);
        }
    }
}

const char* loadShader(const char *path) {
    std::string source;
    std::string line;
    std::ifstream file(path);
    if (file.is_open()) {
        while (std::getline(file, line)) {
            source += line + '\n';
        }
        file.close();
    }

    char *result = new char[source.length() + 1];
    strcpy(result, source.c_str());

    return result;
}

int setupGPU(const uint width, const uint height) {
    /// GLFW and GLAD init ///
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); 

    gpu::window = glfwCreateWindow(width, height, "Raymarching", NULL, NULL);
    if (!gpu::window) {
        std::cout << "[Error] Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(gpu::window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "[Error] Failed to init GLAD" << std::endl;
        return -1;
    }

    const char *vertexShaderSource = loadShader("source/shaders/shader.vert");
    const char *fragmentShaderSource = loadShader("source/shaders/shader.frag");

    /// Load shaders ///
    int success;
    char log[1 << 10];

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 1 << 10, NULL, log);
        std::cout << "[Error] Vertex Shader failed to load" << std::endl;
        std::cout << log << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 1 << 10, NULL, log);
        std::cout << "[Error] Fragment Shader failed to load" << std::endl;
        std::cout << log << std::endl;
    }

    gpu::shader = glCreateProgram();
    glAttachShader(gpu::shader, vertexShader);
    glAttachShader(gpu::shader, fragmentShader);
    glLinkProgram(gpu::shader);
    glGetProgramiv(gpu::shader, GL_LINK_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(gpu::shader, 1 << 10, NULL, log);
        std::cout << "[Error] Fragment Shader failed to load" << std::endl;
        std::cout << log << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    delete[] vertexShaderSource;
    delete[] fragmentShaderSource;

    return 0;
}

int renderGPU(unsigned char *image, const uint width, const uint height) {
    /// Screen space triangles ///
    float vertices[] = {
        -1.0f,  1.0f,
         1.0f,  1.0f,
        -1.0f, -1.0f,

        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f
    };

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Generate texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Generate RBO
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH32F_STENCIL8, width, height);

    GLuint buffer;
    glGenFramebuffers(1, &buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, buffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    // Render to buffer
    glBindFramebuffer(GL_FRAMEBUFFER, buffer);
    glViewport(0, 0, width, height);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(gpu::shader);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glfwTerminate();

    return 0;
}

int main() {
    const uint width = 768, height = 768;
    Image2D<float4> image(width, height);

    /// Init scene objects ///
    std::cout << "...Loading scene" << std::endl;
    loadScene("scene/objects.txt");

    /// Render time ///
    std::cout << "...Rendering" << std::endl;
    std::chrono::time_point<std::chrono::system_clock> start, end;
    std::chrono::duration<double> duration;

    // CPU
    start = std::chrono::system_clock::now();
    renderCPU(image, width, height);
    end = std::chrono::system_clock::now();
    duration = end - start;
    std::cout << "Rendering with CPU (1 thread):\t\t" << duration.count() << "s" << std::endl;

    // OpenMP
    start = std::chrono::system_clock::now();
    renderOMP(image, width, height);
    end = std::chrono::system_clock::now();
    duration = end - start;
    std::cout << "Rendering with OpenMP (4 threads):\t" << duration.count() << "s" << std::endl;
    SaveImage("out_cpu.png", image);

    // GPU
    unsigned char *data = new unsigned char[width * height * 4];
    setupGPU(width, height);
    start = std::chrono::system_clock::now();
    renderGPU(data, width, height);
    end = std::chrono::system_clock::now();
    duration = end - start;
    std::cout << "Rendering with GPU (sphere):\t\t" << duration.count() << "s" << std::endl;
    stbi_flip_vertically_on_write(true);
    stbi_write_jpg("out_gpu.png", width, height, 4, data, 100);

    return 0;
}
