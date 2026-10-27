"""Real language: the Penn Treebank (newspaper English, 10,000-word
vocabulary), as word numbers, one sentence per line, END added by the
programs. Training sets of several sizes, clean and with web-like noise.
Validation and test are always clean, as the labs evaluate.

Web-like noise (an estimate; the labs don't publish theirs): 2% of words
garbled (half become <unk>, half a wrong real word), 3% of sentences
replaced by one of 20 boilerplate lines, 2% of sentences scrambled.

Run: python3 real_prep.py PTB_DIR OUT_DIR
"""
import os
import random
import sys

src, out = sys.argv[1], sys.argv[2]
os.makedirs(out, exist_ok=True)


def read(name):
    with open(f"{src}/ptb.{name}.txt") as f:
        return [line.split() for line in f if line.strip()]


train, valid, test = read("train"), read("valid"), read("test")
vocab = sorted({w for s in train for w in s})
ids = {w: i for i, w in enumerate(vocab)}
V = len(vocab)
unk = ids["<unk>"]
rng = random.Random(7)


def noisy(sents):
    junk = [rng.sample(vocab, rng.randint(5, 15)) for _ in range(20)]
    res = []
    for s in sents:
        r = rng.random()
        if r < 0.03:
            res.append(list(rng.choice(junk)))
            continue
        s = list(s)
        if r < 0.05:
            rng.shuffle(s)
        for i in range(len(s)):
            if rng.random() < 0.02:
                s[i] = "<unk>" if rng.random() < 0.5 else rng.choice(vocab)
        res.append(s)
    return res


def write(name, sents):
    with open(f"{out}/{name}.txt", "w") as f:
        for s in sents:
            f.write(" ".join(str(ids[w]) for w in s) + "\n")


order = list(range(len(train)))
rng.shuffle(order)
for n in (2000, 10000, len(train)):
    part = [train[i] for i in order[:n]]
    write(f"train_clean_{n}", part)
    write(f"train_noisy_{n}", noisy(part))
write("valid", valid)
write("test", test)
with open(f"{out}/meta.txt", "w") as f:
    f.write(f"V {V}\nunk {unk}\nsizes 2000 10000 {len(train)}\n")
print("vocab", V, "train sentences", len(train))
