import numpy as np
import matplotlib.pyplot as plt

# Data
N = 5  # Number of groups
model_size_bytes = np.random.rand(N) * 1e9  # Mock model size in bytes
error_percentage1 = np.random.rand(N) * 100  # Mock error percentage1
error_percentage2 = np.random.rand(N) * 100  # Mock error percentage2

# X locations for groups
ind = np.arange(N)
primary_width = 0.3  # Width of the primary bars
secondary_width = 0.15  # Width of the secondary bars

fig, ax1 = plt.subplots()

# Primary bar plot
primary_x = ind
rects1 = ax1.bar(primary_x, model_size_bytes, primary_width, color='b', label='Model Size')
ax1.set_xlabel('Groups')
ax1.set_ylabel('Size in Bytes', color='b')
ax1.tick_params('y', colors='b')

# Secondary bar plots
ax2 = ax1.twinx()
secondary_x1 = primary_x + primary_width - secondary_width/2
secondary_x2 = secondary_x1 + secondary_width
rects2 = ax2.bar(secondary_x1, error_percentage1, secondary_width, color='r', label='Error Percentage 1')
rects3 = ax2.bar(secondary_x2, error_percentage2, secondary_width, color='g', label='Error Percentage 2')
ax2.set_ylabel('Error Percentage', color='r')
ax2.tick_params('y', colors='r')

# Add labels for groups
ax1.set_xticks(primary_x + (primary_width + secondary_width) / 2)
ax1.set_xticklabels(('Group1', 'Group2', 'Group3', 'Group4', 'Group5'))

# Add a legend
ax1.legend((rects1[0], rects2[0], rects3[0]), ('Model Size', 'Error Percentage 1', 'Error Percentage 2'))

plt.show()
