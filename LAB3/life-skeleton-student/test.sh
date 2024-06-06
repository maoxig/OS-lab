#!/bin/bash

# 检查参数数量
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 STEPS THREADS"
    exit 1
fi

STEPS=$1
THREADS=$2
INPUT_DIR="input"
OUTPUT_DIR="output"

# 创建输出目录
mkdir -p "$OUTPUT_DIR"

# 遍历 input 目录中的所有文件
for FILE in "$INPUT_DIR"/*; do
    BASENAME=$(basename "$FILE")
    # 运行串行版本并测量时间
    time_serial=$( { time ./life-serial $STEPS "$FILE" > "$OUTPUT_DIR/serial_$BASENAME.txt" 2>&1; } 2>&1 )
    serial_time=$(echo "$time_serial" | grep real | awk '{print $2}')

    # 运行并行版本并测量时间
    time_parallel=$( { time ./life $STEPS "$FILE" $THREADS > "$OUTPUT_DIR/parallel_$BASENAME.txt" 2>&1; } 2>&1 )
    parallel_time=$(echo "$time_parallel" | grep real | awk '{print $2}')

    # 比较两个输出文件
    DIFF=$(diff "$OUTPUT_DIR/parallel_$BASENAME.txt" "$OUTPUT_DIR/serial_$BASENAME.txt")
    if [ "$DIFF" != "" ]; then
        echo "Outputs for $BASENAME are different."
        echo "Difference:"
        #echo "$DIFF"
    else
        echo "Outputs for $BASENAME are the same."
        # 计算加速百分比
        serial_seconds=$(echo $serial_time | awk -F'm' '{print $1 * 60 +$2}')
        parallel_seconds=$(echo $parallel_time | awk -F'm' '{print $1 * 60 +$2}')
        speedup=$(echo "$serial_seconds / $parallel_seconds" | bc -l)
        acceleration_percentage=$(echo "($speedup) * 100" | bc -l)
        echo "Parallel version is ${acceleration_percentage}% as serial version."
    fi
done

echo "Comparison complete."

