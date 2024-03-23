#include <LiteMath.h>
#include <Image2d.h>

using namespace LiteMath;
using namespace LiteImage;

namespace render {
    void CPU(Image2D<float4> &image, const uint width, const uint height);
    void OMP(Image2D<float4> &image, const uint width, const uint height);
    void GPU(unsigned char   *image, const uint width, const uint height);

    /// GPU ///
    void push(void);
    void setup(const uint width, const uint height);
    void destroy(void);
}
