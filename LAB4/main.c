#include <stdio.h>
#include <stdlib.h>
#include "fat32.h"
#include <string.h>
#include <assert.h>
int main(){
    int mounted = fat_mount("fat32.img");
    assert(mounted == 0);
    int fd = fat_open("/exam_9.txt");
    assert(fd != -1);
    char *buffer = (char *)malloc(128 * sizeof(char));
    int Read_Size = fat_pread(fd, buffer, 3, 0);
    assert(Read_Size == 3);
    assert(strncmp(buffer, "Sec",3) == 0);
    return 0;
}

