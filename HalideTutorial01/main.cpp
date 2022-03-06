#include "Halide.h"

#include <stdio.h>
#include <vector>

int main()
{
    Halide::Func gradient;
    Halide::Var x, y;
    Halide::Expr e = x + y;
    gradient(x, y) = e;
    std::vector<int32_t> sizes = {800, 600};
    Halide::Buffer<int32_t> output = gradient.realize({800, 600});
    for (int i = 0; i < output.height(); i++)
    {
        for (int j = 0; j < output.width(); j++)
        {
            if (output(j, i) != i + j)
            {
                printf("Something went wrong!\n"
                       "Pixel %d, %d was supposed to be %d, but instead it's %d\n",
                       j, i, i + j, output(j, i));
                return -1;
            }
        }
    }
    printf("Success!\n");

    return 0;
}