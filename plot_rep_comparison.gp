set datafile separator ","
# Extract values from the second line of the CSV (skipping the header)
size = system("tail -n +2 plot_data.csv | head -n 1 | cut -d',' -f1")
cache_flush = system("tail -n +2 plot_data.csv | head -n 1 | cut -d',' -f2")
mod = system("tail -n +2 plot_data.csv | head -n 1 | cut -d',' -f3")
ops = system("tail -n +2 plot_data.csv | head -n 1 | cut -d',' -f4")
reps = system("tail -n +2 plot_data.csv | head -n 1 | cut -d',' -f7")

if (cache_flush == 0) {
     outfile = sprintf("plots/rep_comparison/cf_disabled/rep_comparison_size_%s_cf_%s_mod_%s_ops_%s_reps_%s.png", size, cache_flush, mod, ops, reps)
} else {
     outfile = sprintf("plots/rep_comparison/cf_enabled/rep_comparison_size_%s_cf_%s_mod_%s_ops_%s_reps_%s.png", size, cache_flush, mod, ops, reps)
}

set terminal png size 2400,1200 font 'Arial,16'
set output outfile

# Set the title for the plot
set title sprintf("Performance Comparison for Size: %s, Cache Flush: %s, Mod: %s, Ops: %s Reps: %s", size, cache_flush, mod, ops, reps) font 'Arial,16'

set xlabel "Number of Writes" font 'Arial,14'
set ylabel "Time (seconds)" font 'Arial,14'
set yrange [0:*]

set key outside right top

# Define colors
set linetype 1 lc rgb "#000042"  # ckpt_time
set linetype 2 lc rgb "#bc272d"  # ckpt_restore_time
set linetype 3 lc rgb "#e9c716"  # simple_ckpt_time
set linetype 4 lc rgb "#50ad9f"  # simple_restore_time

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
plot 'plot_data.csv' using ($5-0.5*step/5):($8+$9) skip 1 with boxes lt 2 title "ckpt restore time", \
     '' using ($5-0.5*step/5):8 skip 1 with boxes lt 1 title "ckpt time", \
     '' using ($5+0.5*step/5):($10+$11) skip 1 with boxes lt 4 title "simple restore time", \
     '' using ($5+0.5*step/5):10 skip 1 with boxes lt 3 title "simple ckpt time"
     