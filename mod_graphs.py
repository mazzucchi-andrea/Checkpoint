import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import os

BAR_WIDTH = 0.25
GAP = 0.5  # Gap between groups of bars

try:
    df = pd.read_csv("MVM_CKPT/ckpt_test_results.csv")
except FileNotFoundError as e:
    print(f"Error loading CSV file: {e}")
    exit()

group_cols_ckpt = ["size", "cache_flush", "ops"]
unique_groups_ckpt = df.groupby(group_cols_ckpt).groups.keys()

if not os.path.exists("graphs"):
    os.makedirs("graphs")
if not os.path.exists("graphs/mods_compare"):
    os.makedirs("graphs/mods_compare")
if not os.path.exists("graphs/mods_compare/cf_disabled"):
    os.makedirs("graphs/mods_compare/cf_disabled")
if not os.path.exists("graphs/mods_compare/cf_enabled"):
    os.makedirs("graphs/mods_compare/cf_enabled")

# Define colors for each group
colors = {
    64:  {"ckpt": "#1f77b4", "restore": "#e377c2"},  # Blue -> Pink
    128: {"ckpt": "#ff7f0e", "restore": "#7f7f7f"},  # Orange -> Gray
    256: {"ckpt": "#2ca02c", "restore": "#bcbd22"},  # Green -> Olive
    512: {"ckpt": "#d62728", "restore": "#17becf"},  # Red -> Cyan
}


for group_key in unique_groups_ckpt:
    size, cache_flush, ops = group_key
    group_df = df[
        (df["size"] == size) & (df["cache_flush"] == cache_flush) & (df["ops"] == ops)
    ].copy()
    group_df = group_df.sort_values("writes")
    writes = group_df["writes"].unique()

    mod64 = group_df[group_df["mod"] == 64]
    mod128 = group_df[group_df["mod"] == 128]
    mod256 = group_df[group_df["mod"] == 256]
    mod512 = group_df[group_df["mod"] == 512]

    ckpt_time_64 = mod64["ckpt_time"].values
    restore_time_64 = mod64["restore_time"].values
    ckpt_time_128 = mod128["ckpt_time"].values
    restore_time_128 = mod128["restore_time"].values
    ckpt_time_256 = mod256["ckpt_time"].values
    restore_time_256 = mod256["restore_time"].values
    ckpt_time_512 = mod512["ckpt_time"].values
    restore_time_512 = mod512["restore_time"].values

    fig, ax = plt.subplots(figsize=(12, 6))

    x = np.arange(len(writes)) * (4 * BAR_WIDTH + GAP)  # Adjust x positions for gaps

    # Plot bars for each group
    ax.bar(
        x - 1.5 * BAR_WIDTH,
        ckpt_time_64,
        BAR_WIDTH,
        label="64_ckpt_time",
        color=colors[64]["ckpt"],
    )
    ax.bar(
        x - 1.5 * BAR_WIDTH,
        restore_time_64,
        BAR_WIDTH,
        bottom=ckpt_time_64,
        label="64_restore_time",
        color=colors[64]["restore"],
    )

    ax.bar(
        x - 0.5 * BAR_WIDTH,
        ckpt_time_128,
        BAR_WIDTH,
        label="128_ckpt_time",
        color=colors[128]["ckpt"],
    )
    ax.bar(
        x - 0.5 * BAR_WIDTH,
        restore_time_128,
        BAR_WIDTH,
        bottom=ckpt_time_128,
        label="128_restore_time",
        color=colors[128]["restore"],
    )

    ax.bar(
        x + 0.5 * BAR_WIDTH,
        ckpt_time_256,
        BAR_WIDTH,
        label="256_ckpt_time",
        color=colors[256]["ckpt"],
    )
    ax.bar(
        x + 0.5 * BAR_WIDTH,
        restore_time_256,
        BAR_WIDTH,
        bottom=ckpt_time_256,
        label="256_restore_time",
        color=colors[256]["restore"],
    )

    ax.bar(
        x + 1.5 * BAR_WIDTH,
        ckpt_time_512,
        BAR_WIDTH,
        label="512_ckpt_time",
        color=colors[512]["ckpt"],
    )
    ax.bar(
        x + 1.5 * BAR_WIDTH,
        restore_time_512,
        BAR_WIDTH,
        bottom=ckpt_time_512,
        label="512_restore_time",
        color=colors[512]["restore"],
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
        filename = f"graphs/mods_compare/cf_disabled/combined_bar_size_{size}_ops_{ops}.png"
    else:
        filename = f"graphs/mods_compare/cf_enabled/combined_bar_size_{size}_ops_{ops}.png"

    plt.savefig(filename, dpi=200, bbox_inches="tight")
    plt.close(fig)

print("mod comparing graphs generated successfully in 'graphs/mods_compare' directory.")
