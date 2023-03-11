from cache import *
import os

class CaredValue:
    def __init__(self, f, name):
        self.f = f
        self.name = name

val_time = CaredValue(lambda x: x["time"], "time")
val_example = CaredValue(lambda x: int(x["result"][0].split(' ')[0]), "example")

def _is_success(res_item):
    for item in res_item:
        if item["status"]: return True
    return False


def _apply_f(cared_val: CaredValue, res_item):
    res = []
    for item in res_item:
        if item["status"]:
            res.append(cared_val.f(item))
    assert len(res) > 0
    return sum(res) / len(res)

def _get_solved_num(result, name):
    num = 0
    for task, res in result[name].items():
        if _is_success(res):
            num += 1
    return num

def _calculate_all(cared_val: CaredValue, result):
    data = {}
    for name, res in result.items():
        if _is_success(res):
            data[name] = _apply_f(cared_val, res)
    return data

def _get_all_solved(result_map, solver_list):
    all_solved_tasks = []
    for task in result_map[solver_list[0]]:
        flag = True
        for solver in solver_list:
            if task not in result_map[solver] or not _is_success(result_map[solver][task]):
                flag = False
                break
        if flag: all_solved_tasks.append(task)
    return all_solved_tasks

def _get_average_map(result_map, selector_list, cared_value):
    all_solved_list = _get_all_solved(result_map, selector_list)
    if len(all_solved_list) == 0:
        return {name: None for name in selector_list}
    res = {}
    for selector in selector_list:
        all_res = _calculate_all(cared_value, result_map[selector])
        total = 0
        for task in all_solved_list: total += all_res[task]
        res[selector] = total / len(all_solved_list)
    return res

def _compared_all(learnsy_ave, baseline_ave):
    if learnsy_ave is None or baseline_ave is None:
        return [None, None]
    return [baseline_ave - learnsy_ave, (baseline_ave - learnsy_ave) / baseline_ave * 100]

