#include "stb_image_write.h"

#include <LiteMath.h>
#include <Image2d.h>

// Test runtime
#include <chrono>
#include <iostream>

#include "constants.h"
#include "scene.h"
#include "render.h"

using namespace LiteMath;
using namespace LiteImage;

int main() {
    const uint width = constants::width;
    const uint height = constants::height;
    Image2D<float4> CPUimage(width, height);

    // Init scene objects
    std::cout << "...Loading scene" << std::endl;
    scene::load("scene/objects.txt");

    /// CPU ///
    std::cout << "...Rendering" << std::endl;
    std::chrono::time_point<std::chrono::system_clock> start, end;
    std::chrono::duration<double> duration;

    start = std::chrono::system_clock::now();
    render::CPU(CPUimage, width, height);
    end = std::chrono::system_clock::now();
    duration = end - start;
    std::cout << "Render with CPU (1 thread):\t" << duration.count() << "s" << std::endl;

    // OpenMP
    start = std::chrono::system_clock::now();
    render::OMP(CPUimage, width, height);
    end = std::chrono::system_clock::now();
    duration = end - start;
    std::cout << "Render with OpenMP (4 threads):\t" << duration.count() << "s" << std::endl;

    // Save CPU image
    SaveImage("out_cpu.png", CPUimage, constants::gamma);

    /// GPU ///
    unsigned char *GPUimage = new unsigned char[width * height * 4];
    render::setup(width, height);

    // Push scene data to GPU
    std::chrono::time_point<std::chrono::system_clock> pushStart, pushEnd;
    std::chrono::duration<double> pushDuration;

    pushStart = std::chrono::system_clock::now();
    render::push();
    pushEnd = std::chrono::system_clock::now();
    pushDuration = pushEnd - pushStart;

    // Render with GPU
    start = std::chrono::system_clock::now();
    render::GPU(GPUimage, width, height);
    end = std::chrono::system_clock::now();
    duration = end - start;

    // Cleanup GPU
    render::destroy();

    std::cout << "Render with GPU:\t\t" << duration.count() << "s" << std::endl;
    std::cout << "Copy to GPU:\t\t\t" << pushDuration.count() << "s" << std::endl;

    duration += pushDuration;
    std::cout << "Render + Copy on GPU:\t\t" << duration.count() << "s" << std::endl;

    // Save GPU image
    stbi_flip_vertically_on_write(true);
    stbi_write_jpg("out_gpu.png", width, height,
            constants::stb::channels, GPUimage, constants::stb::quality);
    delete[] GPUimage;

    return 0;
}
