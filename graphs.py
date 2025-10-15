import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import os

# --- Configuration for plotting ---
BAR_WIDTH = 0.35 # Width of the bars
OFFSET = BAR_WIDTH / 2 # Offset to center the group
# ----------------------------------

try:
    df_ckpt = pd.read_csv("MVM_CKPT/ckpt_test_results.csv")
    df_mvm = pd.read_csv("MVM/mvm_test_results.csv")
except FileNotFoundError as e:
    print(f"Error loading CSV file: {e}")
    exit()

group_cols_ckpt = ['size', 'cache_flush', 'mod', 'ops']
unique_groups_ckpt = df_ckpt.groupby(group_cols_ckpt).groups.keys()

if not os.path.exists("graphs_combined"):
    os.makedirs("graphs_combined")

if not os.path.exists("graphs_combined/cf_enabled"):
    os.makedirs("graphs_combined/cf_enabled")

if not os.path.exists("graphs_combined/cf_disabled"):
    os.makedirs("graphs_combined/cf_disabled")

for group_key in unique_groups_ckpt:
    size, cache_flush, mod, ops = group_key

    group_df_ckpt = df_ckpt[(df_ckpt['size'] == size) &
                            (df_ckpt['cache_flush'] == cache_flush) &
                            (df_ckpt['mod'] == mod) &
                            (df_ckpt['ops'] == ops)].copy()

    group_df_ckpt = group_df_ckpt.sort_values('writes')

    ckpt_time = group_df_ckpt['ckpt_time'].values
    restore_time = group_df_ckpt['restore_time'].values

    group_df_mvm = pd.DataFrame() # Initialize an empty DataFrame
    # Filter MVM data based on matching 'cache_flush' and 'ops'
    # Note: MVM data doesn't have 'size' or 'mod'
    group_df_mvm = df_mvm[(df_mvm['cache_flush'] == cache_flush) &
                            (df_mvm['ops'] == ops)].copy()
    
    # Ensure MVM data is also sorted by 'writes'
    group_df_mvm = group_df_mvm.sort_values('writes')
    
    # Merge the two dataframes on 'writes' to align the plots
    # 'how='left'' ensures we only consider 'writes' values present in the CKPT data
    merged_df = pd.merge(group_df_ckpt, group_df_mvm, how='left', 
    left_on=["cache_flush", "ops", "writes", "reads"], right_on=["cache_flush", "ops", "writes", "reads"], suffixes=('_ckpt', '_mvm'))
    merged_df.to_csv("combined_test_results.csv", mode="a", header=False, index=False)
    
    # Only use the writes that are common to both (or NaN for missing MVM)
    writes = merged_df['writes'].values
    ckpt_time = merged_df['ckpt_time'].values
    restore_time = merged_df['restore_time'].values
    mvm_time = merged_df['time'].values # This will have NaN for non-matching writes in MVM

    fig, ax = plt.subplots(figsize=(12, 6))

    x = np.arange(len(writes))

    mvm_time_plot = np.nan_to_num(mvm_time)
    ax.bar(x - OFFSET, mvm_time_plot, BAR_WIDTH, label='mvm_time', color='#2ca02c')

    # 2. Plot the MVM_CKPT stacked bar (shifted right by OFFSET)
    # ckpt_time bottom part
    ax.bar(x + OFFSET, ckpt_time, BAR_WIDTH, label='ckpt_time', color='#1f77b4')
    # restore_time top part
    ax.bar(x + OFFSET, restore_time, BAR_WIDTH, bottom=ckpt_time, label='restore_time', color='#ff7f0e')

    ax.set_xticks(x)
    ax.set_xticklabels(writes.astype(str))

    ax.set_xlabel('Writes (Number of Write Operations)')
    ax.set_ylabel('Time (s)')

    title = (f"Time Comparison (MVM vs MVM_CKPT) for (size={size}, cache_flush={cache_flush}, "
             f"mod={mod}, ops={ops})")
    ax.set_title(title, wrap=True)

    ax.legend()

    # Rotate x-axis labels for better readability
    plt.xticks(rotation=45, ha='right')

    # Adjust layout to prevent labels from being cut off
    plt.tight_layout()

    if cache_flush == 0:
        filename = (f"graphs_combined/cf_disabled/combined_bar_size_{size}_mod_{mod}_ops_{ops}.png")
    else:
        filename = (f"graphs_combined/cf_enabled/combined_bar_size_{size}_mod_{mod}_ops_{ops}.png")

    plt.savefig(filename)
    plt.close(fig)

print("Combined graphs generated successfully in 'graphs_combined' directory.")
