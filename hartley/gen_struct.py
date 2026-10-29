"""Tests of structure, as files real.c and real_nets.py read.

memory   - long-range meaning: a key word at the start decides the answer
           after "?" at the end; filler words in between follow their own
           little grammar. Training gaps 5-20 words; the long test 50-100.
nesting  - brackets inside brackets (three kinds), filler words between;
           a closing bracket must match the last one still open. Training
           up to 4 deep; the long test 5-8 deep.
facts    - knowledge and reasoning: "P lives in C" and "C is in N" for 200
           people, 30 cities, 6 countries. Questions "P city ? C" and
           "P country ? N" are asked in training for 70% of the people; the
           test asks them for the other 30%, whose facts were seen but who
           were never asked about. The country question needs two steps.

Each folder: meta.txt, classes.txt (the scored word classes), train_N.txt,
valid.txt, test.txt with test_mask.txt (class of each scored word, 0 for
the rest) and, where it applies, test_long.txt with test_long_mask.txt.

Run: python3 gen_struct.py OUT_DIR
"""
import os
import random
import sys

OUT = sys.argv[1]


def write(d, name, sents, masks=None):
    with open(f"{d}/{name}.txt", "w") as f:
        for s in sents:
            f.write(" ".join(map(str, s)) + "\n")
    if masks is not None:
        with open(f"{d}/{name}_mask.txt", "w") as f:
            for m in masks:
                f.write(" ".join(map(str, m)) + "\n")


def folder(name, V, classes):
    d = f"{OUT}/{name}"
    os.makedirs(d, exist_ok=True)
    with open(f"{d}/meta.txt", "w") as f:
        f.write(f"V {V}\n")
    with open(f"{d}/classes.txt", "w") as f:
        for c in classes:
            f.write(" ".join(map(str, c)) + "\n")
    return d


def filler_grammar(rng, words):
    return {w: rng.sample(words, 3) for w in words}


def fill(rng, gram, words, n):
    out, w = [], rng.choice(words)
    for _ in range(n):
        out.append(w)
        w = rng.choice(gram[w])
    return out


def memory():
    rng = random.Random(1)
    keys, answers, q = list(range(8)), list(range(8, 16)), 16
    fillers = list(range(17, 37))
    gram = filler_grammar(rng, fillers)
    to = dict(zip(keys, rng.sample(answers, 8)))
    d = folder("memory", 37, [answers])

    def make(n, lo, hi):
        sents, masks = [], []
        for _ in range(n):
            k = rng.choice(keys)
            s = [k] + fill(rng, gram, fillers, rng.randint(lo, hi)) + [q, to[k]]
            sents.append(s)
            masks.append([0] * (len(s) - 1) + [1])
        return sents, masks

    for n in (300, 1000, 3000, 10000):
        write(d, f"train_{n}", make(n, 5, 20)[0])
    write(d, "valid", make(500, 5, 20)[0])
    write(d, "test", *make(2000, 5, 20))
    write(d, "test_long", *make(500, 50, 100))


def nesting():
    rng = random.Random(2)
    opens, closes, fillers = [0, 1, 2], [3, 4, 5], list(range(6, 16))
    gram = filler_grammar(rng, fillers)
    d = folder("nesting", 16, [closes])

    def one(maxd, need):
        while True:
            s, m, stack, deepest, w = [], [], [], 0, rng.choice(fillers)
            while len(s) < 200:
                r = rng.random()
                if len(stack) < maxd and r < 0.25:
                    t = rng.randrange(3)
                    stack.append(t)
                    s.append(opens[t]); m.append(0)
                    deepest = max(deepest, len(stack))
                elif stack and r < 0.5:
                    s.append(closes[stack.pop()]); m.append(1)
                else:
                    s.append(w); m.append(0)
                    w = rng.choice(gram[w])
                    if not stack and rng.random() < 0.15:
                        break
            if not stack and deepest >= need and len(s) < 200:
                return s, m

    def make(n, maxd, need):
        pairs = [one(maxd, need) for _ in range(n)]
        return [p[0] for p in pairs], [p[1] for p in pairs]

    for n in (300, 1000, 3000, 10000):
        write(d, f"train_{n}", make(n, 4, 1)[0])
    write(d, "valid", make(500, 4, 1)[0])
    write(d, "test", *make(2000, 4, 1))
    write(d, "test_long", *make(500, 8, 5))


def facts():
    rng = random.Random(3)
    P, C, N = 200, 30, 6
    people, cities, countries = list(range(P)), list(range(P, P + C)), list(range(P + C, P + C + N))
    lives, in_, is_, ask, city_w, country_w = [P + C + N + i for i in range(6)]
    V = P + C + N + 6
    home = {p: rng.choice(cities) for p in people}
    land = {c: rng.choice(countries) for c in cities}
    asked = set(rng.sample(people, int(0.7 * P)))
    d = folder("facts", V, [cities, countries])
    fact_sents = [[p, lives, in_, home[p]] for p in people] * 3 + [[c, is_, in_, land[c]] for c in cities] * 3

    def q(p, two):
        return [p, country_w, ask, land[home[p]]] if two else [p, city_w, ask, home[p]]

    train = fact_sents + [q(p, two) for p in asked for two in (0, 1)] * 3
    rng.shuffle(train)
    write(d, "train_all", train)
    write(d, "valid", [q(p, two) for p in sorted(asked)[:40] for two in (0, 1)])
    test = [q(p, two) for p in people if p not in asked for two in (0, 1)]
    write(d, "test", test, [[0, 0, 0, 2 if s[1] == country_w else 1] for s in test])


memory()
nesting()
facts()
print("written to", OUT)
