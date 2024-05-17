class DMA:
    def __init__(self, size):
        self.heap = ["#"] * size
        self.allocations = {}
        self.free_size = size

    def malloc(self, id, size, value):
        if id in self.allocations or size > self.free_size:
            return False

        start = len(self.heap) - self.free_size
        self.allocations[id] = {"start": start, "size": size, "value": value}
        self.heap[start : start + size] = value
        self.free_size -= size
        return True

    def free(self, id):
        if id not in self.allocations:
            return False

        allocation = self.allocations.pop(id)
        old_start = allocation["start"]
        old_size = allocation["size"]
        self.free_size += old_size

        for id, allocation in self.allocations.items():
            start = allocation["start"]

            if start > old_start:
                size = allocation["size"]
                new_start = start - old_size
                self.heap[new_start : new_start + size] = allocation["value"]
                self.allocations[id]["start"] = new_start

        self.heap[len(self.heap) - self.free_size : len(self.heap) + 1] = [
            "#"
        ] * self.free_size
        return True

    def data(self):
        return self.allocations
