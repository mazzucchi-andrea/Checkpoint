import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import os

BAR_WIDTH = 0.3

try:
    df_ckpt = pd.read_csv("MVM_CKPT/ckpt_test_results.csv")
    df_mvm = pd.read_csv("MVM/mvm_test_results.csv")
    df_simple = pd.read_csv("SIMPLE_CKPT/simple_ckpt_test_results.csv")
except FileNotFoundError as e:
    print(f"Error loading CSV file: {e}")
    exit()

group_cols_ckpt = ["size", "cache_flush", "mod", "ops"]
unique_groups_ckpt = df_ckpt.groupby(group_cols_ckpt).groups.keys()

if not os.path.exists("graphs"):
    os.makedirs("graphs")
if not os.path.exists("graphs/ckpt_compare"):
    os.makedirs("graphs/ckpt_compare")
if not os.path.exists("graphs/ckpt_compare/cf_enabled"):
    os.makedirs("graphs/ckpt_compare/cf_enabled")
if not os.path.exists("graphs/ckpt_compare/cf_disabled"):
    os.makedirs("graphs/ckpt_compare/cf_disabled")

combined_csv = open("combined_test_results.csv", "w")
combined_csv.write(
    "size,cache_flush,ckpt_mod,ops,writes,reads,ckpt_time,ckpt_restore_time,mvm_time,simple_ckpt_time,simple_restore_time\n"
)
combined_csv.close()

for group_key in unique_groups_ckpt:
    size, cache_flush, mod, ops = group_key

    group_df_ckpt = df_ckpt[
        (df_ckpt["size"] == size)
        & (df_ckpt["cache_flush"] == cache_flush)
        & (df_ckpt["mod"] == mod)
        & (df_ckpt["ops"] == ops)
    ].copy()

    group_df_ckpt = group_df_ckpt.sort_values("writes")

    ckpt_time = group_df_ckpt["ckpt_time"].values
    restore_time = group_df_ckpt["restore_time"].values

    group_df_mvm = pd.DataFrame()  # Initialize an empty DataFrame
    # Filter MVM data based on matching 'cache_flush' and 'ops'
    # Note: MVM data doesn't have 'mod'
    group_df_mvm = df_mvm[
        (df_mvm["size"] == size)
        & (df_mvm["cache_flush"] == cache_flush)
        & (df_mvm["ops"] == ops)
    ].copy()

    # Ensure MVM data is also sorted by 'writes'
    group_df_mvm = group_df_mvm.sort_values("writes")

    group_df_simple = df_simple[
        (df_simple["size"] == size)
        & (df_simple["cache_flush"] == cache_flush)
        & (df_simple["ops"] == ops)
    ].copy()

    group_df_simple = group_df_simple.sort_values("writes")

    merged_df = pd.merge(
        group_df_ckpt,
        group_df_mvm,
        how="left",
        left_on=["size", "cache_flush", "ops", "writes", "reads"],
        right_on=["size", "cache_flush", "ops", "writes", "reads"],
    )

    merged_df = pd.merge(
        merged_df,
        group_df_simple,
        how="left",
        left_on=["size", "cache_flush", "ops", "writes", "reads"],
        right_on=["size", "cache_flush", "ops", "writes", "reads"],
    )
    merged_df.to_csv("combined_test_results.csv", mode="a", header=False, index=False)

    writes = merged_df["writes"].values
    ckpt_time = merged_df["ckpt_time"].values
    restore_time = merged_df["restore_time"].values
    mvm_time = merged_df["time"].values
    simple_ckpt_time = merged_df["simple_ckpt_time"].values
    simple_restore_time = merged_df["simple_restore_time"].values

    fig, ax = plt.subplots(figsize=(12, 6))

    x = np.arange(len(writes))

    mvm_time_plot = np.nan_to_num(mvm_time)
    ax.bar(x - BAR_WIDTH, mvm_time_plot, BAR_WIDTH, label="mvm_time", color="#2ca02c")

    # ckpt_time bottom part
    ax.bar(x, ckpt_time, BAR_WIDTH, label="ckpt_time", color="#1f77b4")
    # restore_time top part
    ax.bar(
        x,
        restore_time,
        BAR_WIDTH,
        bottom=ckpt_time,
        label="restore_time",
        color="#ff7f0e",
    )

    # simple_ckpt_time bottom part
    ax.bar(
        x + BAR_WIDTH,
        simple_ckpt_time,
        BAR_WIDTH,
        label="simple_ckpt_time",
        color="mediumpurple",
    )

    # simple_restore_time top part
    ax.bar(
        x + BAR_WIDTH,
        simple_restore_time,
        BAR_WIDTH,
        bottom=simple_ckpt_time,
        label="simple_restore_time",
        color="gold",
    )

    ax.set_xticks(x)
    ax.set_xticklabels(writes.astype(str))

    ax.set_xlabel("Writes (Number of Write Operations)")
    ax.set_ylabel("Time (s)")

    title = (
        f"Time Comparison for (size={size}, cache_flush={cache_flush}, "
        f"mod={mod}, ops={ops})"
    )
    ax.set_title(title, wrap=True)

    ax.legend(loc="center left", bbox_to_anchor=(1.05, 0.5))

    # Rotate x-axis labels for better readability
    plt.xticks(rotation=45, ha="right")

    # Adjust layout to prevent labels from being cut off
    plt.tight_layout()

    if cache_flush == 0:
        filename = (
            f"graphs/ckpt_compare/cf_disabled/combined_bar_size_{size}_mod_{mod}_ops_{ops}.png"
        )
    else:
        filename = f"graphs/ckpt_compare/cf_enabled/combined_bar_size_{size}_mod_{mod}_ops_{ops}.png"

    plt.savefig(filename)
    plt.close(fig)

print("ckpt cpmparing graphs generated successfully in 'graphs' directory.")
