import os
import subprocess
import sys
import time
from config import RunnerConfig
from cache import *
from tqdm import tqdm
import random

seed_list = [3, 520, 1314]

def _get_tmp_output_file(output_folder: str):
    while True:
        file_id = random.randint(0, 10 ** 9)
        output_file = os.path.join(output_folder, str(file_id) + ".out")
        if not os.path.exists(output_file):
            os.system("touch %s" % output_file)
            return output_file


def _read_all_lines(file):
    with open(file, "r") as inp:
        lines = inp.readlines()
    res = []
    for line in lines:
        while len(line) > 0 and (line[0] == ' ' or line[0] == '\n'): line = line[1:]
        while len(line) > 0 and (line[-1] == ' ' or line[-1] == '\n'): line = line[:-1]
        if len(line) > 0: res.append(line)
    return res

def _deal_thread(thread_pool, pos, cache):
    status = thread_pool[pos]["thread"].poll()
    if status is not None and status != 1:
        output_file = thread_pool[pos]["output_file"]
        name = thread_pool[pos]["name"]
        seed = thread_pool[pos]["seed"]
        result = _read_all_lines(output_file)
        if name not in cache: cache[name] = []
        if len(result) == 0:
            status = {"status": False, "seed": seed}
        else:
            res = result[:-1]
            time_cost = float(result[-1])
            status = {"status": True, "result": res, "time": time_cost, "seed": seed}
        cache[name].append(status)
        os.system("rm %s" % output_file)
        thread_pool[pos] = None


def _run_command(thread_pool, command, name, output_file, cache, seed):
    pos = None
    while pos is None:
        for i in range(len(thread_pool)):
            if thread_pool[i] is not None:
                _deal_thread(thread_pool, i, cache)
            if thread_pool[i] is None:
                pos = i
        time.sleep(0.1)

    thread_pool[pos] = {
        "name": name,
        "seed": seed,
        "output_file": output_file,
        "thread": subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
    }


def _join_all(thread_pool, cache):
    while True:
        is_clear = True
        for i in range(len(thread_pool)):
            if thread_pool[i] is not None:
                _deal_thread(thread_pool, i, cache)
                is_clear = False
        if is_clear: return


def _get_benchmark_name(path: str):
    return os.path.splitext(os.path.basename(path))[0]


def execute(config: RunnerConfig, benchmark_list: list, cache_path: str, thread_num=4,
            output_folder="/tmp/", save_step=5):
    print("current:", os.path.basename(cache_path), config.name)
    thread_pool = [None for _ in range(thread_num)]
    cache = load_cache(cache_path)
    if config.name not in cache:
        cache[config.name] = {}
    step_num = 0

    pre_list = benchmark_list
    benchmark_list = []

    for seed in seed_list[:config.repeat_num]:
        for path in pre_list:
            benchmark_name = _get_benchmark_name(path)
            is_finished = False
            if benchmark_name in cache[config.name]:
                for status in cache[config.name][benchmark_name]:
                    if int(status["seed"]) == seed:
                        is_finished = True
            if not is_finished:
                benchmark_list.append((seed, path))

    if len(benchmark_list) == 0: return cache
    is_cover = False

    for (seed, benchmark_file) in tqdm(benchmark_list):
        benchmark_name = _get_benchmark_name(benchmark_file)

        output_file = _get_tmp_output_file(output_folder)
        command = config.build_command(benchmark_file, output_file, {"seed": seed})
        _run_command(thread_pool, command, benchmark_name, output_file, cache[config.name], seed)

        step_num += 1
        if step_num % save_step == 0:
            save_cache(cache_path, cache, is_cover)
            is_cover = True

    _join_all(thread_pool, cache[config.name])
    save_cache(cache_path, cache, is_cover)
    return cache


def get_all_benchmark(path: str, valid=lambda f: ".sl" in f):
    path_list = os.listdir(path)
    benchmark_list = []
    for file in path_list:
        if valid(file):
            benchmark_list.append(os.path.join(path, file))
    return benchmark_list