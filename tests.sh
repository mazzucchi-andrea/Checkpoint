#!/bin/bash

declare -a cache_flush=(0 1)
declare -a ops=(1000 10000 100000 1000000)
declare -a size=(0x100000UL 0x200000UL 0x400000UL)
declare -a writes=(0.95 0.90 0.85 0.80 0.75 0.70 0.65 0.60 0.55 0.50 0.45 0.40 0.35 0.30)

rm -r graphs

# Tests with MVM example patch
cd MVM
rm mvm_test_results.csv
echo "size,cache_flush,ops,writes,reads,time" > mvm_test_results.csv

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
                make MEM_SIZE=$s WRITES=$w_ops READS=$r_ops CF=$cf
                ./application/prog
            done
        done
    done
done

make clean
cd ..

# Tests with MVM_CKPT
cd MVM_CKPT
rm ckpt_test_results.csv
rm ckpt_repeat_test_results.csv
echo "size,cache_flush,mod,ops,writes,reads,ckpt_time,restore_time" > ckpt_test_results.csv
echo "size,cache_flush,mod,ops,writes,reads,repetitions,ckpt_time,restore_time" > ckpt_repeat_test_results.csv

for mod in 64 128 256 512;
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

# Tests with memcpy as checkpoint and restore mechanism
cd SIMPLE_CKPT
rm simple_ckpt_test_results.csv
rm simple_ckpt_repeat_test_results.csv
echo "size,cache_flush,ops,writes,reads,simple_ckpt_time,simple_restore_time" > simple_ckpt_test_results.csv
echo "size,cache_flush,ops,writes,reads,repetitions,ckpt_time,restore_time" > simple_ckpt_repeat_test_results.csv

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
                make SIZE=$s WRITES=$w_ops READS=$r_ops CF=$cf
                ./a.out
            done
        done
    done
done

make clean
cd ..

sh generate_plots.sh