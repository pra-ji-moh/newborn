#!/bin/sh
# Counting models only (lang.c), same data as run_lang.sh; the nets'
# numbers for the same data are in bench/results/lang_sweep.txt.
set -e
cd "$(dirname "$0")"
cc -std=c99 -O2 lang.c -o lang -lm
OUT=../bench/results/lang_grouped.txt
W=${TMPDIR:-/tmp}/lang_grouped; mkdir -p "$W"
: > "$OUT"
for L in 2 3 1; do for P in 0 5; do for N in 300 1000 3000 10000 30000; do
    echo "=== LANG $L NOISE $P N $N" >> "$OUT"
    ./lang $L $P $N "$W" >> "$OUT"
done; done; done
