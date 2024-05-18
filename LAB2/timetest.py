import timeit

# 创建一个包含大量键值对的字典
dictionary = {f"key_{i}": i for i in range(1000000)}

# 测试使用 .items() 遍历字典的时间
items_time = timeit.timeit(lambda: [item for item in dictionary.items()], number=100)

# 测试使用 .values() 遍历字典的时间
values_time = timeit.timeit(
    lambda: [value for value in dictionary.values()], number=100
)

# 输出结果
print(f"Using .items() time: {items_time} seconds")
print(f"Using .values() time: {values_time} seconds")
