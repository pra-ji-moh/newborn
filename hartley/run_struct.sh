#!/bin/sh
# Tests 7-10: long-range memory, nesting, facts and two-step reasoning,
# and writing (sentences drawn from models trained on the Penn Treebank).
# Needs PTB_DIR. Output: bench/results/struct_sweep.txt
set -e
cd "$(dirname "$0")"
cc -std=c99 -O2 real.c -o real -lm
W=${TMPDIR:-/tmp}/struct; OUT=../bench/results/struct_sweep.txt
python3 gen_struct.py "$W"
: > "$OUT"
for T in memory nesting; do for N in 1000 3000 10000; do
    echo "=== $T $N" >> "$OUT"
    ./real "$W/$T" train_$N.txt task >> "$OUT"
    python3 real_nets.py "$W/$T" train_$N.txt 150 task >> "$OUT"
done; done
echo "=== facts" >> "$OUT"
./real "$W/facts" train_all.txt task >> "$OUT"
python3 real_nets.py "$W/facts" train_all.txt 150 task >> "$OUT"
echo "=== writing (Penn Treebank, all 42,068 sentences)" >> "$OUT"
python3 real_prep.py "$PTB_DIR" "$W/ptb" > /dev/null
./real "$W/ptb" train_clean_42068.txt sample >> "$OUT"
python3 real_nets.py "$W/ptb" train_clean_42068.txt 600 sample >> "$OUT"
