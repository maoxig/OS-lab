#!/bin/bash

# 编译源文件并生成可执行文件
gcc -o main fat32.c main.c

# 如果编译成功，运行可执行文件
if [ $? -eq 0 ]; then
    echo "Compilation successful."
else
    echo "Compilation failed."
fi
