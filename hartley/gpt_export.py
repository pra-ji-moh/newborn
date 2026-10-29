"""Export long-text test documents as prompts for a chat model (ChatGPT,
Claude), with an answer key, and run the relational model on exactly the
same documents so the scores compare like for like.

Takes the first 3 documents of each test set in gen_long2.py's folder:
gaps 50, 500, 2,000, 8,000 words, and the never-seen wordings at 2,000.
Writes, under OUT_DIR:
    prompt_<set>_<n>.txt   paste one into the chat, as is
    key.txt                the answers (do not paste)
    model/                 the same documents for longtext.c
Score a chat model with score_gpt.py.

Run: python3 gpt_export.py PTB_DIR LONG2_DIR OUT_DIR [set ...]
     (a folder with vocab.txt, such as gen_long3.py's, uses that vocabulary;
     sets default to 50 500 2000 8000 new_2000)
"""
import collections
import os
import shutil
import sys

ptb_dir, data, out = sys.argv[1], sys.argv[2], sys.argv[3]
os.makedirs(f"{out}/model", exist_ok=True)

# the vocabulary, built exactly as gen_long2.py builds it
raw = [line.split() for line in open(f"{ptb_dir}/ptb.train.txt") if line.strip()]
common = {w for w, _ in collections.Counter(w for s in raw for w in s).most_common(2000)}
ptb = [[w if w in common else "<unk>" for w in s] for s in raw]
extra = ["lives", "has", "home", "where", "belongs", "includes", "part", "contains", "likes", "liked",
         "fan", "moved", "who", "?", "city", "country", "continent", ".", "calls", "lies", "holds", "admires"]
words = sorted({w for s in ptb for w in s} | set(extra) | {"in", "a", "is", "to", "of", "by"})
vocab = words + [f"person{i}" for i in range(600)] + [f"city{i}" for i in range(60)] + \
    [f"country{i}" for i in range(12)] + [f"continent{i}" for i in range(4)]
if os.path.exists(f"{data}/vocab.txt"):
    vocab = open(f"{data}/vocab.txt").read().split("\n")[:-1]
assert len(vocab) == int(open(f"{data}/meta.txt").read().split()[1]), "vocabulary does not match gen_long2.py"

INSTRUCTIONS = """Read the document below. It is newspaper text mixed with facts about people, cities, countries and continents. Some facts change later in the document (for example, someone moves to another city); always use the latest. In the newspaper text, "<unk>" stands for a rare word and "N" for a number; ignore them.

Answer every question at the end using only the document. Reply with only the answers, numbered, one word per line, like:
1. city7
2. person12

DOCUMENT:
"""


def question(q):
    if q[0] == "who":
        return f"Who likes {q[2]}?"
    kind = q[1]
    return {"city": f"Which city does {q[0]} live in now?",
            "country": f"Which country is {q[0]}'s city in?",
            "continent": f"Which continent is {q[0]}'s country in?"}[kind]


sets = sys.argv[4:] or ["50", "500", "2000", "8000", "new_2000"]
key = open(f"{out}/key.txt", "w")
for name in sets:
    docs = [l.split() for l in open(f"{data}/test_{name}.txt")][:3]
    masks = [l.split() for l in open(f"{data}/test_{name}_mask.txt")][:3]
    with open(f"{out}/model/test_{name}.txt", "w") as f:
        f.write("".join(" ".join(d) + "\n" for d in docs))
    with open(f"{out}/model/test_{name}_mask.txt", "w") as f:
        f.write("".join(" ".join(m) + "\n" for m in masks))
    for n, (d, m) in enumerate(zip(docs, masks), 1):
        toks = [vocab[int(x)] for x in d]
        # split into sentences; questions are the sentences holding an answer
        sents, cur, has = [], [], False
        for t, w in enumerate(toks):
            cur.append((w, m[t] != "0"))
            if w == ".":
                sents.append(cur); cur = []
        first_q = next(i for i, s in enumerate(sents) if any(a for _, a in s))
        text = " ".join(w for s in sents[:first_q] for w, _ in s)
        qs, answers = [], []
        for s in sents[first_q:]:
            ws = [w for w, _ in s]
            ans = next(w for w, a in s if a)
            qs.append(question(ws))
            answers.append(ans)
        with open(f"{out}/prompt_{name}_{n}.txt", "w") as f:
            f.write(INSTRUCTIONS + text + "\n\nQUESTIONS:\n" + "".join(f"{i}. {q}\n" for i, q in enumerate(qs, 1)))
        key.write(f"{name}_{n} " + " ".join(answers) + "\n")
key.close()
for f in ("train.txt", "train_mask.txt", "valid.txt", "classes.txt", "meta.txt", "dot_q.txt", "vocab.txt"):
    if os.path.exists(f"{data}/{f}"):
        shutil.copy(f"{data}/{f}", f"{out}/model/{f}")
print("prompts:", len(sets) * 3)
