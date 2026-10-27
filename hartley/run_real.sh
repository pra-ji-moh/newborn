#!/bin/sh
# Real language (Penn Treebank): grouping and trigram (real.c) against an
# LSTM and a transformer (real_nets.py), clean and web-like noisy training
# data, three sizes, clean test. Needs PTB_DIR with ptb.{train,valid,test}.txt
# Output: bench/results/real_sweep.txt
set -e
cd "$(dirname "$0")"
cc -std=c99 -O2 real.c -o real -lm
W=${TMPDIR:-/tmp}/real; OUT=../bench/results/real_sweep.txt
python3 real_prep.py "$PTB_DIR" "$W"
: > "$OUT"
for N in 2000 10000 42068; do for K in clean noisy; do
    T=train_${K}_$N.txt
    case $N in 2000) S=180;; 10000) S=360;; *) S=900;; esac
    echo "=== $T" >> "$OUT"
    ./real "$W" $T >> "$OUT" &
    python3 real_nets.py "$W" $T $S >> "$OUT.nets.$N.$K"
    wait
    cat "$OUT.nets.$N.$K" >> "$OUT"; rm "$OUT.nets.$N.$K"
done; done
