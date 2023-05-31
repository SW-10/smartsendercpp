import os
import subprocess
import itertools
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import matplotlib.ticker as ticker
import tikzplotlib as tikz
import numpy as np
import time
from multiprocessing import Process


class Config:
    def __init__(self):
        self.columns = {}
        self.timestamps = "1"
        self.inputFile = "mars_subset_4(1).csv"
        self.maxAge = "10000"
        self.chunkSize = "1000"
        self.budget = "1000"
        self.bufferGoal = "1000"
        self.budgetLeftRegressionLength = "10"
        self.chunksToGoal = "10"
        self.cpp_program_path = "../cmake-build-release/smartsendercpp.exe"
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

    def run_cpp_program(self, permutation):
        self.write_config_file(self.config_file_path)
        result = subprocess.run([self.cpp_program_path], text=True, capture_output=True)
        file = "csvs/" + "-".join(permutation) + ".csv"
        text_file = open(file, "w")
        text_file.write("modelSize, tsSize, wErrorBound, wErrorActual \n")
        text_file.write(result.stderr)
        if result.returncode != 0:
            print(result.returncode)

    def plot_results(self, sort_values=False, save_tikz=False):
        directory = 'csvs/'

        data = {}

        for filename in os.listdir(directory):
            if filename.endswith('.csv'):
                df = pd.read_csv(directory + filename)

                for column_name in df.columns:
                    if column_name not in data:
                        data[column_name] = {}
                    data[column_name][filename] = df[column_name].tolist()

        for column_name, column_data in data.items():
            for filename in column_data:
                if not column_data[f"{filename}"]:
                    print(filename)
                    os.remove(directory + filename)

        for column_name, column_data in data.items():

            plot_df = pd.DataFrame(column_data)

            fig, ax = plt.subplots()

            width = 0.5

            if sort_values:
                plot_df = plot_df.sort_values(by=0, axis=1, ascending=False)

            bar_values = []
            bar_labels = []
            for i, filename in enumerate(plot_df.columns):
                for j in range(len(plot_df)):
                    if not np.isnan(plot_df[filename].iloc[j]):
                        bar_values.append(plot_df[filename].iloc[j])
                        bar_labels.append(filename)

            colors = cm.rainbow(np.linspace(0, 1, len(bar_values)))

            ax.bar(range(len(bar_values)), bar_values, tick_label=bar_labels, color=colors)

            ax.set_ylabel(column_name)
            ax.set_title('Comparison of ' + column_name + ' across CSV files')

            ax.yaxis.set_major_locator(ticker.MaxNLocator(nbins=20))

            plt.xticks(rotation=45, ha='right')

            plt.tight_layout()
            if save_tikz:
                tikz.save(f'{column_name}.tex')
            else:
                # plt.show()
                if not sort_values:
                    plt.savefig(f"{column_name}")
                else:
                    plt.savefig(f"{column_name}-sorted")

    def run_with_permutations(self, params_dict, sort_values, save_tikz):
        keys, values = zip(*params_dict.items())
        permutations = list(itertools.product(*values))
        counter = 0

        processes = []
        if __name__ == '__main__':
            for permutation in permutations:
                if len(processes) >= 3:
                    processes[0].join()
                    processes = processes[1:]
                counter += 1
                params = dict(zip(keys, permutation))
                self.set_params(**params)
                self.write_config_file(self.config_file_path)
                process = Process(target = self.run_cpp_program, args=(permutation,))
                time.sleep(1)
                processes.append(process)
                process.start()
                print(counter)

            #self.plot_results(sort_values=sort_values, save_tikz=save_tikz)


# Initialize configuration
config = Config()

config.cpp_program_path = "../cmake-build-release/smartsendercpp.exe"


config.inputFile = "mars-trimmed.csv"

# Set columns with their error bounds and type
config.set_columns(range(2, 88), (5, 10), 3)

# Define permutations
params_dict = {
    "maxAge": ["10000", "100000"],
    "budget": ["18000", "36000", "72000", "144000"],
    "chunkSize": ["1800", "3600", "7200", "14400"],
    "bufferGoal": ["10000"],
    "budgetLeftRegressionLength": ["10"],
    "chunksToGoal": ["5", "10", "20"]
}

# Run with permutations
#config.run_with_permutations(params_dict, sort_values=True, save_tikz=False)

config.plot_results(sort_values=True, save_tikz=False)
