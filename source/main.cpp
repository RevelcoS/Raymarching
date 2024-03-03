#include <LiteMath.h>
#include <Image2d.h>
#include <iostream>

using namespace LiteMath;
using namespace LiteImage;

int main() {
    float2 vector(3.0f, 4.0f);
    std::cout << length(vector) << std::endl;
    std::cout << "Raymarching!" << std::endl;

    auto image = LoadImage<float4>("images/door.png");
    uint width = image.width(),
         height = image.height();

    // Draw red cross
    for (uint i = 0; i < min(width, height); i++) {
        auto coord = int2(i);
        image[coord] = float4(1.0f, 0.0f, 0.0f, 1.0f);
    }

    for (uint i = 0; i < min(width, height); i++) {
        auto coord = int2(width - i - 1, i);
        image[coord] = float4(0.0f, 1.0f, 0.0f, 1.0f);
    }

    SaveImage("images/door_modified.png", image);

    return 0;
}
