#!/bin/bash

declare -a ops=(1000 10000 100000 1000000)
declare -a size=(0x100000UL 0x200000UL 0x400000UL)
declare -a writes=(0.95 0.90 0.85 0.80 0.75 0.70 0.65 0.60 0.55 0.50 0.45 0.40 0.35 0.30)

for mod in 64 128 256 512;
do
    for s in ${size[@]};
    do
        for o in ${ops[@]}; 
        do
            for w in ${writes[@]}; 
            do
                w_ops=$(echo "$o * $w" | bc)
                w_ops=${w_ops%.*}
                r_ops=$(echo "$o - $w_ops" | bc)
                make tests SIZE=$s WRITES=$w_ops READS=$r_ops MOD=$mod
            done
        done
    done
done

make clean