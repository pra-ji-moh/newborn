#!/bin/sh
# Long text: the relational model (longtext.c) with its controls, against a
# transformer (1,024-word window) and an LSTM (long_nets.py). Needs PTB_DIR.
# Output: bench/results/long_sweep.txt
set -e
cd "$(dirname "$0")"
cc -std=c99 -O2 longtext.c -o longtext -lm
W=${TMPDIR:-/tmp}/long; OUT=../bench/results/long_sweep.txt
python3 gen_long.py "$PTB_DIR" "$W" > /dev/null
: > "$OUT"
for c in "" trigram abelian oldest; do ./longtext "$W" $c >> "$OUT"; done
python3 long_nets.py "$W" ${NET_SECS:-1200} >> "$OUT"
