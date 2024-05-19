import json
import time
from dma import DMA


def execute_workload(workload, times: int):
    heap_size = workload["size"]
    requests = workload["requests"]
    expected_results = workload["result"]
    expected_assertions = workload["assert"]

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
                # assert (
                #     result == expected_assertions[i]
                # ), f"Assertion failed for malloc {req_id}"

                # # 检查分配的内存是否正确
                #allocation = dma.data().get(req_id)
                # if allocation:
                #     start = allocation["start"]
                #     size = allocation["size"]
                #     value = allocation["value"]
                #     assert set(dma.heap[start : start + size]) == set(
                #         value
                #     ), f"Memory content mismatch for malloc {req_id}"

            elif req_type == "free":
                result = dma.free(req_id)
                # assert (
                #     result == expected_assertions[i]
                # ), f"Assertion failed for free {req_id}"

                # # 检查释放的内存是否已标记为空闲
                allocations = dma.data()
                # new_heap = dma.heap.copy()

                # for id, allocation in allocations.items():
                #     start = allocation["start"]
                #     size = allocation["size"]
                #     new_heap[start : start + size] = ["#"] * size

                # free_heap = ["#"] * heap_size

                # assert (
                #     new_heap == free_heap
                # ), f"Memory content mismatch for free {req_id}"

        # 在每个workload执行后计算并输出碎片大小
        # 碎片大小可以通过计算连续空闲块的数量和位置来得出
        # 这里需要实现一个方法来计算碎片大小并打印

    end_time = time.time()
    print(f"Execution time for {times} iterations: {end_time - start_time:.6f} seconds")

    execution_time = end_time - start_time
    # # 检查heap中的分配结果
    # allocations = dma.data()
    # for expected_result in expected_results:
    #     req_id = expected_result["id"]
    #     req_size = expected_result["size"]
    #     req_value = expected_result["value"]

    #     assert req_id in allocations
    #     allocation = allocations[req_id]
    #     assert allocation["size"] == req_size
    #     assert set(allocation["value"]) == set(req_value)
    # if execution_time > 0:
    #     efficiency = len(requests) / execution_time
    # else:
    #     efficiency = float("inf")  # 表示无穷大
    # print("Workload execution successful.")
    # print("Execution time: {:.4f} seconds".format(execution_time))
    # print("Efficiency: {:.2f} requests per second".format(efficiency))


def main():
    # 读取workload.json文件
    with open("workload.json", "r") as f:
        workloads = json.load(f)

    # 遍历每个workload进行测试
    for i in range(len(workloads)):
        print("Executing workload #{}".format(i + 1))
        workload = workloads[f"workload{i+1}"]
        execute_workload(workload, 100)


# 使用cProfile来分析main函数
if __name__ == "__main__":
    import cProfile

    cProfile.run("main()", sort="cumtime")
