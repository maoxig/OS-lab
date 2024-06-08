#!/bin/bash

# 定义一个变量来指定路径
mount_point="/mnt/fat32"

# 编译并运行 main 程序
# 编译源文件并生成可执行文件
gcc -o main fat32.c main.c

# 如果编译成功，运行可执行文件
if [ $? -eq 0 ]; then
    echo "Compilation successful."
else
    echo "Compilation failed."
fi

# 检查 main 程序是否成功执行
if [ $? -eq 0 ]; then
    echo "Execution successful."
    
    # 获取 main 程序的输出
    main_output=$(./main "$@")
    echo "Main program output:"
    echo "$main_output"

    # 随机选择一个文件
    random_file=$(find "$mount_point" -type f | shuf -n 1)
    echo "Randomly selected file: $random_file"

    # 随机生成 count 和 offset
    count=$(shuf -i 1-1024 -n 1)
    offset=$(shuf -i 0-1024 -n 1)
    echo "Randomly generated count: $count, offset:$offset"

    # 使用 dd 工具读取文件内容
    dd_output=$(dd if="$random_file" bs=1 skip=$offset count=$count 2>/dev/null)
    echo "dd tool output:"
    echo "$dd_output"

else
    echo "Execution failed."
fi
