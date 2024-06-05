#include "life.h"
#include <pthread.h>
#include <semaphore.h>
//#include <assert.h>

LifeBoard *State;
LifeBoard *Next_state;
int Threads;
int Steps;
typedef struct myargs_t {
    int start;
    int end;
    sem_t *start_;
    sem_t *end_;
} myargs_t; 

void *life_paralle_th(void *args)
{
    myargs_t *args_ = (myargs_t *)args;
    int width = State->width - 2;
    int y_d = args_->start/width + 1;
    int y_h = args_->end/width + 1;
    int x_d = args_->start -(y_d-1)*width+1;
    int x_r = args_->end - (y_h-1)*width+1;
    for(int step = 0; step < Steps; step++)
    {
        sem_wait(args_->start_);
        for(int y = y_d; y <= y_h; y++)
        {
            for(int x = 1; x <= width; x++)
            {
                if(y == y_d && x<x_d) continue;
                if(y == y_h && x>x_r) break;
                int pos = y*State->width + x;
                //int live_neighbors = count_live_neighbors(State, x, y);
                int live_neighbors = 0;
                live_neighbors += State->cells[pos-State->width-1];
                live_neighbors += State->cells[pos-State->width];
                live_neighbors += State->cells[pos-State->width+1];
                live_neighbors += State->cells[pos-1];
                live_neighbors += State->cells[pos];
                live_neighbors += State->cells[pos+1];
                live_neighbors += State->cells[pos+State->width-1];
                live_neighbors += State->cells[pos+State->width];
                live_neighbors += State->cells[pos+State->width+1];
                //printf("%d\n",live_neighbors);
                //LifeCell current_cell = at(State, x, y);
                LifeCell new_cell = (live_neighbors == 3) || (live_neighbors == 4 && State->cells[pos] == 1) ? 1 : 0;
                set_at(Next_state, x, y, new_cell);
                Next_state->cells[pos] = new_cell;
            }
        }
        sem_post(args_->end_);
    }
    return NULL;
}


void simulate_life_parallel(int threads, LifeBoard *state, int steps) {
    // TODO
    if (steps == 0) return;
    Next_state = create_life_board(state->width, state->height);
    if (Next_state == NULL) {
        fprintf(stderr, "Failed to allocate memory for next state.\n");
        return;
    }
    //printf("%d %d\n",Next_state->width,Next_state->height);
    State = state;
    Threads = threads;
    Steps = steps;
    pthread_t thread[threads];
    myargs_t args[threads];
    sem_t start[threads];
    sem_t end[threads];
    int total_size = (State->height - 2)*(State->width - 2);
    int each_size = total_size/threads;
    for(int i=0;i<threads;i++)
    {
        sem_init(&start[i], 0, 0);
        sem_init(&end[i], 0, 0);
        if (i != threads - 1)
        {
            args[i].start = i*each_size;
            args[i].end = args[i].start + each_size - 1;
            args[i].start_ = &start[i];
            args[i].end_ = &end[i];
        }
        else
        {
            args[i].start = i*each_size;
            args[i].end = total_size - 1;
            args[i].start_ = &start[i];
            args[i].end_ = &end[i];
        }
        pthread_create(&thread[i], NULL, life_paralle_th, &args[i]);
    }
    for(int step = 0; step < steps; step++)
    {
        for(int i = 0; i < threads; i++)
        {
            sem_post(&start[i]);
        }
        for(int i = 0; i < threads; i++)
        {
            sem_wait(&end[i]);
        }
        //swap(State, Next_state);
        LifeCell *temp = State->cells;
        State->cells = Next_state->cells;
        Next_state->cells = temp;
    }
    for (int i = 0; i < threads; i++)
    {
        pthread_join(thread[i], NULL);
    }
    destroy_life_board(Next_state);
}