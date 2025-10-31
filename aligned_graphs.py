import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import os

BAR_WIDTH = 0.25
GAP = 0.5  # Gap between groups of bars

try:
    df = pd.read_csv("MVM_CKPT/ckpt_aligned_comparison_test_results.csv")
except FileNotFoundError as e:
    print(f"Error loading CSV file: {e}")
    exit()

group_cols_ckpt = ["size", "cache_flush", "ops"]
unique_groups_ckpt = df.groupby(group_cols_ckpt).groups.keys()

if not os.path.exists("graphs"):
    os.makedirs("graphs")
if not os.path.exists("graphs/aligned_compare"):
    os.makedirs("graphs/aligned_compare")
if not os.path.exists("graphs/aligned_compare/cf_disabled"):
    os.makedirs("graphs/aligned_compare/cf_disabled")
if not os.path.exists("graphs/aligned_compare/cf_enabled"):
    os.makedirs("graphs/aligned_compare/cf_enabled")


for group_key in unique_groups_ckpt:
    size, cache_flush, ops = group_key
    group_df = df[
        (df["size"] == size) & (df["cache_flush"] == cache_flush) & (df["ops"] == ops)
    ].copy()
    group_df = group_df.sort_values("writes")
    writes = group_df["writes"].unique()

    ckpt_not_aligned_time = group_df["ckpt_not_aligned_time"].values
    restore_not_aligned_time = group_df["restore_not_aligned_time"].values
    ckpt_aligned_time = group_df["ckpt_aligned_time"].values
    restore_aligned_time = group_df["restore_aligned_time"].values

    fig, ax = plt.subplots(figsize=(12, 6))

    x = np.arange(len(writes)) * (4 * BAR_WIDTH + GAP)  # Adjust x positions for gaps

    # Plot bars for each group
    ax.bar(
        x - BAR_WIDTH,
        ckpt_not_aligned_time,
        BAR_WIDTH,
        label="ckpt_not_aligned_time",
        color="#1f77b4",
    )
    ax.bar(
        x - BAR_WIDTH,
        restore_not_aligned_time,
        BAR_WIDTH,
        bottom=ckpt_not_aligned_time,
        label="restore_not_aligned_time",
        color="#ff7f0e",
    )

    ax.bar(x, ckpt_aligned_time, BAR_WIDTH, label="ckpt_aligned_time", color="#d62728")
    ax.bar(
        x,
        restore_aligned_time,
        BAR_WIDTH,
        bottom=ckpt_aligned_time,
        label="restore_aligned_time",
        color="#2ca02c",
    )

    ax.set_xticks(x)
    ax.set_xticklabels(writes.astype(str))
    ax.set_xlabel("Writes (Number of Write Operations)")
    ax.set_ylabel("Time (s)")
    title = f"Time Comparison for (size={size}, cache_flush={cache_flush}, ops={ops})"
    ax.set_title(title, wrap=True)
    ax.legend(loc="center left", bbox_to_anchor=(1.05, 0.5))

    plt.xticks(rotation=45, ha="right")
    plt.tight_layout()

    if cache_flush == 0:
        filename = f"graphs/aligned_compare/cf_disabled/aligning_compare_graph_cf_{cache_flush}_size_{size}_ops_{ops}.png"
    else:
        filename = f"graphs/aligned_compare/cf_enabled/aligning_compare_graph_cf_{cache_flush}_size_{size}_ops_{ops}.png"

    plt.savefig(filename, dpi=200, bbox_inches="tight")
    plt.close(fig)

print(
    "aligning comparing graphs generated successfully in 'graphs/aligned_compare' directory."
)
