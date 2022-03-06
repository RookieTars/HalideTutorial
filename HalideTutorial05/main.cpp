#include "Halide.h"
#include <algorithm>
#include <stdio.h>
using namespace Halide;

int main()
{
    Var x("x"), y("y");

    {
        Func gradient("gradient");
        gradient(x, y) = x + y;
        gradient.trace_stores();
        printf("\nEvaluating gradient row-major\n");
        Buffer<int> output = gradient.realize({4, 4});
        printf("Pseudo-code for the schedule:\n");
        gradient.print_loop_nest();
        printf("\n");
    }
    {
        Func gradient("gradient_col_major");
        gradient(x, y) = x + y;
        gradient.trace_stores();
        gradient.reorder(y, x);
        printf("\nEvaluating gradient column-major\n");
        Buffer<int> output = gradient.realize({4, 4});
        printf("Pseudo-code for the schedule:\n");
        gradient.print_loop_nest();
        printf("\n");
    }
    {
        Func gradient("gradient_split");
        gradient(x, y) = x + y;
        gradient.trace_stores();

        Var x_outer, x_inner;
        gradient.split(x, x_outer, x_inner, 2);
        printf("\nEvaluating gradient with x split into x_outer and x_inner \n");
        Buffer<int> output = gradient.realize({4, 4});
        printf("Pseudo-code for the schedule:\n");
        gradient.print_loop_nest();
        printf("\n");
    }
    {
        Func gradient("gradient_fused");
        gradient(x, y) = x + y;
        gradient.trace_stores();

        Var fused("fused");
        gradient.fuse(x, y, fused);
        printf("\nEvaluating gradient with x and y fused\n");
        Buffer<int> output = gradient.realize({4, 4});
        printf("Pseudo-code for the schedule:\n");
        gradient.print_loop_nest();
        printf("\n");
    }
    {
        Func gradient("gradient_tiled");
        gradient(x, y) = x + y;
        gradient.trace_stores();
        Var x_outer("x_outer"), x_inner("x_inner"), y_outer("y_outer"), y_inner("y_inner");
        gradient.split(x, x_outer, x_inner, 4);
        gradient.split(y, y_outer, y_inner, 4);
        gradient.reorder(x_inner, y_inner, x_outer, y_outer);

        printf("\nEvaluating gradient in 4x4 tiles\n");
        Buffer<int> output = gradient.realize({8, 8});

        printf("Pseudo-code for the schedule:\n");
        gradient.print_loop_nest();
        printf("\n");
    }
    {
        Func gradient("gradient_in_vectors");
        gradient(x, y) = x + y;
        gradient.trace_stores();

        Var x_inner("x_inner"), x_outer("x_outer");
        gradient.split(x, x_outer, x_inner, 4);
        gradient.vectorize(x_inner);

        printf("\nEvaluating gradient with x_inner vectorized \n");
        Buffer<int> output = gradient.realize({8, 4});

        printf("Pseudo-code for the schedule:\n");
        gradient.print_loop_nest();
        printf("\n");
    }
    {
        Func gradient("gradient_unroll");
        gradient(x, y) = x + y;
        gradient.trace_stores();

        Var x_outer("x_outer"), x_inner("x_inner");
        gradient.split(x, x_outer, x_inner, 2);
        gradient.unroll(x_inner);
        printf("\nEvaluating gradient unrolled by a factor of two \n");
        Buffer<int> output = gradient.realize({4, 4});

        printf("Pseudo-code for the schedule:\n");
        gradient.print_loop_nest();
        printf("\n");
    }
    {
        Func gradient("gradient_split_7x2");
        gradient(x, y) = x + y;
        gradient.trace_stores();

        Var x_inner("x_inner"), x_outer("x_outer");
        gradient.split(x, x_outer, x_inner, 3);

        printf("\nEvaluating gradient over a 7x2 box with x split by 3 \n");
        Buffer<int> output = gradient.realize({7, 2});

        printf("\nPseudo-code for the schedule:\n");
        gradient.print_loop_nest();
        printf("\n");
    }
    {
        Func gradient("gradient_fused_tiles");
        gradient(x, y) = x + y;
        gradient.trace_stores();

        Var x_inner("x_inner"), x_outer("x_outer"), y_inner("y_inner"), y_outer("y_outer"), tile_index("tile_index");
        gradient.split(x, x_outer, x_inner, 8);
        gradient.split(y, y_outer, y_inner, 2);
        gradient.reorder(x_inner, y_inner, x_outer, y_outer);
        gradient.fuse(x_outer, y_outer, tile_index);
        gradient.parallel(tile_index);

        printf("\nEvaluating gradient tiles in parallel\n");
        Buffer<int> output = gradient.realize({16, 16});

        printf("\nPseudo-code for the schedule:\n");
        gradient.print_loop_nest();
        printf("\n");
    }
    {
        Func gradient_fast("gradient_fast");
        gradient_fast(x, y) = x + y;
        Var x_outer("x_outer"), x_inner("x_inner"), y_outer("y_outer"), y_inner("y_inner"),
            tile_index("tile_index");
        gradient_fast
            .tile(x, y, x_outer, y_outer, x_inner, y_inner, 64, 64)
            .fuse(x_outer, y_outer, tile_index)
            .parallel(tile_index);

        Var x_inner_outer("x_inner_outer"), y_inner_outer("y_inner_outer"), x_vectors("x_vectors"),
            y_pairs("y_pairs");
        gradient_fast
            .tile(x_inner, y_inner, x_inner_outer, y_inner_outer, x_vectors, y_pairs, 4, 2)
            .vectorize(x_vectors)
            .unroll(y_pairs);

        Buffer<int> output = gradient_fast.realize({350, 250});

        printf("Checking Halide result against equivalent C...\n");
        for (int tile_index = 0; tile_index < 6 * 4; tile_index++)
        {
            int y_outer = tile_index / 4;
            int x_outer = tile_index % 4;
            for (int y_inner_outer = 0; y_inner_outer < 64 / 2; y_inner_outer++)
            {
                for (int x_inner_outer = 0; x_inner_outer < 64 / 4; x_inner_outer++)
                {
                    // We're vectorized across x
                    int x = std::min(x_outer * 64, 350 - 64) + x_inner_outer * 4;
                    int x_vec[4] = {x + 0,
                                    x + 1,
                                    x + 2,
                                    x + 3};

                    // And we unrolled across y
                    int y_base = std::min(y_outer * 64, 250 - 64) + y_inner_outer * 2;
                    {
                        // y_pairs = 0
                        int y = y_base + 0;
                        int y_vec[4] = {y, y, y, y};
                        int val[4] = {x_vec[0] + y_vec[0],
                                      x_vec[1] + y_vec[1],
                                      x_vec[2] + y_vec[2],
                                      x_vec[3] + y_vec[3]};

                        // Check the result.
                        for (int i = 0; i < 4; i++)
                        {
                            if (output(x_vec[i], y_vec[i]) != val[i])
                            {
                                printf("There was an error at %d %d!\n",
                                       x_vec[i], y_vec[i]);
                                return -1;
                            }
                        }
                    }
                    {
                        // y_pairs = 1
                        int y = y_base + 1;
                        int y_vec[4] = {y, y, y, y};
                        int val[4] = {x_vec[0] + y_vec[0],
                                      x_vec[1] + y_vec[1],
                                      x_vec[2] + y_vec[2],
                                      x_vec[3] + y_vec[3]};

                        // Check the result.
                        for (int i = 0; i < 4; i++)
                        {
                            if (output(x_vec[i], y_vec[i]) != val[i])
                            {
                                printf("There was an error at %d %d!\n",
                                       x_vec[i], y_vec[i]);
                                return -1;
                            }
                        }
                    }
                }
            }
        }
        printf("\n");

        printf("\nPseudo-code for the schedule:\n");
        gradient_fast.print_loop_nest();
        printf("\n");
    }
    printf("Success!\n");
    return 0;
}