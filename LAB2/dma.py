import numpy as np
class DMA:
    __slots__ = ["heap", "allocations", "heap_size", "free_space_start"]

    def __init__(self, size):
        self.heap = ["#"] * size
        self.allocations = np.recarray(
            (0,), dtype=[("id", "U10"), ("start", int), ("size", int), ("value", "U1")]
        )
        self.heap_size = size
        self.free_space_start = 0

    def malloc(self, id, size, value):
        if size > self.heap_size - self.free_space_start:
            return False
        start = self.free_space_start

        # Update numpy recarray
        self.allocations = np.append(
            self.allocations,
            np.rec.array([(id, start, size, value[0])], dtype=self.allocations.dtype),
        )

        self.heap[start : start + size] = value
        self.free_space_start += size
        return True

    def free(self, id):
        if id not in [a["id"] for a in self.allocations]:
            return False

        # Find the index of the allocation to be freed
        index_to_free = np.where(self.allocations["id"] == id)[0][0]
        allocation = self.allocations[index_to_free]
        old_start = allocation["start"]
        old_size = allocation["size"]

        self.free_space_start -= old_size
        self.heap[old_start : old_start + old_size] = ["#"] * old_size

        # Update the start positions of allocations that come after the freed allocation
        mask = self.allocations["start"] > old_start
        self.allocations["start"][mask] -= old_size

        # Remove the freed allocation from the recarray
        self.allocations = np.delete(self.allocations, index_to_free)
        return True

    def data(self):
        # Combine the numpy recarray with the heap data
        data = {
            allocation["id"]: {
                "start": allocation["start"],
                "size": allocation["size"],
                "value": self.heap[
                    allocation["start"] : allocation["start"] + allocation["size"]
                ],
            }
            for allocation in self.allocations
        }
        return data



# Testing the modified DMA class
dma = DMA(10)
dma_malloc_result = dma.malloc("test_id", 3, "asdb")
dma_data = dma.data()
dma_free_result = dma.free("test_id")
dma_data_after_free = dma.data()

dma_malloc_result, dma_data, dma_free_result, dma_data_after_free
