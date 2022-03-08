#include "Halide.h"
#include <stdio.h>

using namespace Halide;

int main()
{
    Var x("x"), y("y");
    {
        Func producer("producer_default"), consumer("consumer_default");

        producer(x, y) = sin(x * y);

        consumer(x, y) = (producer(x, y) +
                          producer(x, y + 1) +
                          producer(x + 1, y) +
                          producer(x + 1, y + 1)) /
                         4;

        consumer.trace_stores();
        producer.trace_stores();

        printf("\nEvaluating producer-consumer pipeline with default schedule\n");
        consumer.realize({4, 4});

        printf("Pseudo-code for the schedule:\n");
        consumer.print_loop_nest();
        printf("\n");
    }
    {
        Func producer("producer_root"), consumer("consumer_root");
        producer(x, y) = sin(x * y);

        consumer(x, y) = (producer(x, y) +
                          producer(x, y + 1) +
                          producer(x + 1, y) +
                          producer(x + 1, y + 1)) /
                         4;

        producer.compute_root();

        consumer.trace_stores();
        producer.trace_stores();

        printf("\nEvaluating producer.compute_root()\n");
        consumer.realize({4, 4});

        printf("Pseudo-code for the schedule:\n");
        consumer.print_loop_nest();
        printf("\n");
    }
    {
        Func producer("producer_y"), consumer("consumer_y");
        producer(x, y) = sin(x * y);
        consumer(x, y) = (producer(x, y) +
                          producer(x, y + 1) +
                          producer(x + 1, y) +
                          producer(x + 1, y + 1)) /
                         4;

        producer.compute_at(consumer, y);

        producer.trace_stores();
        consumer.trace_stores();

        printf("\nEvaluating producer.compute_at(consumer,y)\n");
        consumer.realize({4, 4});

        printf("Pseudo-code for the schedule:\n");
        consumer.print_loop_nest();
        printf("\n");
    }
    {
        Func producer("producer_root_y"), consumer("consumer_root_y");
        producer(x, y) = sin(x * y);
        consumer(x, y) = (producer(x, y) +
                          producer(x, y + 1) +
                          producer(x + 1, y) +
                          producer(x + 1, y + 1)) /
                         4;

        producer.store_root();
        producer.compute_at(consumer, y);

        producer.trace_stores();
        consumer.trace_stores();

        printf("\nEvaluating producer.store_root().compute_at(consumer,y)\n");
        consumer.realize({4, 4});

        printf("Pseudo-code for the schedule:\n");
        consumer.print_loop_nest();
        printf("\n");
    }
    {
        Func producer("producer_root_x"), consumer("consumer_root_x");

        producer(x, y) = sin(x * y);
        consumer(x, y) = (producer(x, y) +
                          producer(x, y + 1) +
                          producer(x + 1, y) +
                          producer(x + 1, y + 1)) /
                         4;

        producer.store_root().compute_at(consumer, x);

        producer.trace_stores();
        consumer.trace_stores();

        printf("\nEvaluating producer.store().compute_at(consumer,x)\n");
        consumer.realize({4, 4});
    }
    {
        Func producer("producer_tile"), consumer("consumer_tile");
        producer(x, y) = sin(x * y);
        consumer(x, y) = (producer(x, y) +
                          producer(x, y + 1) +
                          producer(x + 1, y) +
                          producer(x + 1, y + 1)) /
                         4;

        Var x_outer, y_outer, x_inner, y_inner;
        consumer.tile(x, y, x_outer, y_outer, x_inner, y_inner, 4, 4);
        producer.compute_at(consumer, x_outer);

        consumer.trace_stores();
        producer.trace_stores();

        printf("\nEvaluating:\n"
               "consumer.tile(x,y,x_outer,y_outer,x_inner,y_inner,4,4);\n"
               "producer.compute_at(consumer,x_outer);\n");
        consumer.realize({8, 8});

        printf("Pseudo-code for the schedule:\n");
        consumer.print_loop_nest();
        printf("\n");
    }
    {
        Func producer("producer_mixed"), consumer("consumer_mixed");
        producer(x, y) = sin(x * y);
        consumer(x, y) = (producer(x, y) +
                          producer(x, y + 1) +
                          producer(x + 1, y) +
                          producer(x + 1, y + 1)) /
                         4;

        Var yo, yi;
        consumer.split(y, yo, yi, 16);
        consumer.parallel(yo);
        consumer.vectorize(x, 4);

        producer.store_at(consumer, yo);
        producer.compute_at(consumer, yi);
        producer.vectorize(x, 4);
        Buffer<float> halide_result = consumer.realize({160, 160});

        float c_result[160][160];

        // For every strip of 16 scanlines (this loop is parallel in
        // the Halide version)
        for (int yo = 0; yo < 160 / 16; yo++)
        {
            int y_base = yo * 16;

            // Allocate a two-scanline circular buffer for the producer
            float producer_storage[2][161];

            // For every scanline in the strip of 16:
            for (int yi = 0; yi < 16; yi++)
            {
                int y = y_base + yi;

                for (int py = y; py < y + 2; py++)
                {
                    // Skip scanlines already computed *within this task*
                    if (yi > 0 && py == y)
                        continue;

                    // Compute this scanline of the producer in 4-wide vectors
                    for (int x_vec = 0; x_vec < 160 / 4 + 1; x_vec++)
                    {
                        int x_base = x_vec * 4;
                        // 4 doesn't divide 161, so push the last vector left
                        // (see lesson 05).
                        if (x_base > 161 - 4)
                            x_base = 161 - 4;
                        // If you're on x86, Halide generates SSE code for this part:
                        int x[] = {x_base, x_base + 1, x_base + 2, x_base + 3};
                        float vec[4] = {sinf(x[0] * py), sinf(x[1] * py),
                                        sinf(x[2] * py), sinf(x[3] * py)};
                        producer_storage[py & 1][x[0]] = vec[0];
                        producer_storage[py & 1][x[1]] = vec[1];
                        producer_storage[py & 1][x[2]] = vec[2];
                        producer_storage[py & 1][x[3]] = vec[3];
                    }
                }

                // Now compute consumer for this scanline:
                for (int x_vec = 0; x_vec < 160 / 4; x_vec++)
                {
                    int x_base = x_vec * 4;
                    // Again, Halide's equivalent here uses SSE.
                    int x[] = {x_base, x_base + 1, x_base + 2, x_base + 3};
                    float vec[] = {
                        (producer_storage[y & 1][x[0]] +
                         producer_storage[(y + 1) & 1][x[0]] +
                         producer_storage[y & 1][x[0] + 1] +
                         producer_storage[(y + 1) & 1][x[0] + 1]) /
                            4,
                        (producer_storage[y & 1][x[1]] +
                         producer_storage[(y + 1) & 1][x[1]] +
                         producer_storage[y & 1][x[1] + 1] +
                         producer_storage[(y + 1) & 1][x[1] + 1]) /
                            4,
                        (producer_storage[y & 1][x[2]] +
                         producer_storage[(y + 1) & 1][x[2]] +
                         producer_storage[y & 1][x[2] + 1] +
                         producer_storage[(y + 1) & 1][x[2] + 1]) /
                            4,
                        (producer_storage[y & 1][x[3]] +
                         producer_storage[(y + 1) & 1][x[3]] +
                         producer_storage[y & 1][x[3] + 1] +
                         producer_storage[(y + 1) & 1][x[3] + 1]) /
                            4};

                    c_result[y][x[0]] = vec[0];
                    c_result[y][x[1]] = vec[1];
                    c_result[y][x[2]] = vec[2];
                    c_result[y][x[3]] = vec[3];
                }
            }
        }
        printf("Pseudo-code for the schedule:\n");
        consumer.print_loop_nest();
        printf("\n");

        // Look on my code, ye mighty, and despair!

        // Let's check the C result against the Halide result. Doing
        // this I found several bugs in my C implementation, which
        // should tell you something.
        for (int y = 0; y < 160; y++)
        {
            for (int x = 0; x < 160; x++)
            {
                float error = halide_result(x, y) - c_result[y][x];
                // It's floating-point math, so we'll allow some slop:
                if (error < -0.001f || error > 0.001f)
                {
                    printf("halide_result(%d, %d) = %f instead of %f\n",
                           x, y, halide_result(x, y), c_result[y][x]);
                    return -1;
                }
            }
        }
    }
    // This stuff is hard. We ended up in a three-way trade-off
    // between memory bandwidth, redundant work, and
    // parallelism. Halide can't make the correct choice for you
    // automatically (sorry). Instead it tries to make it easier for
    // you to explore various options, without messing up your
    // program. In fact, Halide promises that scheduling calls like
    // compute_root won't change the meaning of your algorithm -- you
    // should get the same bits back no matter how you schedule
    // things.

    // So be empirical! Experiment with various schedules and keep a
    // log of performance. Form hypotheses and then try to prove
    // yourself wrong. Don't assume that you just need to vectorize
    // your code by a factor of four and run it on eight cores and
    // you'll get 32x faster. This almost never works. Modern systems
    // are complex enough that you can't predict performance reliably
    // without running your code.

    // We suggest you start by scheduling all of your non-trivial
    // stages compute_root, and then work from the end of the pipeline
    // upwards, inlining, parallelizing, and vectorizing each stage in
    // turn until you reach the top.

    // Halide is not just about vectorizing and parallelizing your
    // code. That's not enough to get you very far. Halide is about
    // giving you tools that help you quickly explore different
    // trade-offs between locality, redundant work, and parallelism,
    // without messing up the actual result you're trying to compute.
    printf("Success!\n");
    return 0;
}