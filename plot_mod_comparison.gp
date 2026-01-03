set datafile separator ","
# Extract values from the second line of the CSV (skipping the header)
size = system("tail -n +2 plot_data.csv | head -n 1 | cut -d',' -f1")
cache_flush = system("tail -n +2 plot_data.csv | head -n 1 | cut -d',' -f2")
ops = system("tail -n +2 plot_data.csv | head -n 1 | cut -d',' -f3")

if (cache_flush == 0) {
     outfile = sprintf("plots/mod_comparison/cf_disabled/mod_comparison_size_%s_cf_%s_ops_%s.png", size, cache_flush, ops)
} else {
     outfile = sprintf("plots/mod_comparison/cf_enabled/mod_comparison_size_%s_cf_%s_ops_%s.png", size, cache_flush, ops)
}

set terminal png size 2400,1200 font 'Arial,16'
set output outfile

# Set the title for the plot
set title sprintf("Performance Comparison for Size: %s, Cache Flush: %s, Ops: %s", size, cache_flush, ops) font 'Arial,16'

set xlabel "Number of Writes" font 'Arial,14'
set ylabel "Time (seconds)" font 'Arial,14'
set yrange [0:*]

set key outside right top

# Define colors
set linetype 1 lc rgb "#00347d"  # 8_ckpt_time
set linetype 2 lc rgb "#008dff"  # 8_ckpt_restore_time
set linetype 3 lc rgb "#ff73bc"  # 16_ckpt_time
set linetype 4 lc rgb "#c701ff"  # 16_ckpt_restore_time
set linetype 5 lc rgb "#4ecb8d"  # 32_ckpt_time
set linetype 6 lc rgb "#ff9d3a"  # 32_ckpt_restore_time
set linetype 7 lc rgb "#f9e858"  # 64_ckpt_time
set linetype 8 lc rgb "#d83034"  # 64_ckpt_restore_time


set style fill solid border -1
set grid ytics linestyle 0 linecolor rgb "#E0E0E0"

# Get statistics from the writes column (column 4)
stats 'plot_data.csv' using 4 skip 1 nooutput
# Calculate step as the difference between max and min divided by (number of points - 1)
step = (STATS_max - STATS_min) / (STATS_records - 1)
set xrange [(STATS_min - step):(STATS_max + step)]
set xtics step
set xtics rotate by -45

set boxwidth step/5

# Create three groups of bars with proper stacking
plot 'plot_data.csv' using ($4-1.5*(step/5)):($6+$7) skip 1 with boxes lt 2 title "8 ckpt restore time", \
     '' using ($4-1.5*(step/5)):($6) skip 1 with boxes lt 1 title "8 ckpt time", \
     '' using ($4-0.5*(step/5)):($8+$9) skip 1 with boxes lt 4 title "16 ckpt restore time", \
     '' using ($4-0.5*(step/5)):($8) skip 1 with boxes lt 3 title "16 ckpt time", \
     '' using ($4+0.5*(step/5)):($10+$11) skip 1 with boxes lt 6 title "32 ckpt restore time", \
     '' using ($4+0.5*(step/5)):($10) skip 1 with boxes lt 5 title "32 ckpt time", \
     '' using ($4+1.5*(step/5)):($12+$13) skip 1 with boxes lt 8 title "64 ckpt restore time", \
     '' using ($4+1.5*(step/5)):($12) skip 1 with boxes lt 7 title "64 ckpt time"