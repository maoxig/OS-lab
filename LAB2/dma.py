class DMA:
    __slots__ = ["heap", "allocations", "heap_size", "free_space_start","offsets"]

    def __init__(self, size):
        self.heap = ["#"] * size
        self.allocations = {}
        self.heap_size = size
        self.free_space_start = 0
        self.offsets = []

    def malloc(self, id, size, value):

        if size > self.heap_size - self.free_space_start:
            return False
        start = self.free_space_start
        self.allocations[id] = {"start": start, "size": size, "value": value}
        self.heap[start : start + size] = value
        self.free_space_start += size
        return True

    def free(self, id):
        if id not in self.allocations:
            return False
        allocation = self.allocations.pop(id)
        old_start = allocation["start"]
        old_size = allocation["size"]

        self.free_space_start -= old_size
        del self.heap[old_start : old_start + old_size]
        self.heap += ["#"] * old_size
        self.offsets.append((old_start, old_size))
        return True

    def data(self):

        for old_start, old_size in self.offsets:
            for allocation in reversed(self.allocations.values()):
                if allocation["start"] > old_start:
                    allocation["start"] -= old_size
                else:
                    break
            self.offsets.pop(0)
        return self.allocations
