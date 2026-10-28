"""The incumbents on real language: an LSTM and a transformer trained by
backpropagation on the same files real.c reads (real_prep.py's output).
Each sentence is predicted from its start, END included, as in real.c.
Validation (clean) picks the stopping point. Prints bits per word and
perplexity on the clean test set.

Run: python3 real_nets.py DATA_DIR TRAIN_FILE seconds_per_model [task|sample]
  task   - also score the masked words within their class (classes.txt,
           test_mask.txt) and the long test, as real.c does
  sample - also write 8 sentences from each model
"""
import os
import math
import sys
import time

import torch
import torch.nn as nn

torch.manual_seed(0)
torch.set_num_threads(3)
D, TRAIN, BUDGET = sys.argv[1], sys.argv[2], float(sys.argv[3])
MODE = sys.argv[4] if len(sys.argv) > 4 else ""
V = int(open(f"{D}/meta.txt").readline().split()[1])
END, BOS, PAD = V, V + 1, V + 2


def load(name):
    with open(f"{D}/{name}") as f:
        return [[int(x) for x in line.split()] + [END] for line in f if line.strip()]


train, valid, test = load(TRAIN), load("valid.txt"), load("test.txt")


def batch(sents):
    n = max(len(s) for s in sents)
    x = torch.full((len(sents), n), PAD, dtype=torch.long)
    y = torch.full((len(sents), n), -1, dtype=torch.long)
    for i, s in enumerate(sents):
        x[i, : len(s)] = torch.tensor([BOS] + s[:-1])
        y[i, : len(s)] = torch.tensor(s)
    return x, y


class LSTM(nn.Module):
    def __init__(self, d=256):
        super().__init__()
        self.emb = nn.Embedding(V + 3, d)
        self.rnn = nn.LSTM(d, d, num_layers=2, batch_first=True, dropout=0.3)
        self.drop = nn.Dropout(0.3)
        self.out = nn.Linear(d, V + 1)

    def forward(self, x):
        return self.out(self.drop(self.rnn(self.drop(self.emb(x)))[0]))


class Transformer(nn.Module):
    def __init__(self, d=256, layers=3, heads=4):
        super().__init__()
        self.emb = nn.Embedding(V + 3, d)
        self.pos = nn.Embedding(512, d)
        layer = nn.TransformerEncoderLayer(d, heads, 4 * d, dropout=0.2, batch_first=True)
        self.enc = nn.TransformerEncoder(layer, layers)
        self.out = nn.Linear(d, V + 1)

    def forward(self, x):
        n = x.shape[1]
        h = self.emb(x) + self.pos(torch.arange(n))
        mask = torch.triu(torch.full((n, n), float("-inf")), 1)
        return self.out(self.enc(h, mask=mask))


def bits(model, sents):
    model.eval()
    tot, n = 0.0, 0
    with torch.no_grad():
        for i in range(0, len(sents), 64):
            x, y = batch(sents[i : i + 64])
            loss = nn.functional.cross_entropy(model(x).reshape(-1, V + 1), y.reshape(-1), ignore_index=-1, reduction="sum")
            tot += loss.item()
            n += (y >= 0).sum().item()
    model.train()
    return tot / n / math.log(2)


def fit(model):
    opt = torch.optim.Adam(model.parameters(), lr=1e-3)
    start, best, state, steps = time.time(), float("inf"), None, 0
    g = torch.Generator().manual_seed(1)
    check = max(50, len(train) // 32 // 4)
    while time.time() - start < BUDGET:
        perm = torch.randperm(len(train), generator=g).tolist()
        for i in range(0, len(train), 32):
            x, y = batch([train[j] for j in perm[i : i + 32]])
            loss = nn.functional.cross_entropy(model(x).reshape(-1, V + 1), y.reshape(-1), ignore_index=-1)
            opt.zero_grad()
            loss.backward()
            nn.utils.clip_grad_norm_(model.parameters(), 1.0)
            opt.step()
            steps += 1
            if steps % check == 0:
                v = bits(model, valid)
                if v < best:
                    best, state = v, {k: t.clone() for k, t in model.state_dict().items()}
            if time.time() - start > BUDGET:
                break
    v = bits(model, valid)
    if v < best or state is None:
        state = model.state_dict()
    model.load_state_dict(state)
    return steps


def read_masks(name):
    with open(f"{D}/{name}") as f:
        return [[int(x) for x in line.split()] for line in f]


classes = []
if MODE == "task":
    with open(f"{D}/classes.txt") as f:
        classes = [[int(x) for x in line.split()] for line in f]
    sets = [("", test, read_masks("test_mask.txt"))]
    if os.path.exists(f"{D}/test_long.txt"):
        sets.append(("long_", load("test_long.txt"), read_masks("test_long_mask.txt")))


def probe(model, sents, masks):
    """Bits and hits on masked words, within their class."""
    model.eval()
    tot, n, right = 0.0, 0, 0
    with torch.no_grad():
        for i in range(0, len(sents), 32):
            x, _ = batch(sents[i : i + 32])
            lp = torch.log_softmax(model(x), -1)
            for b, (s, m) in enumerate(zip(sents[i : i + 32], masks[i : i + 32])):
                for t, k in enumerate(m):
                    if not k:
                        continue
                    cls = classes[k - 1]
                    sub = lp[b, t, cls]
                    sub = sub - torch.logsumexp(sub, 0)
                    j = cls.index(s[t])
                    tot -= sub[j].item() / math.log(2)
                    right += int(sub.argmax().item() == j)
                    n += 1
    model.train()
    return (tot / n, right / n) if n else (0.0, 0.0)


def sample(model, count=8):
    model.eval()
    g = torch.Generator().manual_seed(5)
    with torch.no_grad():
        for _ in range(count):
            seq = [BOS]
            for _ in range(60):
                p = torch.softmax(model(torch.tensor([seq]))[0, -1], -1)
                w = torch.multinomial(p, 1, generator=g).item()
                if w == END:
                    break
                seq.append(w)
            print("SAMPLE", name, " ".join(map(str, seq[1:])), flush=True)


for name, model in (("lstm", LSTM()), ("transformer", Transformer())):
    t0 = time.time()
    steps = fit(model)
    b = bits(model, test)
    extra = ""
    if MODE == "task":
        for tag, ss, ms in sets:
            pb, pr = probe(model, ss, ms)
            extra += f" {tag}probe_bits {pb:.4f} {tag}probe_right {pr:.4f}"
    print(f"RESULT {name} bits {b:.4f} perplexity {2 ** b:.1f}{extra} steps {steps} secs {time.time() - t0:.0f}", flush=True)
    if MODE == "sample":
        sample(model)
