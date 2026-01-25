set datafile separator ","
# Extract values from the second line of the CSV (skipping the header)
size = system("tail -n +2 plot_data.csv | head -n 1 | cut -d',' -f1")
cache_flush = system("tail -n +2 plot_data.csv | head -n 1 | cut -d',' -f2")
mod = system("tail -n +2 plot_data.csv | head -n 1 | cut -d',' -f3")
ops = system("tail -n +2 plot_data.csv | head -n 1 | cut -d',' -f4")

outfile = sprintf("plots/ckpt_comparison/ckpt/ckpt_comparison_size_%s_cf_%s_mod_%s_ops_%s.png", size, cache_flush, mod, ops)

set terminal png size 2400,1200 font 'Arial,16'
set output outfile

# Set the title for the plot
set title sprintf("Checkpoint Performance Comparison for Size: %s, Cache Flush: %s, Mod: %s, Ops: %s", size, cache_flush, mod, ops) font 'Arial,16'

set xlabel "Number of Writes" font 'Arial,14'
set ylabel "Time (seconds)" font 'Arial,14'
set yrange [0:*]

set key outside right top

# Define colors
set linetype 1 lc rgb "#ff7f0e"  # asm_grid_ckpt_bs

set linetype 2 lc rgb "#2ca02c"  # grid_ckpt_bs_c_patch

set linetype 3 lc rgb "#1f77b4"  # simple_ckpt

set style fill solid border -1
set grid ytics linestyle 0 linecolor rgb "#E0E0E0"

# Get statistics from the writes column (column 5)
stats 'plot_data.csv' using 5 skip 1 nooutput
# Calculate step as the difference between max and min divided by (number of points - 1)
step = (STATS_max - STATS_min) / (STATS_records - 1)
set xrange [(STATS_min - step):(STATS_max + step)]
set xtics step
set xtics rotate by -45

set boxwidth step/5

# Create three groups of bars with proper stacking
plot 'plot_data.csv' using ($5-step/5):7:8 skip 1 with boxerrorbars lt 1 title "asm GRID CKPT BS", \
     '' using 5:11:12 skip 1 with boxerrorbars lt 2 title "C Patch GRID CKPT BS", \
     '' using ($5+step/5):15:16 skip 1 with boxerrorbars lt 3 title "SIMPLE CKPT"

unset output

outfile = sprintf("plots/ckpt_comparison/restore/restore_comparison_size_%s_cf_%s_mod_%s_ops_%s.png", size, cache_flush, mod, ops)

set terminal png size 2400,1200 font 'Arial,16'
set output outfile

# Set the title for the plot
set title sprintf("Restore Performance Comparison for Size: %s, Cache Flush: %s, Mod: %s, Ops: %s", size, cache_flush, mod, ops) font 'Arial,16'

set xlabel "Number of Writes" font 'Arial,14'
set ylabel "Time (seconds)" font 'Arial,14'
set yrange [0:*]

set key outside right top

# Define colors
set linetype 1 lc rgb "#ff7f0e"  # asm_grid_ckpt_bs

set linetype 2 lc rgb "#2ca02c"  # grid_ckpt_bs_c_patch

set linetype 3 lc rgb "#1f77b4"  # simple_ckpt

set style fill solid border -1
set grid ytics linestyle 0 linecolor rgb "#E0E0E0"

# Get statistics from the writes column (column 5)
stats 'plot_data.csv' using 5 skip 1 nooutput
# Calculate step as the difference between max and min divided by (number of points - 1)
step = (STATS_max - STATS_min) / (STATS_records - 1)
set xrange [(STATS_min - step):(STATS_max + step)]
set xtics step
set xtics rotate by -45

set boxwidth step/5

# Create three groups of bars with proper stacking
plot 'plot_data.csv' using ($5-step/5):9:10 skip 1 with boxerrorbars lt 1 title "asm GRID CKPT BS", \
     '' using 5:13:14 skip 1 with boxerrorbars lt 2 title "C Patch GRID CKPT BS", \
     '' using ($5+step/5):17:18 skip 1 with boxerrorbars lt 3 title "SIMPLE CKPT"