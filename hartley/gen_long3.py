"""Passives and new words: one more test set for gen_long2.py's folder.

test_pas_2000: like gen_long2.py's documents (2,000 words between facts and
questions), but every fact is in wording never seen in training, and half
of the "likes" facts are passives of verbs never seen:
    P resides in C / C is the home of P
    C is located in K / K is made up of C
    K is located in X / X is made up of K
    P adores Q / Q is adored by P / P respects Q / Q is respected by P
The vocabulary is gen_long2.py's with the new words added at the end, so
every earlier file keeps its numbers. Writes into a copy of the folder.

Run: python3 gen_long3.py PTB_DIR LONG2_DIR OUT_DIR
"""
import collections
import os
import random
import shutil
import sys

ptb_dir, src, out = sys.argv[1], sys.argv[2], sys.argv[3]
if os.path.exists(out):
    shutil.rmtree(out)
shutil.copytree(src, out)
rng = random.Random(41)

# gen_long2.py's vocabulary, exactly
raw = [line.split() for line in open(f"{ptb_dir}/ptb.train.txt") if line.strip()]
common = {w for w, _ in collections.Counter(w for s in raw for w in s).most_common(2000)}
ptb = [[w if w in common else "<unk>" for w in s] for s in raw]
extra = ["lives", "has", "home", "where", "belongs", "includes", "part", "contains", "likes", "liked",
         "fan", "moved", "who", "?", "city", "country", "continent", ".", "calls", "lies", "holds", "admires"]
words = sorted({w for s in ptb for w in s} | set(extra) | {"in", "a", "is", "to", "of", "by"})
people = [f"person{i}" for i in range(600)]
cities = [f"city{i}" for i in range(60)]
countries = [f"country{i}" for i in range(12)]
continents = [f"continent{i}" for i in range(4)]
vocab = words + people + cities + countries + continents
assert len(vocab) == int(open(f"{src}/meta.txt").read().split()[1]), "not gen_long2.py's vocabulary"
for w in ["resides", "located", "made", "up", "adores", "adored", "respects", "respected", "the"]:
    if w not in vocab:
        vocab.append(w)
ids = {w: i for i, w in enumerate(vocab)}

FORMS = {
    "home": [lambda p, c: [p, "resides", "in", c], lambda p, c: [c, "is", "the", "home", "of", p]],
    "land": [lambda c, k: [c, "is", "located", "in", k], lambda c, k: [k, "is", "made", "up", "of", c]],
    "cont": [lambda k, x: [k, "is", "located", "in", x], lambda k, x: [x, "is", "made", "up", "of", k]],
    "likes": [lambda p, q: [p, "adores", q], lambda p, q: [q, "is", "adored", "by", p],
              lambda p, q: [p, "respects", q], lambda p, q: [q, "is", "respected", "by", p]],
}


def filler(n_words):
    out_, n = [], 0
    while n < n_words:
        s = rng.choice(ptb) + ["."]
        out_.append(s)
        n += len(s)
    return out_


def document(gap):
    ps = rng.sample(people, 8)
    cs = rng.sample(cities, 8)
    ks = rng.sample(countries, 6)
    land = {c: rng.choice(ks) for c in cs}
    cont = {k: rng.choice(continents) for k in ks}
    home = {p: rng.choice(cs) for p in ps}
    order = ps[:]
    rng.shuffle(order)
    likes = {order[i]: order[(i + 1) % 8] for i in range(8)}
    say = lambda kind, a, b: rng.choice(FORMS[kind])(a, b) + ["."]
    facts = [say("home", p, home[p]) for p in ps] + [say("land", c, land[c]) for c in cs]
    facts += [say("cont", k, cont[k]) for k in ks] + [say("likes", p, likes[p]) for p in ps]
    rng.shuffle(facts)
    movers = rng.sample(ps, 3)
    moves = []
    for p in movers:
        new = rng.choice([c for c in cs if c != home[p]])
        home[p] = new
        moves.append([p, "moved", "to", new, "."])
    qs = []
    for p in ps:
        qs.append(([p, "city", "?", home[p], "."], 1))
        qs.append(([p, "country", "?", land[home[p]], "."], 2))
        qs.append(([p, "continent", "?", cont[land[home[p]]], "."], 3))
        qs.append((["who", "likes", likes[p], "?", p, "."], 4))
    rng.shuffle(qs)
    qs = qs[:16]
    half = gap // 2
    sents = [(s, 0) for s in filler(rng.randint(5, 30))] + [(f, 0) for f in facts]
    sents += [(s, 0) for s in filler(half)] + [(m, 0) for m in moves] + [(s, 0) for s in filler(gap - half)] + qs
    toks, mask = [], []
    for s, k in sents:
        for j, w in enumerate(s):
            toks.append(ids[w])
            mask.append(k if (k and j == len(s) - 2) else 0)
    return toks, mask


docs = [document(2000) for _ in range(60)]
with open(f"{out}/test_pas_2000.txt", "w") as f, open(f"{out}/test_pas_2000_mask.txt", "w") as g:
    for t, m in docs:
        f.write(" ".join(map(str, t)) + "\n")
        g.write(" ".join(map(str, m)) + "\n")
with open(f"{out}/meta.txt", "w") as f:
    f.write(f"V {len(vocab)}\n")
with open(f"{out}/vocab.txt", "w") as f:
    f.write("\n".join(vocab) + "\n")
print("vocab", len(vocab))
