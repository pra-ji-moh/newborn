#!/bin/sh
# The full sweep: counting models (lang.c) and gradient nets (lang_nets.py)
# on the same files. Output: bench/results/lang_sweep.txt
set -e
cd "$(dirname "$0")"
cc -std=c99 -O2 lang.c -o lang -lm
OUT=../bench/results/lang_sweep.txt
W=${TMPDIR:-/tmp}/lang_sweep; mkdir -p "$W"
: > "$OUT"
for L in 2 1 3; do for P in 0 5; do for N in 300 1000 3000 10000 30000; do
    echo "=== LANG $L NOISE $P N $N" >> "$OUT"
    ./lang $L $P $N "$W" >> "$OUT"
    python3 lang_nets.py "$W" 90 2>/dev/null >> "$OUT"
done; done; done
