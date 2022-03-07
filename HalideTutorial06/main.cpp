#include "Halide.h"
#include <stdio.h>

using namespace Halide;

int main()
{
    Func gradient("gradient");
    Var x("x"), y("y");
    gradient(x, y) = x + y;
    gradient.trace_stores();

    printf("Evaluating gradient from (0,0) to (7,7)\n");
    Buffer<int> result(8, 8);
    gradient.realize(result);

    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {
            if (result(x, y) != x + y)
            {
                printf("something went wrong!\n");
                return -1;
            }
        }
    }

    Buffer<int> shifted(5, 7);
    shifted.set_min(100, 50);
    gradient.realize(shifted);

    for (int y = 50; y < 57; y++)
    {
        for (int x = 100; x < 105; x++)
        {
            if (shifted(x, y) != x + y)
            {
                printf("Something went wrong!\n");
                return -1;
            }
        }
    }
    printf("Success!\n");
    return 0;
}