from typing import List, Dict, Tuple
import sys


class DMA:
    def __init__(self, size):
        # 假设初始化时空间中每个地址都存储一个 # 符号
        self.heap: List[str] = ["#"] * size
        # 你可以在这里额外定义需要的数据结构
        self.allocations: Dict[int, Dict[str, int]] = {}
        self.free_size: int = size
        self.free_spaces: List[Tuple[int, int]] = [(0, size)]

    # 你需要实现以下三个 API
    def malloc(self, id: int, size: int, value: list) -> bool:
        if id in self.allocations:  # 这里的检查可以考虑删去
            return False
        if size > self.free_size:  # 一旦空闲大于需求，必须分配
            return False
        else:
            start = self.find_free_space(size)
            if start == -1:
                self.compact()  # 确保整理后的大小能够继续分配
                start = self.find_free_space(size)
            self.allocate_memory(id, start, size, value)

            self.free_size = self.free_size - size
            return True

    def free(self, id: int) -> bool:
        """释放指定id的空间段"""
        if id not in self.allocations:
            return False

        allocation = self.allocations[id]
        start = allocation["start"]
        size = allocation["size"]
        self.free_size = self.free_size + size

        self.free_memory(start, size)
        del self.allocations[id]
        return True

    def data(self) -> dict:
        return self.allocations

    # 除了上述 API 外，你可以额外定义必要的辅助函数
    # 例如，通过调用 compact 来对内存空间进行碎片整理
    def compact(self) -> None:
        free_spaces = self.get_free_spaces()
        if not free_spaces:
            return

        sorted_allocations = sorted(
            self.allocations.items(), key=lambda x: x[1]["start"]
        )
        new_heap = ["#"] * len(self.heap)
        new_allocations = {}

        current_start = 0
        for id, allocation in sorted_allocations:
            size = allocation["size"]
            value = allocation["value"]
            new_start = current_start
            new_allocations[id] = {"start": new_start, "size": size, "value": value}

            for i in range(size):
                new_heap[new_start + i] = value[i]

            current_start += size

        self.heap = new_heap
        self.allocations = new_allocations

    def find_free_space(self, size: int) -> int:
        """
        查找指定大小的空间，只查找，目前采用最佳适应，查找失败时返回-1
        """
        free_spaces = self.get_free_spaces()
        if not free_spaces:
            return -1
        best_start = -1
        best_size = sys.maxsize
        for start, free_size in free_spaces:
            if size <= free_size and size < best_size:
                best_size = size
                best_start = start
        return best_start

    def allocate_memory(self, id: int, start: int, size: int, value: list) -> None:
        """分配指定段内存，只分配"""
        self.allocations[id] = {"start": start, "size": size, "value": value}
        for i in range(size):
            self.heap[start + i] = value[i]

    def free_memory(self, start: int, size: int):
        """释放指定段内存，只释放"""
        for i in range(size):
            self.heap[start + i] = "#"

    def get_free_spaces(self):
        """获取空闲空间大小"""
        free_spaces = []
        start = None
        for i, char in enumerate(self.heap):
            if char == "#":
                if start is None:
                    start = i
            else:
                if start is not None:
                    size = i - start
                    free_spaces.append((start, size))
                    start = None

        if start is not None:
            size = len(self.heap) - start
            free_spaces.append((start, size))

        return free_spaces


if __name__ == "__main__":
    from test import main

    main()