def _get_hard_status(result_map, learnsy_name, baseline_name, cared_value):
    learnsy_res = _calculate_all(cared_value, result_map[learnsy_name])
    baseline_res = _calculate_all(cared_value, result_map[baseline_name])
    both_solved_list = _get_all_solved(result_map, [learnsy_name, baseline_name])
    if len(both_solved_list) == 0:
        return 0, (0, 0)
    info_list = sorted([(learnsy_res[name] + baseline_res[name], name) for name in both_solved_list])
    num = max(1, len(info_list) // 10)
    learnsy_sum, baseline_sum = 0, 0
    for _, name in info_list[-num:]:
        learnsy_sum += learnsy_res[name]
        baseline_sum += baseline_res[name]
    return num, (learnsy_sum, baseline_sum)

def _compared_hard(result_map, learnsy_name, baseline_name, cared_value):
    num, (learnsy_sum, baseline_sum) = _get_hard_status(result_map, learnsy_name, baseline_name, cared_value)
    if num == 0: return [None, None]
    return [(baseline_sum - learnsy_sum) / num, (baseline_sum - learnsy_sum) / baseline_sum * 100]
def _compared_merged_hard(ir_result_map, is_result_map, learnsy_name, baseline_name, cared_value):
    num_ir, (learnsy_ir, baseline_ir) = _get_hard_status(ir_result_map, learnsy_name, baseline_name, cared_value)
    num_is, (learnsy_is, baseline_is) = _get_hard_status(is_result_map, learnsy_name, baseline_name, cared_value)
    num, (learnsy_sum, baseline_sum) = num_ir + num_is, (learnsy_is + learnsy_ir, baseline_ir + baseline_is)
    if num == 0: return [None, None]
    return [(baseline_sum - learnsy_sum) / num, (baseline_sum - learnsy_sum) / baseline_sum * 100]

def _round_string(s, size):
    assert len(s) <= size
    while len(s) + 2 <= size:
        s = " " + s + " "
    if len(s) < size:
        return " " + s
    return s

def _print_row(size_list, val_list):
    print("|", end="")
    for size, val in zip(size_list, val_list):
        if type(val) == str:
            s = val
        elif type(val) == float:
            s = str(val)[:5]
        else:
            s = str(val)
        print("%s|" % _round_string(s, size), end="")
    print()

def _get_name(name):
    if name == "samplesy120": return "SampleSy"
    if name[:8] == "samplesy": return "SampleSy" + name[8:]
    if name[:7] == "learnsy": return "LearnSy" + name[7:]
    name_map = {"eusolver": "EuSolver", "maxflash": "MaxFlash", "polygen": "PolyGen",
                "default": "Default", "significant": "SigInp", "biased": "Biased", "randomsy": "RandomSy"}
    return name_map[name]

def _print_warning(warning_list):
    for dataset_name, solver_name, selector_name in warning_list:
        content = "[Warning] Results of selector " + _get_name(selector_name) + " on dataset " + dataset_name
        if solver_name is not None: content += " with solver " + _get_name(solver_name)
        content += " is missing"
        print(content)

def _merge_result(ir_result, is_result):
    merged_result = {}
    for name in ir_result:
        if name in is_result:
            merged_result[name] = {}
            for task in ir_result[name]:
                merged_result[name][task] = ir_result[name][task]
            for task in is_result[name]:
                merged_result[name][task] = is_result[name][task]
    return merged_result

def draw_table3(cache_dir):
    print(), print()
    print("Table 3. The results of comparing LearnSy with baselines on interactive tasks.")
    print()
    _print_row([12, 14, 39], ["Selector", "#Solved", "#Iteration"])

    row_size = [12, 14, 7, 7, 7, 7, 7]
    _print_row(row_size, ["", "", "Ave", "RD(#)", "RD(%)", "RDH(#)", "RDH(%)"])

    selector_list = ["learnsy", "randomsy"] + ["samplesy" + str(t) for t in [3, 5, 10, 15, 120]]
    root = selector_list[0]
    warning_list = []

    ir_path = os.path.join(cache_dir, "intsy_repair.json")
    is_path = os.path.join(cache_dir, "intsy_string.json")
    ir_result, is_result = load_cache(ir_path), load_cache(is_path)
    current_selectors = list(filter(lambda x: x in ir_result and x in is_result, selector_list))
    for selector in selector_list:
        if selector not in ir_result:
            warning_list.append(("IR", None, selector))
        if selector not in is_result:
            warning_list.append(("IS", None, selector))
    merged_result = _merge_result(ir_result, is_result)
    if root in ir_result and root in is_result:
        merged_ave_map = _get_average_map(merged_result, current_selectors, val_example)
        print("|" + "-" * (sum(row_size) + len(row_size) - 1) + "|")
        _print_row(row_size, [_get_name(root), _get_solved_num(merged_result, root), merged_ave_map[root], "", "", "", ""])

        for selector in current_selectors[1:]:
            val_list = [_get_name(selector), _get_solved_num(merged_result, selector), merged_ave_map[selector]] + \
                        _compared_all(merged_ave_map[root], merged_ave_map[selector]) + _compared_merged_hard(is_result, ir_result, root, selector, val_example)
            _print_row(row_size, val_list)

    _print_warning(warning_list)

def draw_table5(cache_dir):
    print(), print()
    print("Table 5. The results of comparing LearnSy with baselines on non-interactive tasks.")
    print()
    # print head
    _print_row([9, 12, 10, 10, 23, 39], ["Dataset", "PBE Solver", "Selector", "#Solved", "#Iteration", "TimeCost (s)"])

    row_size = [9, 12, 10, 10, 7, 7, 7, 7, 7, 7, 7, 7]
    _print_row(row_size, ["", "", "", "", "Ave", "RD(#)", "RD(%)", "Ave", "RD(s)", "RD(%)", "RDH(s)", "RD(%)"])

    dataset_list = [("CS", "string", ["eusolver", "maxflash"], ["learnsy", "default", "significant"]),
                    ("CI", "clia", ["eusolver", "polygen"], ["learnsy", "default"]),
                    ("CB", "bv", ["eusolver"], ["learnsy", "default", "biased"])]
    warning_list = []

    for dataset_name, dataset, solver_list, selector_list in dataset_list:
        is_dataset_first = True
        for solver in solver_list:
            cache_path = os.path.join(cache_dir, solver + "_" + dataset + ".json")
            result_map = load_cache(cache_path)

            root = selector_list[0]
            current_selectors = list(filter(lambda name: name in result_map, selector_list))
            for selector in selector_list:
                if selector not in result_map:
                    warning_list.append((dataset_name, solver, selector))

            if root not in result_map: continue

            ave_example_map = _get_average_map(result_map, current_selectors, val_example)
            ave_time_map = _get_average_map(result_map, current_selectors, val_time)

            if is_dataset_first:
                print("|" + "-" * (sum(row_size) + len(row_size) - 1) + "|")
            else:
                print("|" + " " * 9 + "|" + "-" * (sum(row_size[1:]) + len(row_size) - 2) + "|")
            _print_row(row_size, [dataset_name if is_dataset_first else "", _get_name(solver), _get_name(root),
                                  _get_solved_num(result_map, root), ave_example_map[root], "", "", ave_time_map[root],
                                  "", "", "", ""])
            is_dataset_first = False

            for selector in current_selectors[1:]:
                val_list = ["", "", _get_name(selector), _get_solved_num(result_map, selector), ave_example_map[selector]] + \
                           _compared_all(ave_example_map[root], ave_example_map[selector]) + [ave_time_map[selector]] + \
                           _compared_all(ave_time_map[root], ave_time_map[selector]) + \
                           _compared_hard(result_map, root, selector, val_time)
                _print_row(row_size, val_list)

    _print_warning(warning_list)

def draw_table6(cache_dir):
    print(), print()
    print("Table 6. The results of comparing LearnSy with its variants.")
    print()

    _print_row([9, 12, 5, 10, 10, 23, 39],
               ["Dataset", "PBE Solver", "Exp", "Selector", "#Solved", "#Iteration", "TimeCost (s)"])
    row_size = [9, 12, 5, 10, 10, 7, 7, 7, 7, 7, 7, 7, 7]
    _print_row(row_size, ["", "", "", "", "", "Ave", "RD(#)", "RD(%)", "Ave", "RD(s)", "RD(%)", "RDH(s)", "RDH(%)"])
    warning_list = []

    cache_path = os.path.join(cache_dir, "maxflash_string.json")
    result_map = load_cache(cache_path)
    exp_list = [(3, ["learnsy", "learnsyU"]), (4, ["learnsy", "learnsy0"])]

    for selector in ["learnsy", "learnsyU", "learnsy0"]:
        if selector not in result_map:
            warning_list.append(("CS", "maxflash", selector))

    is_start = True
    for exp_id, (root, baseline) in exp_list:
        if root not in result_map or baseline not in result_map:
            continue
        if is_start:
            print("|" + "-" * (sum(row_size) + len(row_size) - 1) + "|")
        else:
            print("|", end="")
            for size in row_size[:2]: print(" " * size + "|", end="")
            print("-" * (sum(row_size[2:]) + len(row_size) - 3) + "|")

        ave_example_map = _get_average_map(result_map, [root, baseline], val_example)
        ave_time_map = _get_average_map(result_map, [root, baseline], val_time)

        _print_row(row_size, ["CS" if is_start else "", "MaxFlash" if is_start else "", exp_id, _get_name(root),
                              _get_solved_num(result_map, root), ave_example_map[root], "", "",
                              ave_time_map[root], "", "", "", ""])
        val_list = ["", "", "", _get_name(baseline), _get_solved_num(result_map, baseline), ave_example_map[baseline]] + \
                   _compared_all(ave_example_map[root], ave_example_map[baseline]) + [ave_time_map[baseline]] + \
                   _compared_all(ave_time_map[root], ave_time_map[baseline]) + \
                   _compared_hard(result_map, root, baseline, val_time)
        _print_row(row_size, val_list)

    _print_warning(warning_list)