#include "Halide.h"

#include "halide_image_io.h"

using namespace Halide::Tools;

int main()
{
    Halide::Buffer<uint8_t> input = load_image("../images/rgb.png");
    Halide::Func brighter;
    Halide::Var x, y, c;
    Halide::Expr value = input(x, y, c);
    value = Halide::cast<float>(value);
    value = value * 1.5f;
    value = Halide::min(value, 255.0f);
    value = Halide::cast<uint8_t>(value);
    brighter(x, y, c) = value;
    Halide::Buffer<uint8_t> output = brighter.realize({input.width(), input.height(), input.channels()});
    save_image(output, "brighter.png");
    printf("Success!\n");
    return 0;
}
