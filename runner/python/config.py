class RunnerConfig:
    def __init__(self, bin_file: str, builder, name: str, repeat_num=1, time_limit: int = -1, memory_limit: int = -1):
        self.bin_file = bin_file
        self.time_limit = time_limit
        self.memory_limit = memory_limit
        self.command_builder = builder
        self.name = name
        self.repeat_num = repeat_num

    def build_command(self, task_file: str, result_file: str, extra_flags):
        command = [self.command_builder(self.bin_file, task_file, result_file, extra_flags), ">/dev/null", "2>/dev/null"]
        if self.time_limit != -1:
            command = ["timeout " + str(self.time_limit)] + command
        if self.memory_limit != -1:
            command = ['ulimit -v ' + str(self.memory_limit * 1024 * 1024) + ';'] + command
        return " ".join(command)
    