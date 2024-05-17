from typing import List, Dict, Tuple

import sys


class FreeBlock:
    def __init__(self, start: int, size: int):
        self.start = start
        self.size = size
        self.next = None
        self.prev = None


class DMA:
    def __init__(self, size):
        self.heap: List[str] = ["#"] * size
        self.allocations: Dict[int, Dict[str, int]] = {}
        self.free_block_list = FreeBlock(0, size)
        self.free_size = size

    def malloc(self, id: int, size: int, value: list) -> bool:
        if id in self.allocations:
            return False

        if size > self.free_size:  # 如果空闲空间不够，尝试整理碎片
            return False
        else:
            block = self.find_best_fit_block(size)
            if block is None:
                self.compact()

            block = self.find_best_fit_block(size)
            self.allocate_block(id, block, size, value)
            self.free_size -= size
            return True

    def free(self, id: int) -> bool:
        if id not in self.allocations:
            return False

        allocation = self.allocations.pop(id)
        start = allocation["start"]
        size = allocation["size"]
        self.free_block(start, size)
        self.free_size += size
        return True

    def data(self) -> dict:
        return self.allocations

    def find_best_fit_block(self, size: int) -> FreeBlock:
        best_block = None
        best_block_size = sys.maxsize
        current_block = self.free_block_list.next

        while current_block is not None:
            if current_block.size >= size and current_block.size < best_block_size:
                best_block = current_block
                best_block_size = current_block.size
            current_block = current_block.next

        return best_block

    def allocate_block(self, id: int, block: FreeBlock, size: int, value: list):
        start = block.start
        self.allocations[id] = {"start": start, "size": size, "value": value}
        self.heap[start : start + size] = value

        if block.size == size:
            self.remove_free_block(block)
        else:
            block.start += size
            block.size -= size

    def free_block(self, start: int, size: int):
        self.heap[start : start + size] = ["#"] * size
        self.insert_free_block(start, size)

    def insert_free_block(self, start: int, size: int):
        new_block = FreeBlock(start, size)
        prev_block = self.find_previous_block(start)
        next_block = prev_block.next

        new_block.prev = prev_block
        new_block.next = next_block

        if next_block:
            next_block.prev = new_block
        prev_block.next = new_block

        self.merge_adjacent_blocks(new_block)

    def find_previous_block(self, start: int) -> FreeBlock:
        current_block = self.free_block_list
        while current_block.next and current_block.next.start <= start:
            current_block = current_block.next
        return current_block

    def compact(self) -> None:
        # 记录新的空闲块开始位置
        new_free_block_start = 0
        # 遍历所有已分配的块，并将它们移动到堆的开始处
        for id, allocation in sorted(
            self.allocations.items(), key=lambda item: item[1]["start"]
        ):
            start = allocation["start"]
            size = allocation["size"]
            value = allocation["value"]
            # 只有当当前块不在正确的位置时才移动
            if start != new_free_block_start:
                self.heap[new_free_block_start : new_free_block_start + size] = value
                # 更新块的起始地址
                self.allocations[id]["start"] = new_free_block_start
            new_free_block_start += size

        # 清空剩余的堆空间作为新的空闲块
        self.free_memory(new_free_block_start, len(self.heap) - new_free_block_start)
        # 重新设置空闲块列表
        self.free_block_list = FreeBlock(0, new_free_block_start)
        self.free_block_list.next = FreeBlock(
            new_free_block_start, len(self.heap) - new_free_block_start
        )
        self.free_block_list.next.prev = self.free_block_list

    def remove_free_block(self, block: FreeBlock):
        if block.prev:
            block.prev.next = block.next
        if block.next:
            block.next.prev = block.prev
        if block == self.free_block_list.next:
            self.free_block_list.next = block.next
        if block == self.free_block_list.prev:
            self.free_block_list.prev = block.prev

    def merge_adjacent_blocks(self, block: FreeBlock):
        if block.next and block.next.start == block.start + block.size:
            block.size += block.next.size
            self.remove_free_block(block.next)
            self.merge_adjacent_blocks(block)

        if block.prev and block.prev.start + block.prev.size == block.start:
            block.prev.size += block.size
            self.remove_free_block(block)
            self.merge_adjacent_blocks(block.prev)

    def free_memory(self, start: int, size: int):
        """释放指定段内存，只释放"""
        self.heap[start : start + size] = ["#"] * size
        # for i in range(size):
        #     self.heap[start + i] = "#"


