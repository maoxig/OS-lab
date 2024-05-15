import time
import queue
from collections import deque

# 创建一个包含100万个字符的列表
char_list = ["a"] * 1000000
# 创建一个包含100万个字符的元组
char_tuple = tuple(char_list)
# 创建一个包含100万个字符的集合
char_set = set(char_list)
# 创建一个包含100万个字符的队列
char_queue = queue.Queue()
for char in char_list:
    char_queue.put(char)
# 创建一个包含100万个字符的双端队列
char_deque = deque(char_list)

# 遍历列表并计时
start_time = time.time()
for char in char_list:
    pass
list_time = time.time() - start_time

# 遍历元组并计时
start_time = time.time()
for char in char_tuple:
    pass
tuple_time = time.time() - start_time

# 遍历集合并计时
start_time = time.time()
for char in char_set:
    pass
set_time = time.time() - start_time

# 遍历队列并计时
start_time = time.time()
while not char_queue.empty():
    char_queue.get()
queue_time = time.time() - start_time

# 遍历双端队列并计时
start_time = time.time()
for char in char_deque:
    pass
deque_time = time.time() - start_time

# 输出遍历时间
print(f"List traversal time: {list_time:.6f} seconds")
print(f"Tuple traversal time: {tuple_time:.6f} seconds")
print(f"Set traversal time: {set_time:.6f} seconds")
print(f"Queue traversal time: {queue_time:.6f} seconds")
print(f"Deque traversal time: {deque_time:.6f} seconds")
