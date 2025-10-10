import matplotlib.pyplot as plt
import pandas as pd
import os

# Load the CSV file
df = pd.read_csv("MVM_CKPT/test_results.csv")

# Identify the unique groups based on (size, cache_flush, mod, ops)
group_cols = ['size', 'cache_flush', 'mod', 'ops']
unique_groups = df.groupby(group_cols).groups.keys()

if not os.path.exists("graphs_ckpt"):
    os.makedirs("graphs_ckpt")

# Iterate through each unique group and generate a stacked bar graph
for group_key in unique_groups:
    # Filter data for the current group
    group_df = df[(df['size'] == group_key[0]) &
                  (df['cache_flush'] == group_key[1]) &
                  (df['mod'] == group_key[2]) &
                  (df['ops'] == group_key[3])].copy()

    # Sort by 'writes' for a logical progression on the x-axis
    group_df = group_df.sort_values('writes')

    # Prepare data for plotting
    x_labels = group_df['writes'].astype(str)
    ckpt_time = group_df['ckpt_time']
    restore_time = group_df['restore_time']

    # Create the stacked bar plot
    fig, ax = plt.subplots(figsize=(12, 6))

    # Plot ckpt_time (bottom)
    ax.bar(x_labels, ckpt_time, label='ckpt_time', color='#1f77b4')

    # Plot restore_time (on top of ckpt_time)
    # The 'bottom' argument ensures stacking starts where ckpt_time ends
    ax.bar(x_labels, restore_time, bottom=ckpt_time, label='restore_time', color='#ff7f0e')

    # Set labels and title
    ax.set_xlabel('Writes (Number of Write Operations)') # X-axis label set to 'Writes'
    ax.set_ylabel('Time (s)')

    # Construct a descriptive title
    title = (f"Stacked Time for (size={group_key[0]}, cache_flush={group_key[1]}, "
             f"mod={group_key[2]}, ops={group_key[3]})")
    ax.set_title(title, wrap=True)

    # Add legend
    ax.legend()

    # Rotate x-axis labels for better readability
    plt.xticks(rotation=45, ha='right')

    # Adjust layout to prevent labels from being cut off
    plt.tight_layout()

    # Create a filename
    filename = (f"graphs_ckpt/stacked_bar_size_{group_key[0]}_flush_{group_key[1]}_mod_{group_key[2]}_ops_{group_key[3]}.png")
    plt.savefig(filename)
    plt.close(fig)


# Load the CSV file
df = pd.read_csv("MVM/test_results.csv")

group_cols = ['cache_flush', 'ops']
unique_groups = df.groupby(group_cols).groups.keys()

if not os.path.exists("graphs_mvm"):
    os.makedirs("graphs_mvm")

for group_key in unique_groups:
    # Filter data for the current group
    group_df = df[(df['cache_flush'] == group_key[0]) &
                  (df['ops'] == group_key[1])].copy()

    # Sort by 'writes' for a logical progression on the x-axis
    group_df = group_df.sort_values('writes')

    # Prepare data for plotting
    x_labels = group_df['writes'].astype(str)
    time = group_df['time']

    # Create the stacked bar plot
    fig, ax = plt.subplots(figsize=(12, 6))

    # Plot time
    ax.bar(x_labels, time, label='time', color='#1f77b4')

    # Set labels and title
    ax.set_xlabel('Writes (Number of Write Operations)') # X-axis label set to 'Writes'
    ax.set_ylabel('Time (s)')

    # Construct a descriptive title
    title = (f"Time for (cache_flush={group_key[0]}, ops={group_key[1]})")
    ax.set_title(title, wrap=True)

    # Add legend
    ax.legend()

    # Rotate x-axis labels for better readability
    plt.xticks(rotation=45, ha='right')

    # Adjust layout to prevent labels from being cut off
    plt.tight_layout()

    # Create a filename
    filename = (f"graphs_mvm/bar_cache_flush_{group_key[0]}_ops_{group_key[1]}.png")
    plt.savefig(filename)
    plt.close(fig)
