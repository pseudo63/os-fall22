#include "life.h"
#include <pthread.h>

struct thread_args {
    LifeBoard *state;
    LifeBoard *other_state;
    int start_row;
    int stop_row;
    int steps;
    int thread_id;
    pthread_barrier_t *barrier;
};

void *simulate_life_thread(void *void_args);

void simulate_life_parallel(int threads, LifeBoard &state, int steps) {

    pthread_barrier_t barrier;

    if (pthread_barrier_init(&barrier, NULL, threads)) {
        perror("Error initializing the barrier");
        return;
    }

    pthread_t *thread_array = new pthread_t[threads];
    struct thread_args *thread_args_array = new struct thread_args[threads];
    LifeBoard other_state{state.width(), state.height()};

    for (int i = 0; i < threads; i++) {
        thread_args_array[i].state = &state; 
        thread_args_array[i].other_state = &other_state;
        thread_args_array[i].barrier = &barrier;
        thread_args_array[i].steps = steps;
        thread_args_array[i].thread_id = i;

        // start is inclusive, stop is exclusive
        thread_args_array[i].start_row = (((state.height() - 2) * i) / threads) + 1;
        thread_args_array[i].stop_row =  (((state.height() - 2) * (i + 1)) / threads) + 1;

        pthread_create(&thread_array[i], NULL, simulate_life_thread, &thread_args_array[i]);
    }

    for (int i = 0; i < threads; i++) {
        pthread_join(thread_array[i], NULL);
    }

    pthread_barrier_destroy(&barrier);

    delete[] thread_array;
    delete[] thread_args_array;
}

void *simulate_life_thread(void *void_args) {
    struct thread_args *args = (struct thread_args*) void_args;
    for (int step = 0; step < args->steps; ++step) {
        /* We use the range [1, width - 1) here instead of
         * [0, width) because we fix the edges to be all 0s.
         */
        for (int y = args->start_row; y < args->stop_row; ++y) {
            for (int x = 1; x < args->state->width() - 1; ++x) {
                int live_in_window = 0;
                /* For each cell, examine a 3x3 "window" of cells around it,
                 * and count the number of live (true) cells in the window. */
                for (int y_offset = -1; y_offset <= 1; ++y_offset) {
                    for (int x_offset = -1; x_offset <= 1; ++x_offset) {
                        if (args->state->at(x + x_offset, y + y_offset)) {
                            ++live_in_window;
                        }
                    }
                }
                /* Cells with 3 live neighbors remain or become live.
                   Live cells with 2 live neighbors remain live. */
                args->other_state->at(x, y) = (live_in_window == 3  || (live_in_window == 4 && args->state->at(x, y)));
                /* dead cell with 3 neighbors or live cell with 2 */ /* live cell with 3 neighbors */
            }
        }
        pthread_barrier_wait(args->barrier);

        if (args->thread_id == 0) {
            swap(*args->state, *args->other_state);
        }

        pthread_barrier_wait(args->barrier);

    }
    return 0;
}
