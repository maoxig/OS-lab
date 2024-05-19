import timeit

# 创建一个包含大量元素的字典
dict_size = 100000
test_dict = {f"key_{i}": i for i in range(dict_size)}


# 定义不同的查询方式
def direct_access(dict_obj, key):
    return dict_obj[key]


def get_method(dict_obj, key):
    return dict_obj.get(key)


def in_operator(dict_obj, key):
    return key in dict_obj


def items_iteration(dict_obj, key):
    for k, v in dict_obj.items():
        if k == key:
            return v
    return None


# 创建不同的测试语句
direct_access_stmt = f"direct_access(test_dict, 'key_99999')"
get_method_stmt = f"get_method(test_dict, 'key_99999')"
in_operator_stmt = f"in_operator(test_dict, 'key_99999')"
items_iteration_stmt = f"items_iteration(test_dict, 'key_99999')"

# 运行基准测试
print(
    "Direct Access: ",
    timeit.timeit(direct_access_stmt, globals=globals(), number=10000),
)
print("Get Method: ", timeit.timeit(get_method_stmt, globals=globals(), number=10000))
print("In Operator: ", timeit.timeit(in_operator_stmt, globals=globals(), number=10000))
print(
    "Items Iteration: ",
    timeit.timeit(items_iteration_stmt, globals=globals(), number=10000),
)
