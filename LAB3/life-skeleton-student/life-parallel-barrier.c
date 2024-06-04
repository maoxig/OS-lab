#include "life.h"
#include <pthread.h>
#include <semaphore.h>

// 全局变量
pthread_barrier_t barrier; // 屏障
int total_num_threads;

// 线程结构体，用于传递参数
typedef struct
{
    LifeBoard *state;
    LifeBoard *next_state;
    int thread_id;
    int steps;

} ThreadArgs;

void *simulate_life_thread(void *args)
{
    ThreadArgs *thread_args = (ThreadArgs *)args;
    LifeBoard *state = thread_args->state;
    LifeBoard *next_state = thread_args->next_state;
    int thread_id = thread_args->thread_id;
    int steps = thread_args->steps;
    int width = state->width;
    int height = state->height;
    int chunk_size = (height - 2) / total_num_threads;

    int start_row = thread_id * chunk_size;
    int end_row = (thread_id == total_num_threads - 1) ? height - 2 : (thread_id + 1) * chunk_size;

    for (int step = 0; step < steps; step++)
    {
        for (int y = start_row + 1; y < end_row + 1; y++)
        {
            for (int x = 1; x < width - 1; x++)
            {
                int live_neighbors = count_live_neighbors(state, x, y);
                LifeCell current_cell = at(state, x, y);
                LifeCell new_cell = (live_neighbors == 3) || (live_neighbors == 4 && current_cell == 1) ? 1 : 0;
                set_at(next_state, x, y, new_cell);
            }
        }

        // 使用屏障来同步线程
        pthread_barrier_wait(&barrier);

        // 所有线程都已经计算完毕，交换当前状态和下一状态
        if (thread_id == 0)
        {
            swap(state, next_state);
        }

        // 等待所有线程完成交换
        pthread_barrier_wait(&barrier);
    }

    pthread_exit(NULL);
}

void simulate_life_parallel(int num_threads, LifeBoard *state, int steps)
{
    if (steps == 0)
        return;

    LifeBoard *next_state = create_life_board(state->width, state->height);
    if (next_state == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for next state.\n");
        return;
    }

    pthread_t threads[num_threads];
    ThreadArgs thread_args[num_threads];

    // 初始化屏障
    pthread_barrier_init(&barrier, NULL, num_threads);
    total_num_threads = num_threads;

    // 创建并启动线程
    for (int i = 0; i < num_threads; i++)
    {
        thread_args[i].state = state;
        thread_args[i].next_state = next_state;
        thread_args[i].thread_id = i;
        thread_args[i].steps = steps;
        pthread_create(&threads[i], NULL, simulate_life_thread, &thread_args[i]);
    }

    // 等待所有线程完成
    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // 清理资源
    pthread_barrier_destroy(&barrier);
    destroy_life_board(next_state);
}
