#include "life.h"
#include <pthread.h>
#include <semaphore.h>

// 全局变量
pthread_mutex_t mutex; // 互斥锁
pthread_cond_t cond;   // 条件变量
int threads_done = 0;      // 记录已完成任务的线程数
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
        for (int y = start_row + 1; y < end_row+1; y++)
        {
            for (int x = 1; x < width - 1; x++)
            {

                // int live_neighbors = 0;
                // for (int dy = -1; dy <= 1; ++dy) {
                //     for (int dx = -1; dx <= 1; ++dx) {
                //         int nx = x + dx, ny = y + dy;
                //         if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                //             live_neighbors += state->cells[ny * width + nx];
                //         }
                //     }
                // }
                int live_neighbors = count_live_neighbors(state, x, y);
                LifeCell current_cell = state->cells[y * state->width + x];//LifeCell current_cell = at(state, x, y);
                LifeCell new_cell = (live_neighbors == 3) || (live_neighbors == 4 && current_cell == 1) ? 1 : 0;
                next_state->cells[y * next_state->width + x] = new_cell; 
                //set_at(next_state, x, y, new_cell);
            }
        }

        // 使用互斥锁和条件变量来同步线程
        pthread_mutex_lock(&mutex);
        threads_done++;

        // 检查是否所有线程都完成了这一轮迭代
        if (threads_done == total_num_threads)
        {
            threads_done = 0; // 重置计数器
            swap(state, next_state); // 交换当前状态和下一状态
            // LifeCell *temp = state->cells;
            // state->cells = next_state->cells;
            // next_state->cells = temp;
            pthread_cond_broadcast(&cond); // 唤醒所有等待的线程
            
        }
        else
        {

            pthread_cond_wait(&cond, &mutex);
            
        }
        
        pthread_mutex_unlock(&mutex);
        
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

    // 初始化互斥锁和条件变量
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    threads_done = 0;
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
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    destroy_life_board(next_state);
}