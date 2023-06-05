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
        first_file = "csvs/" + "-".join(permutation) + "-all.csv"
        second_file = "csvs/" + "-".join(permutation) + "-columns.csv"
        first_file_content = "modelSize, tsSize, wErrorBound, wErrorActual, errorBoundImportant, errorImportant, errorBoundNotImportant, errorNotImportant \n"
        second_file_content = "cid,avgErrorBound,avgError\n"

        data_parts = result.stderr.split(';')
        first_file_content += data_parts[0].strip() + '\n'

        # Process subsequent data parts
        for part in data_parts[1:]:
            # Split each part by comma and add newline after every third element
            part_data = part.strip().split(',')
            temp_content = ', '.join(
                part_data[i] if (i + 1) % 3 != 0 else part_data[i] + '\n' for i in range(len(part_data)))
            temp_content = temp_content.replace("\n, ", "\n")
            temp_content = temp_content.replace(" ", "")
            second_file_content += temp_content

        with open(first_file, "w") as text_file:
            text_file.write(first_file_content)

        with open(second_file, "w") as text_file:
            text_file.write(second_file_content)

        if result.returncode != 0:
            print(result.returncode)

    def run_with_permutations(self, params_dict, sort_values, save_tikz):
        keys, values = zip(*params_dict.items())
        permutations = list(itertools.product(*values))
        counter = 0

        processes = []
        if __name__ == '__main__':
            for permutation in permutations:
                self.run_cpp_program(permutation)
                # if len(processes) >= 3:
                #     processes[0].join()
                #     processes = processes[1:]
                # counter += 1
                # params = dict(zip(keys, permutation))
                # self.set_params(**params)
                # self.write_config_file(self.config_file_path)
                # process = Process(target=self.run_cpp_program, args=(permutation,))
                # time.sleep(1)
                # processes.append(process)
                # process.start()
                # print(counter)

            self.plot_results(sort_values=sort_values, save_tikz=save_tikz)

    def plot_results(self, sort_values=False, save_tikz=False):
        directory = 'csvs/'

        data = {}
        error_data = {}

        error_bound_values = []
        error_bound_labels = []

        error_values = []
        error_labels = []

        for filename in os.listdir(directory):
            if filename.endswith('.csv'):
                df = pd.read_csv(directory + filename)

                if "-all.csv" in filename:
                    for column_name in df.columns:
                        if column_name not in data:
                            data[column_name] = {}
                        data[column_name][filename] = df[column_name].tolist()

                elif "-columns.csv" in filename:
                    for column_name in df.keys():
                        if column_name == 'cid':
                            continue
                        if column_name not in error_data:
                            error_data[column_name] = {}
                        error_data[column_name][filename] = [df[column_name].max(), df[column_name].min()]
                    for j in range(len(error_data)):
                        error_bound_values.append(error_data['avgErrorBound'][filename][j])
                        error_bound_labels.append(filename)

                        error_values.append(error_data['avgError'][filename][j])
                        error_labels.append(filename)

        for column_name, column_data in data.items():
            for filename in column_data:
                if not column_data[f"{filename}"]:
                    print(filename)
                    os.remove(directory + filename)

        for column_name, column_data in data.items():

            plot_df = pd.DataFrame(column_data)

            fig, ax = plt.subplots()

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

            error_bound_x_values = [bar_labels.index(filename.replace("-columns.csv", "-all.csv")) + 0.5 for filename in
                                    error_bound_labels]
            error_x_values = [bar_labels.index(filename.replace("-columns.csv", "-all.csv")) + 0.75 for filename in
                              error_labels]

            if column_name == "modelSize":
                ax2 = ax.twinx()

                color = 'tab:blue'
                ax2.set_ylabel('Error (%)', color=color)
                ax2.tick_params(axis='y', labelcolor=color)

                ax2.plot(error_bound_x_values, error_bound_values, color='tab:blue', label='avgErrorBound')
                ax2.plot(error_x_values, error_values, color='tab:orange', label='avgError')

                ax2.legend(loc='upper left')

            plt.xticks(rotation=45, ha='right')

            if save_tikz:
                tikz.save(f'{column_name}.tex')
            else:
                # plt.show()
                if not sort_values:
                    plt.savefig(f"{column_name}")
                else:
                    plt.savefig(f"{column_name}-sorted")


# Initialize configuration
config = Config()

config.cpp_program_path = "../cmake-build-release/smartsendercpp.exe"

config.inputFile = "mars_subset_4.csv"

# Set columns with their error bounds and type
config.set_columns(range(2, 88), (5, 10), 3)

# Define permutations
params_dict = {
    "maxAge": ["100", "1000", "1000000"],
    "budget": ["72000"],
    "chunkSize": ["900"],
    "bufferGoal": ["10000"],
    "budgetLeftRegressionLength": ["10"],
    "chunksToGoal": ["10"]
}

config.run_with_permutations(params_dict, sort_values=False, save_tikz=False)
