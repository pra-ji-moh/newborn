#!/bin/sh
# The fair long-text comparison: the nets get their best chance - short
# training documents (fact and question close together), a 256-word window
# and a small, fast transformer so it trains for thousands of steps, and 30
# minutes each. Testing is unchanged (50 to 8,000 words). Needs PTB_DIR.
# Output: bench/results/long_fair.txt
set -e
cd "$(dirname "$0")"
cc -std=c99 -O2 longtext.c -o longtext -lm
W=${TMPDIR:-/tmp}/long; OUT=../bench/results/long_fair.txt
python3 gen_long.py "$PTB_DIR" "$W" > /dev/null
./longtext "$W" > "$OUT"
python3 long_nets.py "$W" ${NET_SECS:-1800} 256 train_short.txt 128 2 >> "$OUT"
