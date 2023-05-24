import os
import subprocess
import itertools


class Config:
    def __init__(self):
        self.columns = {}
        self.timestamps = "1"
        self.inputFile = "cobham_hour_reloaded_no_pos.csv"
        self.maxAge = "180000"
        self.chunkSize = "604800"
        self.budget = "10000"
        self.bufferGoal = "2000"
        self.budgetLeftRegressionLength = "10"
        self.chunksToGoal = "10"
        self.cpp_program_path = "C:\\Users\\power\\CLionProjects\\smartsendercpp\\cmake-build-release\\smartsendercpp.exe"
        self.config_file_path = "../moby.cfg"

    def set_params(self, **kwargs):
        for key, value in kwargs.items():
            setattr(self, key, value)

    def set_columns(self, cols, error_bounds, threshold):
        for col in cols:
            self.columns[col] = [error_bounds, threshold]

    def columns_str(self):
        str_ = ""
        for col, vals in self.columns.items():
            str_ += "{} {}-{} {} ".format(col, vals[0][0], vals[0][1], vals[1])
        return str_.rstrip()

    def write_config_file(self, config_file_path):
        with open(config_file_path, 'w') as file:
            file.write("--columns=\"{}\"\n".format(self.columns_str()))
            file.write("--timestamps=\"{}\"\n".format(self.timestamps))
            file.write("--inputFile=\"{}\"\n".format(self.inputFile))
            file.write("--maxAge=\"{}\"\n".format(self.maxAge))
            file.write("--chunkSize=\"{}\"\n".format(self.chunkSize))
            file.write("--budget=\"{}\"\n".format(self.budget))
            file.write("--bufferGoal=\"{}\"\n".format(self.bufferGoal))
            file.write("--budgetLeftRegressionLength=\"{}\"\n".format(self.budgetLeftRegressionLength))
            file.write("--chunksToGoal=\"{}\"\n".format(self.chunksToGoal))

    def run_cpp_program(self):
        result = subprocess.run([self.cpp_program_path], text=True, capture_output=True)
        return result

    def run_with_permutations(self, params_dict):
        keys, values = zip(*params_dict.items())
        permutations = list(itertools.product(*values))

        for permutation in permutations:
            params = dict(zip(keys, permutation))
            self.set_params(**params)
            self.write_config_file(self.config_file_path)
            self.run_cpp_program()


# Initialize configuration
config = Config()

config.cpp_program_path = "../cmake-build-release/smartsendercpp.exe"

# Set columns with their error bounds and type
config.set_columns(range(2, 63), (5, 10), 4.0)

# Define permutations
params_dict = {
    "maxAge": ["180000", "190000"],
    "budget": ["20000", "30000"],
    "bufferGoal": ["2000", "3000"],
}

# Run with permutations
config.run_with_permutations(params_dict)
