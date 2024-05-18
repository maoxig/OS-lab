import timeit

# 定义两个列表
list1 = [1, 2, 3]
list2 = [4, 5, 6]


# 定义两个测试函数
def test_extend():
    result = list1.copy()
    result.extend(list2)


def test_plus_equal():
    result = list1.copy()
    result += list2


# 创建两个timeit.Timer对象
plus_equal_timer = timeit.Timer(test_plus_equal)
extend_timer = timeit.Timer(test_extend)


# 执行测试
plus_equal_time = plus_equal_timer.timeit(number=1000000)
extend_time = extend_timer.timeit(number=1000000)


# 输出结果
print(f"使用 '+=' 运算符连接列表的时间: {plus_equal_time:.6f} 秒")
print(f"使用 'extend' 连接列表的时间: {extend_time:.6f} 秒")
