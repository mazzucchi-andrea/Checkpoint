#!/bin/bash

declare -a ops=(1000 10000 100000 1000000)
declare -a size=(0x100000UL 0x200000UL 0x400000UL)
declare -a writes=(0.95 0.90 0.85 0.80 0.75 0.70 0.65 0.60 0.55 0.50 0.45 0.40 0.35 0.30)

cd MVM_CKPT
echo "size,cache_flush,mod,ops,writes,reads,ckpt_time,restore_time" > test_results.csv

for mod in 64 128 256;
do
    for cf in 0 1;
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
                    make ALLOCATOR_AREA_SIZE=$s WRITES=$w_ops READS=$r_ops CF=$cf MOD=$mod
                    ./application/prog
                done
            done
        done
    done
done

cd ..
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
python graphs.py

