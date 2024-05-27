import timeit
import numpy as np
from array import array

# 创建一个包含1000个元素的NumPy数组、列表和array数组
size = 1000
np_arr = np.arange(size)
lst = list(range(size))
arr = array("i", range(size))

# 设置测试参数
offset = 10  # 偏移量
index = size // 2  # 从中间开始


# 定义一个函数来使用NumPy将指定索引之后的数都添加offset，并转换成列表
def modify_with_numpy_and_convert(arr, index, offset):
    arr[index:] += offset
    return arr.tolist()


# 定义一个函数来不使用NumPy将指定索引之后的数都添加offset，并转换成列表
def modify_with_list_and_convert(lst, index, offset):
    lst[index:] = [x + offset for x in lst[index:]]
    return lst




# 创建timeit的Timer对象来进行基准测试
numpy_convert_timer = timeit.Timer(
    lambda: modify_with_numpy_and_convert(np_arr.copy(), index, offset)
)
list_convert_timer = timeit.Timer(
    lambda: modify_with_list_and_convert(lst.copy(), index, offset)
)


# 进行基准测试，重复3次，每次运行1000次
numpy_convert_time = numpy_convert_timer.timeit(number=1000)
list_convert_time = list_convert_timer.timeit(number=1000)


# 输出结果
print(
    f"使用NumPy将指定索引之后的数都添加offset，并转换成列表时间: {numpy_convert_time} 秒"
)
print(
    f"使用Python列表将指定索引之后的数都添加offset，并转换成列表时间: {list_convert_time} 秒"
)
