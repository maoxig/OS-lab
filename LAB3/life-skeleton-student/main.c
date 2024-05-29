#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "life.h"

int main(int argc, char** argv) {
    int steps = 0;
    char input_file[256];
    int threads =1;
    if (argc == 4) {
        steps = atoi(argv[1]);
        strcpy(input_file, argv[2]);
        threads =atoi(argv[3]);
    }
    if (argc != 4) {
        printf("Usage:\n"
               "  %s STEPS FILENAME\n"
               "    Run serial computation and print out result\n", argv[0]);
        return 1;
    }
    LifeBoard* board = malloc(sizeof(LifeBoard));
    FILE* in = fopen(input_file, "r");
    read_life_board(in, board);
    simulate_life_parallel(threads,board, steps);
    print_life_board(board);
    destroy_life_board(board);
    return 0;
}