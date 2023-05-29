import os
import subprocess
import itertools
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import matplotlib.ticker as ticker
import tikzplotlib as tikz
import numpy as np


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

    def run_cpp_program(self, permutation, keys):
        result = subprocess.run([self.cpp_program_path], text=True, capture_output=True)
        file = "csvs/" + "-".join(permutation) + ".csv"
        text_file = open(file, "w")
        text_file.write("modelSize, tsSize, wErrorBound, wErrorActual \n")
        text_file.write(result.stderr)

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
                plt.show()

    def run_with_permutations(self, params_dict, sort_values, save_tikz):
        keys, values = zip(*params_dict.items())
        permutations = list(itertools.product(*values))
        counter = 0

        for permutation in permutations:
            counter += 1
            params = dict(zip(keys, permutation))
            self.set_params(**params)
            self.write_config_file(self.config_file_path)
            self.run_cpp_program(permutation, keys)
            print(counter)

        self.plot_results(sort_values=sort_values, save_tikz=save_tikz)


# Initialize configuration
config = Config()

config.cpp_program_path = "../cmake-build-release/smartsendercpp.exe"


config.inputFile = "mars_subset_4(1).csv"

# Set columns with their error bounds and type
config.set_columns(range(2, 88), (5, 10), 2.5)

# Define permutations
params_dict = {
    "maxAge": ["100", "1000", "10000"],
    "budget": ["100", "1000", "10000"],
    "chunkSize": ["100", "1000", "10000"],
    "bufferGoal": ["100", "1000", "10000"],
    "budgetLeftRegressionLength": ["10", "100", "1000"],
    "chunksToGoal": ["1", "10", "100"]
}

# Run with permutations
config.run_with_permutations(params_dict, sort_values=True, save_tikz=False)
