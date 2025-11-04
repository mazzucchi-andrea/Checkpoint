#!/bin/bash

echo "size,cache_flush,mod,ops,writes,reads,ckpt_time,restore_time, time" > combined_test_results.csv
python -m venv .venv
source .venv/bin/activate
pip install --upgrade pip
pip install -r requirements.txt
rm combined_test_results.csv
python graphs.py
python mod_graphs.py