#include <stdio.h>
#include <stdlib.h>
#include "fat32.h"
#include <string.h>
#include <assert.h>
int main(){
    int mounted = fat_mount("fat32.img");
    assert(mounted == 0);
    int fd = fat_open("/exam_1.txt");
    assert(fd != -1);
    int read_num =15;
    char *buffer = (char *)malloc(128 * sizeof(char));
    int Read_Size = fat_pread(fd, buffer, read_num, 893);
    printf("Read_size:%d\n",Read_Size);
    assert(Read_Size == read_num);
    printf("buf:%s\n",buffer);
    assert(strncmp(buffer, "OTE: The type W",read_num) == 0);
    return 0;
}

