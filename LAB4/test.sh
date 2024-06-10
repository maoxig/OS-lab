#!/bin/bash

# 定义一个变量来指定路径
mount_point="/mnt/fat32"

# 编译并运行 main 程序
# 编译源文件并生成可执行文件
gcc -o main fat32.c main.c

# 检查 main 程序是否成功执行
if [ $? -eq 0 ]; then
    echo "Compilation successful."
    
    # 随机选择一个文件
    random_file=$(find "$mount_point" -type f | shuf -n 1| cut -c $((${#mount_point}+1))-)
    echo "Randomly selected file:$random_file"

    # 随机生成 count 和 offset
    count=$(shuf -i 1-1024 -n 1)
    offset=$(shuf -i 0-8192 -n 1)
    echo "Randomly generated count: $count, offset:$offset"

    # 随机选择一个目录
    random_dir=$(find "$mount_point" -type d | shuf -n 1| cut -c $((${#mount_point}+1))-)
    echo "Randomly selected directory: $random_dir"

    # 创建一个临时文件来存储命令序列
    commands_file="commands.txt"

    echo "open $random_file" >$commands_file
    # echo "pread $mount_point/test_file.txt 10 0" >>$commands_file
    echo "read $random_file $count $offset" >>$commands_file
    echo "readdir $random_dir" >>$commands_file
    echo "exit" >> $commands_file

    # 运行 main 程序并传递命令文件路径
    main_output=$(cat $commands_file | ./main )
    echo "Main program output for commands:"
    echo "$main_output"



    
else
    echo "Compilation failed."
fi
