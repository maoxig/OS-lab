class DMA:
    __slots__ = ["heap", "allocations", "free_size", "free_space_start"]

    def __init__(self, size):
        self.heap = ["#"] * size
        self.allocations = {}
        self.free_size = size
        self.free_space_start = 0

    def malloc(self, id, size, value):
        if size > self.free_size:
            return False
        start = self.free_space_start
        self.allocations[id] = {"start": start, "size": size, "value": value}
        self.heap[start : start + size] = value
        self.free_size -= size
        self.free_space_start += size
        return True

    def free(self, id):
        if id not in self.allocations:
            return False
        allocation = self.allocations.pop(id)
        old_start = allocation["start"]
        old_size = allocation["size"]
        self.free_size += old_size
        self.free_space_start -= old_size
        del self.heap[old_start : old_start + old_size]
        self.heap += ["#"] * old_size
        for allocation in reversed(self.allocations.values()):
            if allocation["start"] > old_start:
                allocation["start"] -= old_size
            else:
                break
        return True

    def data(self):
        return self.allocations

