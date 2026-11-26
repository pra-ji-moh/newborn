"""Build the router's test set from public datasets. Nothing here is written by us:
every query is a real one from its dataset, and its label (does it need the big
model?) comes from which dataset it is in.

  lookup, existing material (small model is enough):
    sciq      SciQ (Allen AI) science facts
    webq      WebQuestions (Stanford) facts about people, places, media
    openqa    Dolly-15k (Databricks) "open_qa": human-written fact questions
  needs the big model:
    gsm8k     GSM8K (OpenAI) maths word problems, steps in order
    creative  Dolly-15k "creative_writing": write something new

Usage: python3 build_eval.py RAW_DIR   (writes data/eval.jsonl)
Output tokens are estimated from each dataset's own reference answer.
"""
import json, random, sys, os
raw = sys.argv[1]
N = 600  # per source, half dev, half test
random.seed(7)

def take(rows, n):
    rows = [r for r in rows if r["q"].strip()]
    random.shuffle(rows)
    return rows[:n]

src = {}
src["sciq"] = take([{"q": x["question"], "a": x["correct_answer"] + ". " + x.get("support", "")[:300]}
                    for x in json.load(open(os.path.join(raw, "sciq/SciQ dataset-2 3/train.json")))], N)
src["webq"] = take([{"q": x["qText"], "a": ", ".join(x["answers"])}
                    for x in json.load(open(os.path.join(raw, "webq.json")))], N)
dolly = [json.loads(l) for l in open(os.path.join(raw, "dolly.jsonl"))]
src["openqa"] = take([{"q": x["instruction"], "a": x["response"]} for x in dolly if x["category"] == "open_qa"], N)
src["creative"] = take([{"q": x["instruction"], "a": x["response"]} for x in dolly
                        if x["category"] == "creative_writing" and not x["context"].strip()], N)
src["gsm8k"] = take([{"q": x["question"], "a": x["answer"]}
                     for x in map(json.loads, open(os.path.join(raw, "gsm8k_test.jsonl")))], N)

big = {"gsm8k", "creative"}
out = []
for s, rows in src.items():
    for i, r in enumerate(rows):
        out.append({"source": s, "needs_big": s in big, "split": "dev" if i % 2 == 0 else "test",
                    "q": r["q"].strip(), "in_tok": max(1, round(len(r["q"]) / 4)),
                    "out_tok": max(1, round(len(r["a"]) / 4))})
with open("data/eval.jsonl", "w") as f:
    for r in out: f.write(json.dumps(r) + "\n")
from collections import Counter
print(len(out), Counter((r["source"], r["split"]) for r in out))
