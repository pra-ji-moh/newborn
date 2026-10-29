#!/bin/sh
# The relational model learning the newborn's way, from 0 (newborn_lang.c),
# with its two controls. Needs PTB_DIR and gen_long2.py's folder (LONG2_DIR,
# made by run_long2.sh). Output: bench/results/newborn_lang.txt
set -e
cd "$(dirname "$0")"
cc -std=c99 -O2 newborn_lang.c -o newborn_lang -lm
W=${TMPDIR:-/tmp}/long3; OUT=../bench/results/newborn_lang.txt
python3 gen_long3.py "$PTB_DIR" "$LONG2_DIR" "$W" > /dev/null
{ echo "=== full design"; ./newborn_lang "$W"
  echo "=== control: no ideas for new wordings"; ./newborn_lang "$W" no_ideas
  echo "=== control: ideas without Wernicke's frame word"; ./newborn_lang "$W" no_wernicke; } > "$OUT"
