#!/bin/bash

# 检查参数数量
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 STEPS THREADS"
    exit 1
fi

STEPS=$1
THREADS=$2
INPUT_DIR="input"

# 创建输出目录
mkdir -p output

# 遍历 input 目录中的所有文件
for FILE in "$INPUT_DIR"/*; do
    BASENAME=$(basename "$FILE")
    # 运行并行版本并输出到文件
    ./life $STEPS "$FILE" $THREADS > "output/parallel_$BASENAME.txt"

    # 运行串行版本并输出到文件
    ./life-serial $STEPS "$FILE" > "output/serial_$BASENAME.txt"

    # 比较两个输出文件
    DIFF=$(diff "output/parallel_$BASENAME.txt" "output/serial_$BASENAME.txt")
    if [ "$DIFF" != "" ]; then
        echo "Outputs for $BASENAME are different."
        echo "Difference:"
        echo "$DIFF"
    else
        echo "Outputs for $BASENAME are identical."
    fi
done

echo "Comparison complete."

