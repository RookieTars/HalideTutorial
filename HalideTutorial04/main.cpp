#include "Halide.h"
#include <stdio.h>
using namespace Halide;

int main()
{
    Var x("x"), y("y");
    {
        Func gradient("gradient");
        gradient(x, y) = x + y;
        gradient.trace_stores();
        printf("Evaluating gradient\n");
        Buffer<int> output = gradient.realize({8, 8});

        Func parallel_gradient("parallel_gradient");
        parallel_gradient(x, y) = x + y;
        parallel_gradient.trace_stores();
        gradient.parallel(y);
        printf("\nEvaluating parallel_gradient\n");
        parallel_gradient.realize({8, 8});
    }
    {
        Func f;
        f(x, y) = sin(x) + cos(y);

        Func g;
        g(x, y) = sin(x) + print(cos(y));

        printf("\nEvaluating sin(x) + cos(y), and just printing cos(y)\n");
        g.realize({4, 4});
    }
    {
        Func f;
        Expr e = cos(y);
        e = print_when(x == 37 && y == 42, e, "<- this is cos(y) at x,y == (37,42)");
        f(x, y) = sin(x) + e;
        printf("\nEvaluating sin(x) + cos(y), and printing cos(y) at a single pixel\n");
        f.realize({640, 480});

        Func g;
        e = cos(y);
        e = print_when(e < 0, e, "cos(y) < 0 at y == ", y);
        g(x, y) = sin(x) + e;
        printf("\nEvaluating sin(x) + cos(y), and printing whenever cos(y) < 0\n");
        g.realize({4, 4});
    }
    {
        Var fizz("fizz"), buzz("buzz");
        Expr e = 1;
        for (int i = 2; i < 100; i++)
        {
            if (i % 3 == 0 && i % 5 == 0)
            {
                e += fizz * buzz;
            }
            else if (i % 3 == 0)
            {
                e += fizz;
            }
            else if (i % 5 == 0)
            {
                e += buzz;
            }
            else
            {
                e += i;
            }
        }
        std::cout << "Printing a complex Expr: " << e << "\n";
    }
    printf("Success!\n");
    return 0;
}