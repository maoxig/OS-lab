class DMA:
    def __init__(self, size):
        self.heap = ["#"] * size
        self.allocations = {}
        self.free_size = size

    def malloc(self, id: int, size: int, value: list):

        if id in self.allocations:
            return False

        if size > self.free_size:
            return False
        else:

            start = len(self.heap) - self.free_size
            self.allocations[id] = {"start": start, "size": size, "value": value}
            self.heap[start : start + size] = value
            self.free_size -= size

            return True

    def free(self, id: int) -> bool:
        if id not in self.allocations:
            return False

        allocation = self.allocations.pop(id)
        old_start = allocation["start"]
        old_size = allocation["size"]

        self.free_size += old_size

        new_free_block_start = old_start

        sorted_blocks = sorted(
            self.allocations.items(), key=lambda item: item[1]["start"]
        )

        for id, allocation in sorted_blocks:
            start = allocation["start"]
            size = allocation["size"]
            if start > new_free_block_start:

                value = allocation["value"]
                self.heap[new_free_block_start : new_free_block_start + size] = value
                self.allocations[id]["start"] = new_free_block_start
                new_free_block_start += size

        self.heap[new_free_block_start : len(self.heap) + 1] = ["#"] * (
            len(self.heap) - new_free_block_start
        )

        return True

    def data(self) -> dict:
        return self.allocations
