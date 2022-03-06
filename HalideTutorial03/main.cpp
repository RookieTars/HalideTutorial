#include "Halide.h"
#include <stdio.h>

using namespace Halide;

int main()
{
    Func gradient("gradient");
    Var x("x"), y("y");
    Expr e = x + y;
    gradient(x, y) = e;
    Buffer<int> output = gradient.realize({8, 8});
    gradient.compile_to_lowered_stmt("gradient.html", {}, HTML);
    printf("Success!\n");
    return 0;
}