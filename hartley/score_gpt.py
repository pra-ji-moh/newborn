"""Score a chat model's replies to the prompts from gpt_export.py.

Save each reply as reply_<set>_<n>.txt next to the prompts (the numbered
list, as the model wrote it), then:
    python3 score_gpt.py OUT_DIR
Prints the share right per test set and per kind of question.
"""
import os
import re
import sys

out = sys.argv[1]
kinds = {}
for line in open(f"{out}/key.txt"):
    name, *answers = line.split()
    path = f"{out}/reply_{name}.txt"
    prompt = open(f"{out}/prompt_{name}.txt").read().split("QUESTIONS:\n")[1].strip().splitlines()
    if not os.path.exists(path):
        print(f"{name}: no reply yet")
        continue
    got = {}
    for l in open(path):
        m = re.match(r"\s*(\d+)[.):]\s*(\S+)", l)
        if m:
            got[int(m.group(1))] = m.group(2).strip(".,;*").lower()
    right = 0
    for i, (a, q) in enumerate(zip(answers, prompt), 1):
        kind = "who-likes" if q.split(". ", 1)[1].startswith("Who") else q.split()[2]
        ok = got.get(i) == a
        right += ok
        s = kinds.setdefault((name.rsplit("_", 1)[0], kind), [0, 0])
        s[0] += ok
        s[1] += 1
    print(f"{name}: {right}/{len(answers)} right")
print()
for (st, kind), (r, n) in sorted(kinds.items()):
    print(f"set {st:9s} {kind:10s} {r}/{n}")
