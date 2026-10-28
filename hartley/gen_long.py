"""Long text: reading comprehension over a stream, as files longtext.c and
long_nets.py read.

Each document is real newspaper text (Penn Treebank sentences) with facts
about people made up for that document, so nothing can be learned by heart:
    P lives in C .      C is in K .      P likes Q .
Later in the same document come questions, each answered by the next word:
    P city ? C          (one step)
    P country ? K       (two steps: lives in, then is in)
    who likes Q ? P     (direction: the one who likes Q, not the one Q likes)
The gap between a fact and its question is measured in words of text:
about 50, 500, 2,000 or 8,000.

Files (one document per line, words as numbers):
    train.txt                      training documents, all gaps mixed
    valid.txt
    test_<gap>.txt, test_<gap>_mask.txt   test documents at one gap; the mask
                                   marks each answer with its class
    classes.txt                    1: cities, 2: countries, 3: people
    meta.txt                       V (END is V)
Run: python3 gen_long.py PTB_DIR OUT_DIR
"""
import os
import random
import sys

src, out = sys.argv[1], sys.argv[2]
os.makedirs(out, exist_ok=True)
rng = random.Random(11)

ptb = [line.split() for line in open(f"{src}/ptb.train.txt") if line.strip()]
words = sorted({w for s in ptb for w in s})
people = [f"person{i}" for i in range(300)]
cities = [f"city{i}" for i in range(40)]
countries = [f"country{i}" for i in range(8)]
extra = ["lives", "likes", "who", "?", "city", "country", "."]
vocab = words + [w for w in extra if w not in words] + people + cities + countries
ids = {w: i for i, w in enumerate(vocab)}
V = len(vocab)


def filler(n_words):
    out_, n = [], 0
    while n < n_words:
        s = rng.choice(ptb) + ["."]
        out_.append(s)
        n += len(s)
    return out_


def document(gap):
    """Facts about four people, then about `gap` words of text, then questions."""
    ps = rng.sample(people, 4)
    cs = rng.sample(cities, 4)
    land = {c: rng.choice(countries) for c in cs}
    home = {p: rng.choice(cs) for p in ps}
    likes = {ps[i]: ps[(i + 1) % 4] for i in range(4)}
    facts = [[p, "lives", "in", home[p], "."] for p in ps] + [[c, "is", "in", land[c], "."] for c in cs]
    facts += [[p, "likes", likes[p], "."] for p in ps]
    rng.shuffle(facts)
    qs = []
    for p in ps:
        qs.append(([p, "city", "?", home[p], "."], 1))
        qs.append(([p, "country", "?", land[home[p]], "."], 2))
        liked = likes[p]
        qs.append((["who", "likes", liked, "?", p, "."], 3))
    rng.shuffle(qs)
    qs = qs[:6]
    sents = [(s, 0) for s in filler(rng.randint(5, 30))]      # a little text first
    sents += [(f, 0) for f in facts]
    sents += [(s, 0) for s in filler(gap)]
    sents += qs
    toks, mask = [], []
    for s, k in sents:
        for j, w in enumerate(s):
            toks.append(ids[w])
            # the answer is the word before the closing "."
            mask.append(k if (k and j == len(s) - 2) else 0)
    return toks, mask


def write(name, docs):
    with open(f"{out}/{name}.txt", "w") as f, open(f"{out}/{name}_mask.txt", "w") as g:
        for t, m in docs:
            f.write(" ".join(map(str, t)) + "\n")
            g.write(" ".join(map(str, m)) + "\n")


gaps = [50, 500, 2000, 8000]
write("train", [document(rng.choice(gaps[:3])) for _ in range(1500)])
write("valid", [document(rng.choice(gaps[:3])) for _ in range(100)])
for g in gaps:
    write(f"test_{g}", [document(g) for _ in range(60)])
with open(f"{out}/classes.txt", "w") as f:
    for group in (cities, countries, people):
        f.write(" ".join(str(ids[w]) for w in group) + "\n")
with open(f"{out}/meta.txt", "w") as f:
    f.write(f"V {V}\n")
with open(f"{out}/dot_q.txt", "w") as f:
    f.write(f"{ids['.']} {ids['?']}\n")
print("vocab", V)
