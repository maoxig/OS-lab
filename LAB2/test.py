import json
import time
from dma import DMA


def execute_workload(workload, times: int):
    heap_size = workload["size"]
    requests = workload["requests"]
    expected_results = workload["result"]
    expected_assertions = workload["assert"]

    # 初始化内存分配器

    start_time = time.time()
    for _ in range(times):
        dma = DMA(heap_size)
        for i, request in enumerate(requests):
            req_type = request["type"]
            req_id = request["id"]

            if req_type == "malloc":
                req_size = request["size"]
                req_value = request["value"]

                result = dma.malloc(req_id, req_size, req_value)
                assert result == expected_assertions[i]

            elif req_type == "free":
                result = dma.free(req_id)
                assert result == expected_assertions[i]
    end_time = time.time()

    execution_time = end_time - start_time
    # 检查heap中的分配结果
    allocations = dma.data()
    for expected_result in expected_results:
        req_id = expected_result["id"]
        req_size = expected_result["size"]
        req_value = expected_result["value"]

        assert req_id in allocations
        allocation = allocations[req_id]
        assert allocation["size"] == req_size
        assert allocation["value"] == req_value
    if execution_time > 0:
        efficiency = len(requests) / execution_time
    else:
        efficiency = float("inf")  # 表示无穷大
    print("Workload execution successful.")
    print("Execution time: {:.4f} seconds".format(execution_time))
    print("Efficiency: {:.2f} requests per second".format(efficiency))


def main():
    # 读取workload.json文件
    with open("workload.json", "r") as f:
        workloads = json.load(f)

    # 遍历每个workload进行测试
    for i in range(len(workloads)):
        print("Executing workload #{}".format(i + 1))
        workload = workloads[f"workload{i+1}"]
        execute_workload(workload, 10)


if __name__ == "__main__":
    main()
