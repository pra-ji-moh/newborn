#!/bin/sh
# The harder long-text test (gen_long2.py): varied wording, moves, three
# steps, eight people, new wordings. The relational model (longtext.c)
# against a transformer (512-word window) and an LSTM (whole stream), both
# trained on short question-heavy documents for their best chance.
# Needs PTB_DIR. Output: bench/results/long2.txt
set -e
cd "$(dirname "$0")"
cc -std=c99 -O2 longtext.c -o longtext -lm
W=${TMPDIR:-/tmp}/long2; OUT=../bench/results/long2.txt
python3 gen_long2.py "$PTB_DIR" "$W" > /dev/null
./longtext "$W" > "$OUT"
LSTM_SECS=${LSTM_SECS:-1200} python3 long_nets.py "$W" ${NET_SECS:-3600} 512 train_short.txt 192 3 > "$W/nets.txt"
cat "$W/nets.txt" >> "$OUT"
