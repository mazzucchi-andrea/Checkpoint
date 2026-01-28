#!/bin/bash

declare -a cache_flush=(0 1)
declare -a ops=(1000 10000 100000 1000000)
declare -a size=(0x100000 0x200000 0x400000)
declare -a mods=(8 16 32 64)
declare -a chunks=(32 64 128 256 512 1024 2048 4096)
declare -a writes=(0.95 0.90 0.85 0.80 0.75 0.70 0.65 0.60 0.55 0.50 0.45 0.40 0.35 0.30)

# --- Error Checking ---
if ! command -v bc &> /dev/null
then
    echo "Error: bc could not be found."
    echo "Please install bc to run this script."
    exit 1
fi

# Tests with MVM_GRID_CKPT_BS
cd MVM_GRID_CKPT_BS
rm ckpt_test_results.csv
rm ckpt_repeat_test_results.csv
echo "size,cache_flush,mod,ops,writes,reads,ckpt_time,ckpt_ci,restore_time,restore_ci" > ckpt_test_results.csv
echo "size,cache_flush,mod,ops,writes,reads,repetitions,ckpt_time,ckpt_ci,restore_time,restore_ci" > ckpt_repeat_test_results.csv

for mod in ${mods[@]};
do
    for s in ${size[@]};
    do
        for cf in ${cache_flush[@]};
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

make clean
cd ..

# Tests with MVM chunk patch
cd MVM_CHUNK_SET
rm chunk_test_results.csv
rm chunk_repeat_test_results.csv
echo "size,cache_flush,chunk,ops,writes,reads,ckpt_time,ckpt_ci,restore_time,restore_ci" > chunk_test_results.csv
echo "size,cache_flush,chunk,ops,writes,reads,repetitions,ckpt_time,ckpt_ci,restore_time,restore_ci" > chunk_repeat_test_results.csv

for chunk in ${chunks[@]};
do
    for s in ${size[@]};
    do
        for cf in ${cache_flush[@]};
        do
            for o in ${ops[@]}; 
            do
                for w in ${writes[@]}; 
                do
                    w_ops=$(echo "$o * $w" | bc)
                    w_ops=${w_ops%.*}
                    r_ops=$(echo "$o - $w_ops" | bc)
                    make ALLOCATOR_AREA_SIZE=$s WRITES=$w_ops READS=$r_ops CF=$cf CHUNK=$chunk
                    ./application/prog
                done
            done
        done
    done
done

make clean
cd ..

sh generate_plots.sh