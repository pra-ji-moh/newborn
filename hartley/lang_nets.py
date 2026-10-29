"""The incumbents: a transformer and an LSTM trained by backpropagation, on the
files lang.c writes. Same task (next word), same scoring (bits per word above
the floor; END removed and renormalised on the long set; answers scored
separately). Validation picks the stopping point, as it picks the bands for
the counting models.

Run: python3 lang_nets.py DIR [seconds_per_model]
"""
import math
import sys
import time

import torch
import torch.nn as nn

torch.manual_seed(0)
torch.set_num_threads(4)
D = sys.argv[1]
BUDGET = float(sys.argv[2]) if len(sys.argv) > 2 else 120.0


def load(name):
    with open(f"{D}/{name}.txt") as f:
        return [[int(x) for x in line.split()] for line in f if line.strip()]


meta = {}
with open(f"{D}/meta.txt") as f:
    for line in f:
        k, v = line.split()
        meta[k] = float(v)
V = int(meta["V"])
END, BOS = V, V + 1
NSYM = V + 2
MARK = int(meta["answer_marker"])
train, val, test, long_ = load("train"), load("val"), load("test"), load("long")


def batch(sents):
    n = max(len(s) for s in sents) + 1
    x = torch.full((len(sents), n), END, dtype=torch.long)
    y = torch.full((len(sents), n), -1, dtype=torch.long)
    for i, s in enumerate(sents):
        seq = [BOS] + s
        x[i, : len(seq)] = torch.tensor(seq)
        y[i, : len(s)] = torch.tensor(s)
    return x[:, :-1], y[:, :-1]


class Transformer(nn.Module):
    def __init__(self, d=64, layers=2, heads=4):
        super().__init__()
        self.emb = nn.Embedding(NSYM, d)
        layer = nn.TransformerEncoderLayer(d, heads, 4 * d, dropout=0.0, batch_first=True)
        self.enc = nn.TransformerEncoder(layer, layers)
        self.out = nn.Linear(d, V + 1)
        self.d = d

    def pos(self, n):
        p = torch.arange(n).unsqueeze(1)
        i = torch.arange(0, self.d, 2)
        ang = p / (10000 ** (i / self.d))
        pe = torch.zeros(n, self.d)
        pe[:, 0::2], pe[:, 1::2] = torch.sin(ang), torch.cos(ang)
        return pe

    def forward(self, x):
        n = x.shape[1]
        h = self.emb(x) * math.sqrt(self.d) + self.pos(n)
        mask = torch.triu(torch.full((n, n), float("-inf")), 1)
        return self.out(self.enc(h, mask=mask))


class LSTM(nn.Module):
    def __init__(self, d=128):
        super().__init__()
        self.emb = nn.Embedding(NSYM, d)
        self.rnn = nn.LSTM(d, d, batch_first=True)
        self.out = nn.Linear(d, V + 1)

    def forward(self, x):
        return self.out(self.rnn(self.emb(x))[0])


def score(model, sents, no_end):
    model.eval()
    bits = abits = 0.0
    words = awords = 0
    with torch.no_grad():
        for i in range(0, len(sents), 64):
            chunk = sents[i : i + 64]
            x, y = batch(chunk)
            logits = model(x)
            if no_end:
                logits[..., END] = float("-inf")
            lp = torch.log_softmax(logits, -1)
            for b, s in enumerate(chunk):
                for t, w in enumerate(s):
                    v = -lp[b, t, w].item() / math.log(2)
                    bits += v
                    words += 1
                    if t and s[t - 1] == MARK:
                        abits += v
                        awords += 1
    model.train()
    return bits / words, (abits / awords if awords else 0.0)


def fit(model):
    opt = torch.optim.Adam(model.parameters(), lr=1e-3)
    best, best_state, start, steps = float("inf"), None, time.time(), 0
    g = torch.Generator().manual_seed(1)
    while time.time() - start < BUDGET:
        perm = torch.randperm(len(train), generator=g).tolist()
        for i in range(0, len(train), 32):
            x, y = batch([train[j] for j in perm[i : i + 32]])
            loss = nn.functional.cross_entropy(model(x).reshape(-1, V + 1), y.reshape(-1), ignore_index=-1)
            opt.zero_grad()
            loss.backward()
            opt.step()
            steps += 1
            if steps % 50 == 0:
                v, _ = score(model, val, False)
                if v < best:
                    best, best_state = v, {k: t.clone() for k, t in model.state_dict().items()}
            if time.time() - start > BUDGET:
                break
    v, _ = score(model, val, False)
    if v < best:
        best_state = model.state_dict()
    model.load_state_dict(best_state)
    return steps


for name, model in (("transformer", Transformer()), ("lstm", LSTM())):
    t0 = time.time()
    steps = fit(model)
    te, ate = score(model, test, False)
    lo, alo = score(model, long_, True)
    params = sum(p.numel() for p in model.parameters())
    print(
        f"RESULT {name} {te - meta['floor_test']:.4f} {lo - meta['floor_long']:.4f} "
        f"{ate - meta['floor_ans_test']:.4f} {alo - meta['floor_ans_long']:.4f} "
        f"params={params} steps={steps} secs={time.time() - t0:.0f}"
    )
