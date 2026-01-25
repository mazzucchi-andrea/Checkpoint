set datafile separator ","
# Extract values from the second line of the CSV (skipping the header)
size = system("tail -n +2 plot_data.csv | head -n 1 | cut -d',' -f1")
cache_flush = system("tail -n +2 plot_data.csv | head -n 1 | cut -d',' -f2")
ops = system("tail -n +2 plot_data.csv | head -n 1 | cut -d',' -f3")

outfile = sprintf("plots/mod_comparison/ckpt/ckpt_mod_comparison_size_%s_cf_%s_ops_%s.png", size, cache_flush, ops)

set terminal png size 2400,1200 font 'Arial,16'
set output outfile

# Set the title for the plot
set title sprintf("Checkpoint Performance Comparison for Size: %s, Cache Flush: %s, Ops: %s", size, cache_flush, ops) font 'Arial,16'

set xlabel "Number of Writes" font 'Arial,14'
set ylabel "Time (seconds)" font 'Arial,14'
set yrange [0:*]

set key outside right top

## Define colors 
set linetype 1 lc rgb "#1f77b4"  # MOD  8 bytes

set linetype 2 lc rgb "#ff7f0e"  # MOD 16 bytes

set linetype 3 lc rgb "#2ca02c"  # MOD 32 bytes

set linetype 4 lc rgb "#d62728"  # MOD 64 bytes

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
plot 'plot_data.csv' using ($4-1.5*(step/5)):6:7 skip 1 with boxerrorbars lt 1 title "8 Byte Grid Elem", \
     '' using ($4-0.5*(step/5)):10:11 skip 1 with boxerrorbars lt 2 title "16 Byte Grid Elem", \
     '' using ($4+0.5*(step/5)):14:15 skip 1 with boxerrorbars lt 3 title "32 Byte Grid Elem", \
     '' using ($4+1.5*(step/5)):18:19 skip 1 with boxerrorbars lt 4 title "64 Byte Grid Elem"

unset output

outfile = sprintf("plots/mod_comparison/restore/restore_mod_comparison_size_%s_cf_%s_ops_%s.png", size, cache_flush, ops)

set terminal png size 2400,1200 font 'Arial,16'
set output outfile

# Set the title for the plot
set title sprintf("Restore Performance Comparison for Size: %s, Cache Flush: %s, Ops: %s", size, cache_flush, ops) font 'Arial,16'

set xlabel "Number of Writes" font 'Arial,14'
set ylabel "Time (seconds)" font 'Arial,14'
set yrange [0:*]

set key outside right top

## Define colors 
set linetype 1 lc rgb "#1f77b4"  # MOD  8 bytes

set linetype 2 lc rgb "#ff7f0e"  # MOD 16 bytes

set linetype 3 lc rgb "#2ca02c"  # MOD 32 bytes

set linetype 4 lc rgb "#d62728"  # MOD 64 bytes

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
plot 'plot_data.csv' using ($4-1.5*(step/5)):8:9 skip 1 with boxerrorbars lt 1 title "8 Byte Grid Elem", \
     '' using ($4-0.5*(step/5)):12:13 skip 1 with boxerrorbars lt 2 title "16 Byte Grid Elem", \
     '' using ($4+0.5*(step/5)):16:17 skip 1 with boxerrorbars lt 3 title "32 Byte Grid Elem", \
     '' using ($4+1.5*(step/5)):20:21 skip 1 with boxerrorbars lt 4 title "64 Byte Grid Elem"