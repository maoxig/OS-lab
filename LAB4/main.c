#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fat32.h"

#define MAX_COMMAND_LENGTH 100

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <image_path>\n", argv[0]);
        return 1;
    }

    const char *image_path = argv[1];
    int mounted = fat_mount(image_path);
    if (mounted != 0) {
        printf("Error: unable to mount image %s\n", image_path);
        return 1;
    }

    char command[MAX_COMMAND_LENGTH];
    while (1) {
        printf("Enter command (type 'exit' to quit): ");
        fgets(command, MAX_COMMAND_LENGTH, stdin);
        if (strcmp(command, "exit\n") == 0) {
            break;
        }

        char *token = strtok(command, " ");
        if (!token) {
            printf("Error: invalid command\n");
            continue;
        }

        const char *command_str = token;
        if (strcmp(command_str, "open") == 0) {
            if (strtok(NULL, " ")) {
                const char *path = strtok(NULL, " ");
                int fd = fat_open(path);
                printf("File descriptor for %s: %d\n", path, fd);
            } else {
                printf("Error: missing path argument for 'open'\n");
            }
        } else if (strcmp(command_str, "read") == 0) {
            if (strtok(NULL, " ") && strtok(NULL, " ") && strtok(NULL, " ")) {
                const char *path = strtok(NULL, " ");
                int count = atoi(strtok(NULL, " "));
                int offset = atoi(strtok(NULL, " "));
                int fd = fat_open(path);
                if (fd != -1) {
                    char *buffer = (char *)malloc(count + 1);
                    int read_size = fat_pread(fd, buffer, count, offset);
                    buffer[read_size] = '\0'; // Ensure null-terminated string
                    printf("Read %d bytes from file %s: %s\n", read_size, path, buffer);
                    free(buffer);
                    fat_close(fd);
                } else {
                    printf("Error: unable to open file %s\n", path);
                }
            } else {
                printf("Error: missing arguments for 'read'\n");
            }
        } else if (strcmp(command_str, "readdir") == 0) {
            if (strtok(NULL, " ")) {
                const char *path = strtok(NULL, " ");
                struct FilesInfo *files = fat_readdir(path);
                if (files) {
                    printf("Files in directory %s:\n", path);
                    for (int i = 0; i < files->size; ++i) {
                        printf("Name: %s, Size: %u\n", files->files[i].DIR_Name, files->files[i].DIR_FileSize);
                    }
                    free(files->files);
                    free(files);
                } else {
                    printf("Error: unable to read directory %s\n", path);
                }
            } else {
                printf("Error: missing path argument for 'readdir'\n");
            }
        } else {
            printf("Error: unknown command %s\n", command_str);
        }
    }

    fat_mount(NULL); // Unmount the image
    return 0;
}
