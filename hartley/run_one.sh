#!/bin/sh
# The one rule (one.py): every earlier test, one model, one pass each.
# Needs PTB_DIR with ptb.{train,valid,test}.txt. Output: bench/results/one.txt
set -e
cd "$(dirname "$0")"
W=${TMPDIR:-/tmp}/one; OUT=../bench/results/one.txt
python3 gen_struct.py "$W/struct" > /dev/null
python3 real_prep.py "$PTB_DIR" "$W/real" > /dev/null
python3 gen_long2.py "$PTB_DIR" "$W/long2" > /dev/null
python3 gen_long3.py "$PTB_DIR" "$W/long2" "$W/long3" > /dev/null
{
echo "=== structure: memory, nesting (300 / 1,000 / 10,000 sentences), facts"
for T in memory nesting; do for N in 300 1000 10000; do echo "$T"; python3 one.py "$W/struct/$T" train_$N.txt task; done; done
echo facts; python3 one.py "$W/struct/facts" train_all.txt task
echo "=== structure, controls (1,000 sentences; facts)"
for C in trust=0 far=0 newest=0 bind=0; do
    for T in memory nesting; do echo "$T"; python3 one.py "$W/struct/$T" train_1000.txt task $C; done
    echo facts; python3 one.py "$W/struct/facts" train_all.txt task $C
done
echo "=== real English (Penn Treebank)"
for K in clean noisy; do for N in 2000 10000 42068; do python3 one.py "$W/real" train_${K}_$N.txt; done; done
echo "=== real English, controls (10,000 sentences) and learning on through the test"
for C in trust=0 far=0 longer=0 bind=0 life=1; do python3 one.py "$W/real" train_clean_10000.txt $C; done
python3 one.py "$W/real" train_clean_42068.txt life=1
echo "=== long text"
python3 one_long.py "$W/long3" 300
python3 one_long.py "$W/long3" 1500
echo "=== long text, controls"
python3 one_long.py "$W/long3" 1500 kinds=0
python3 one_long.py "$W/long3" 1500 trust=0
} > "$OUT"
