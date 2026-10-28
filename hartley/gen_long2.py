"""Harder long text, as files longtext.c and long_nets.py read.

Like gen_long.py, but:
  - facts in many wordings, some with the order reversed:
        P lives in C / P has a home in C / C is where P lives / in C lives P
        C is in K / C belongs to K / K includes C
        K is in X / K is part of X / X contains K
        P likes Q / Q is liked by P / P is a fan of Q
  - updates: a third of the people later "moved to" another city; the
    answer is the latest city (and its country and continent)
  - three steps: "P continent ?" (city, then country, then continent)
  - eight people per document, 16 questions
  - test_new_*: facts in wordings never seen in training
        P calls C home / C is home to P / C lies in K / X holds K /
        P admires Q
The filler is real newspaper text with its vocabulary cut to the 2,000 most
common words (the rest become <unk>), the same for every model, so the nets
train fast enough on this machine.

Files: train.txt (gaps 50/500/2000 mixed), train_short.txt (gaps 0-100,
for the nets), valid.txt, test_<gap>.txt and _mask.txt (gaps 50, 500,
2000, 8000), test_new_<gap>.txt (50, 2000), classes.txt (1 cities,
2 countries, 3 continents, 4 people), meta.txt, dot_q.txt.
Run: python3 gen_long2.py PTB_DIR OUT_DIR
"""
import collections
import os
import random
import sys

src, out = sys.argv[1], sys.argv[2]
os.makedirs(out, exist_ok=True)
rng = random.Random(23)

raw = [line.split() for line in open(f"{src}/ptb.train.txt") if line.strip()]
common = {w for w, _ in collections.Counter(w for s in raw for w in s).most_common(2000)}
ptb = [[w if w in common else "<unk>" for w in s] for s in raw]
people = [f"person{i}" for i in range(600)]
cities = [f"city{i}" for i in range(60)]
countries = [f"country{i}" for i in range(12)]
continents = [f"continent{i}" for i in range(4)]
extra = ["lives", "has", "home", "where", "belongs", "includes", "part", "contains", "likes", "liked",
         "fan", "moved", "who", "?", "city", "country", "continent", ".", "calls", "lies", "holds", "admires"]
words = sorted({w for s in ptb for w in s} | set(extra) | {"in", "a", "is", "to", "of", "by"})
vocab = words + people + cities + countries + continents
ids = {w: i for i, w in enumerate(vocab)}

SEEN = {
    "home": [lambda p, c: [p, "lives", "in", c], lambda p, c: [p, "has", "a", "home", "in", c],
             lambda p, c: [c, "is", "where", p, "lives"], lambda p, c: ["in", c, "lives", p]],
    "land": [lambda c, k: [c, "is", "in", k], lambda c, k: [c, "belongs", "to", k], lambda c, k: [k, "includes", c]],
    "cont": [lambda k, x: [k, "is", "in", x], lambda k, x: [k, "is", "part", "of", x], lambda k, x: [x, "contains", k]],
    "likes": [lambda p, q: [p, "likes", q], lambda p, q: [q, "is", "liked", "by", p], lambda p, q: [p, "is", "a", "fan", "of", q]],
}
NEW = {
    "home": [lambda p, c: [p, "calls", c, "home"], lambda p, c: [c, "is", "home", "to", p]],
    "land": [lambda c, k: [c, "lies", "in", k]],
    "cont": [lambda k, x: [x, "holds", k]],
    "likes": [lambda p, q: [p, "admires", q]],
}


def filler(n_words):
    out_, n = [], 0
    while n < n_words:
        s = rng.choice(ptb) + ["."]
        out_.append(s)
        n += len(s)
    return out_


def document(gap, forms=SEEN):
    ps = rng.sample(people, 8)
    cs = rng.sample(cities, 8)
    ks = rng.sample(countries, 6)
    land = {c: rng.choice(ks) for c in cs}
    cont = {k: rng.choice(continents) for k in ks}
    home = {p: rng.choice(cs) for p in ps}
    order = ps[:]
    rng.shuffle(order)
    likes = {order[i]: order[(i + 1) % 8] for i in range(8)}
    say = lambda kind, a, b: rng.choice(forms[kind])(a, b) + ["."]
    facts = [say("home", p, home[p]) for p in ps] + [say("land", c, land[c]) for c in cs]
    facts += [say("cont", k, cont[k]) for k in ks] + [say("likes", p, likes[p]) for p in ps]
    rng.shuffle(facts)
    # updates: a third of the people move, after the first facts
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


def write(name, docs):
    with open(f"{out}/{name}.txt", "w") as f, open(f"{out}/{name}_mask.txt", "w") as g:
        for t, m in docs:
            f.write(" ".join(map(str, t)) + "\n")
            g.write(" ".join(map(str, m)) + "\n")


write("train", [document(rng.choice([50, 500, 2000])) for _ in range(1500)])
write("train_short", [document(rng.randint(0, 100)) for _ in range(8000)])
write("valid", [document(rng.choice([50, 500, 2000])) for _ in range(100)])
for g in (50, 500, 2000, 8000):
    write(f"test_{g}", [document(g) for _ in range(60)])
for g in (50, 2000):
    write(f"test_new_{g}", [document(g, NEW) for _ in range(60)])
with open(f"{out}/classes.txt", "w") as f:
    for group in (cities, countries, continents, people):
        f.write(" ".join(str(ids[w]) for w in group) + "\n")
with open(f"{out}/meta.txt", "w") as f:
    f.write(f"V {len(vocab)}\n")
with open(f"{out}/dot_q.txt", "w") as f:
    f.write(f"{ids['.']} {ids['?']}\n")
print("vocab", len(vocab))
