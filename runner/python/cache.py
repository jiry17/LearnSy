import json
import os


def load_cache(cache_path: str):
    if not os.path.exists(cache_path):
        return {}
    with open(cache_path, "r") as inp:
        return json.load(inp)

def backup_cache(cache_path: str):
    if not os.path.exists(cache_path): return
    name, ext_name = os.path.splitext(cache_path)
    name += "_bk"
    backup_id = 0
    while os.path.exists(name + str(backup_id) + ext_name):
        backup_id += 1
    backup_path = name + str(backup_id) + ext_name
    os.system("cp %s %s" % (cache_path, backup_path))

def save_cache(cache_path: str, cache, is_cover: bool):
    if os.path.exists(cache_path) and not is_cover:
        backup_cache(cache_path)
    with open(cache_path, "w") as oup:
        json.dump(cache, oup, indent=4)

def remove_util(cache_path, cond):
    pre_result = load_cache(cache_path)
    result = {}

    for selector, pre_selector_res in pre_result.items():
        selector_res = {}
        for task_name, pre_status_list in pre_selector_res.items():
            status_list = []
            for status in pre_status_list:
                if not cond(selector, task_name, status):
                    status_list.append(status)
            if len(status_list) > 0:
                selector_res[task_name] = status_list
        if len(selector_res) > 0:
            result[selector] = selector_res

    save_cache(cache_path, result, False)