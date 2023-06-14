import os
import re
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
from matplotlib.ticker import FuncFormatter, MaxNLocator

plt.rcParams['figure.figsize'] = (20, 10)


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

        for part in data_parts[1:]:
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
                if len(processes) >= 3:
                    processes[0].join()
                    processes = processes[1:]
                counter += 1
                params = dict(zip(keys, permutation))
                self.set_params(**params)
                self.write_config_file(self.config_file_path)
                process = Process(target=self.run_cpp_program, args=(permutation,))
                time.sleep(1)
                processes.append(process)
                process.start()
                print(counter)
        config.plot_results(sort_values=sort_values, save_tikz=save_tikz)

    def plot_results(self, sort_values=False, save_tikz=False, errorPerColumn=False, calculateIntervalsInError=False):
        directory = 'csvs_old/'

        def format_func(value, tick_number):
            if value.is_integer():
                return f'{value:.0f}'
            else:
                return f'{value:.3f}'

        data = {}
        error_data = {}
        error_bound_values = []
        error_values = []

        for filename in os.listdir(directory):
            if filename.endswith('.csv'):
                df = pd.read_csv(directory + filename)
                if "-all.csv" in filename:
                    for column_name in df.columns:
                        if column_name not in data:
                            data[column_name] = {}
                        data[column_name][filename] = df[column_name].tolist()

        # for column_name, column_data in data.items():
        #     for filename in column_data:
        #         if not column_data[f"{filename}"]:
        #             print(filename)
        #             os.remove(directory + filename)

        data_df = pd.DataFrame(data)

        if calculateIntervalsInError:
            interval_ranges = np.arange(0, 0.1, 0.01)

            for filename in os.listdir(directory):
                if filename.endswith('.csv'):
                    df = pd.read_csv(directory + filename)
                    if "-columns.csv" in filename:
                        for column_name in ['avgErrorBound']:
                            df[column_name + '_Interval'] = pd.cut(df[column_name], bins=interval_ranges)
                            plt.figure(figsize=(10, 6))
                            (df[column_name + '_Interval'].value_counts(sort=False, normalize=True) * 100).plot(
                                kind='bar', color='b',
                                alpha=0.5)
                            plt.title(f'Distribution of {column_name} in {filename}')
                            plt.xlabel('AvgErrorBound Intervals')
                            plt.ylabel('Frequency (%)')
                            if save_tikz:
                                tikz.save(f'{filename}.tex')
                            else:
                                plt.tight_layout()
                                plt.show()
            # interval_ranges = np.arange(0, 10, 1)  # assuming the max value won't exceed 100
            # interval_labels = [f'{i}-{i + 1}' for i in interval_ranges[:-1]]
            #
            # for filename in os.listdir(directory):
            #     if filename.endswith('.csv'):
            #         df = pd.read_csv(directory + filename)
            #         if "-columns.csv" in filename:
            #             for column_name in ['avgErrorBound']:
            #                 df[column_name] = pd.cut(df[column_name], bins=interval_ranges, labels=interval_labels)
            #                 print(f"For {column_name} in {filename}:")
            #                 print(df[column_name].value_counts().sort_index())
            #                 print("\n")
        elif errorPerColumn:
            for filename in os.listdir(directory):
                if filename.endswith('.csv'):
                    df = pd.read_csv(directory + filename)
                    if "-columns.csv" in filename:
                        for column_name in df.keys():
                            if column_name == 'cid':
                                continue
                            if column_name not in error_data:
                                error_data[column_name] = {}
                            error_data[column_name][filename] = df[column_name].max()

            filenames = list(error_data['avgErrorBound'].keys())
            parts = [list(map(int, re.findall(r'\d+', filename))) for filename in filenames]
            changing_part_index = next(i for i in range(len(parts[0])) if len(set(part[i] for part in parts)) > 1)

            filenames.sort(key=lambda filename: int(filename.split('-')[changing_part_index]))

            max_values_error_bound = [error_data['avgErrorBound'][filename] for filename in filenames]
            max_values_error = [error_data['avgError'][filename] for filename in filenames]

            fig, ax = plt.subplots(figsize=(10, 7))
            ax.grid(True, linestyle='--')

            ax.plot(filenames, max_values_error_bound, color='r', marker='s', label='Max Value of avgErrorBound')
            ax.plot(filenames, max_values_error, color='b', marker='^', label='Max Value of avgError')

            ax.set_xticks(filenames)
            ax.set_xticklabels(filenames, rotation=45, ha='right')
            ax.set_title('Maximum Values of avgErrorBound and avgError')

            if save_tikz:
                tikz.save(f'{filename}.tex')
            else:
                plt.tight_layout()
                plt.show()

        else:
            for column_name in data_df.columns:

                if sort_values:
                    if column_name == "modelSize":
                        data_df = data_df.sort_values(by=column_name)

                column_data = data_df[column_name]

                plot_df = pd.DataFrame(column_data)

                if sort_values:
                    if column_name != "modelSize":
                        plot_df = plot_df.sort_values(by=column_name, axis=0, ascending=True)

                fig, ax = plt.subplots()
                ax.yaxis.grid(True, linestyle='--')

                line_values = [item for sublist in plot_df[column_name].dropna().values.tolist() for item in sublist]
                line_labels = plot_df.index.tolist()

                plt.xticks(rotation=45, ha='right')

                primary_line = ax.plot(line_labels, line_values, color='blue', marker='o')

                ax.set_ylabel(column_name)
                ax.set_title('Comparison of ' + column_name + ' across permutations')

                formatter = FuncFormatter(format_func)
                ax.yaxis.set_major_locator(MaxNLocator(nbins=20))
                ax.yaxis.set_major_formatter(formatter)

                if column_name == "modelSize":
                    wError_values = [item for sublist in data_df[' wErrorActual'].dropna().values.tolist() for item in
                                     sublist]
                    wErrorBound_values = [item for sublist in data_df[' wErrorBound'].dropna().values.tolist() for item
                                          in sublist]

                    ax2 = ax.twinx()
                    secondary_line_1 = ax2.plot(line_labels, wErrorBound_values, color='r', marker='s')
                    secondary_line_2 = ax2.plot(line_labels, wError_values, color='g', marker='^')
                    ax2.set_ylabel('Error (%)', color='r')
                    ax2.tick_params('y', colors='r')

                plt.tight_layout()
                if save_tikz:
                    tikz.save(f'{column_name}.tex')
                if True:
                    if not sort_values:
                        plt.savefig(f"{column_name}")
                    else:
                        plt.savefig(f"{column_name}-sorted")
# Initialize configuration
config = Config()

config.cpp_program_path = "../cmake-build-release/smartsendercpp.exe"

config.inputFile = "mars_subset_4(1).csv"

# Set columns with their error bounds and type
config.set_columns(range(2, 88), (5, 10), 3)

# Define permutations
params_dict = {
    "maxAge": ["1000000"],
    "budget": ["100000"],
    "chunkSize": ["500", "1000", "2000", "3000", "4000", "5000", "6000", "7000", "8000", "9000", "10000", "20000",
                  "30000", "40000", "50000", "60000", "70000", "80000", "90000", "100000"],
    "bufferGoal": ["10000"],
    "budgetLeftRegressionLength": ["10"],
    "chunksToGoal": ["10"]
}

# config.run_with_permutations(params_dict, sort_values=False, save_tikz=False)

# config.run_with_permutations(params_dict, True, True)

config.plot_results(sort_values=True, save_tikz=False, errorPerColumn=False, calculateIntervalsInError=False)
