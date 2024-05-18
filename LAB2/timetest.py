import time

# 创建一个大型列表
list_size = 1000000
test_list = list(range(list_size))

# 直接遍历列表
start_time = time.time()
for item in test_list:
    _ = item
direct_iter_time = time.time() - start_time

# 使用enumerate遍历列表
start_time = time.time()
for index, item in enumerate(test_list):
    _ = index, item
enumerate_time = time.time() - start_time

# 输出结果
print(f"直接遍历列表所需时间: {direct_iter_time:.6f} 秒")
print(f"使用enumerate遍历列表所需时间: {enumerate_time:.6f} 秒")
