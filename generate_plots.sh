#!/bin/bash

declare -a cache_flush=(0)
declare -a ops=(1000)
declare -a size=(0x100000)
declare -a mods=(8 16 32 64)
declare -a reps=(2 4 6 8 10)

DATAFILE="plot_data.csv"
PLOT_CKPT_COMPARE="plot_ckpt_comparison.gp"
PLOT_MOD_COMPARE="plot_mod_comparison.gp"
PLOT_REP_COMPARE="plot_rep_comparison.gp"

rm -r plots
mkdir plots
mkdir plots/ckpt_comparison
mkdir plots/ckpt_comparison/ckpt
mkdir plots/ckpt_comparison/restore
mkdir plots/mod_comparison
mkdir plots/mod_comparison/ckpt
mkdir plots/mod_comparison/restore
mkdir plots/rep_comparison
mkdir plots/rep_comparison/ckpt
mkdir plots/rep_comparison/restore

# --- Error Checking ---
if ! command -v gnuplot &> /dev/null
then
    echo "Error: Gnuplot could not be found."
    echo "Please install gnuplot to run this script."
    exit 1
fi

if [ ! -f "$PLOT_CKPT_COMPARE" ]; then
    echo "Error: Gnuplot template '$PLOT_CKPT_COMPARE' not found!"
    echo "Please make sure it's in the same directory as this script."
    exit 1
fi

if [ ! -f "$PLOT_MOD_COMPARE" ]; then
    echo "Error: Gnuplot template '$PLOT_MOD_COMPARE' not found!"
    echo "Please make sure it's in the same directory as this script."
    exit 1
fi

if [ ! -f "$PLOT_REP_COMPARE" ]; then
    echo "Error: Gnuplot template '$PLOT_REP_COMPARE' not found!"
    echo "Please make sure it's in the same directory as this script."
    exit 1
fi

echo "Found $PLOT_CKPT_COMPARE."
echo "Found $PLOT_MOD_COMPARE."
echo "Found $PLOT_REP_COMPARE."
echo "Generating plots for all unique combinations..."

gcc get_ckpt_data.c -o get_plot_data

for mod in ${mods[@]};
do
    for s in ${size[@]};
    do
        for cf in ${cache_flush[@]};
        do
            for o in ${ops[@]}; 
            do
                ./get_plot_data $s $cf $mod $o
                gnuplot $PLOT_CKPT_COMPARE
            done
        done
    done
done

echo "All ckpt comparison plots generated."


gcc get_mod_data.c -o get_plot_data


for s in ${size[@]};
do
    for cf in ${cache_flush[@]};
    do
        for o in ${ops[@]}; 
        do
            ./get_plot_data $s $cf $o
            gnuplot $PLOT_MOD_COMPARE
        done
    done
done

echo "All mod comparison plots generated."


gcc get_rep_data.c -o get_plot_data

for r in ${reps[@]};
do
    for mod in ${mods[@]};
    do
        for s in ${size[@]};
        do
            for cf in ${cache_flush[@]};
            do
                for o in ${ops[@]}; 
                do
                    ./get_plot_data $s $cf $mod $o $r
                    gnuplot $PLOT_REP_COMPARE
                done
            done
        done
    done
done

echo "All rep comparison plots generated."


rm get_plot_data
rm plot_data.csv

echo "All plots generated."

