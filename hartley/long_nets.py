"""The incumbents on long text (gen_long.py's files): a transformer that sees
the last 1,024 words (a fixed window, as real models have one) and an LSTM
that reads the whole document. Trained by backpropagation to predict every
next word; scored on the answers, within their class, as longtext.c is.

Run: python3 long_nets.py DATA_DIR seconds_per_model [window] [train_file] [width] [layers]
     (LSTM_SECS in the environment gives the LSTM its own budget)
The fair run (run_long_fair.sh) gives the nets their best chance: short
training documents (train_short.txt), a 256-word window and a small,
fast transformer, so it trains for thousands of steps.
"""
import math
import os
import sys
import time

import torch
import torch.nn as nn

torch.manual_seed(0)
torch.set_num_threads(4)
D, BUDGET = sys.argv[1], float(sys.argv[2])
W = int(sys.argv[3]) if len(sys.argv) > 3 else 1024
TRAIN = sys.argv[4] if len(sys.argv) > 4 else "train.txt"
WIDTH = int(sys.argv[5]) if len(sys.argv) > 5 else 256
LAYERS = int(sys.argv[6]) if len(sys.argv) > 6 else 4
V = int(open(f"{D}/meta.txt").readline().split()[1])


def load(name):
    with open(f"{D}/{name}") as f:
        return [[int(x) for x in line.split()] for line in f if line.strip()]


train = load(TRAIN)
classes = [[int(x) for x in line.split()] for line in open(f"{D}/classes.txt")]


class Transformer(nn.Module):
    def __init__(self, d=WIDTH, layers=LAYERS, heads=4):
        super().__init__()
        self.emb = nn.Embedding(V, d)
        self.pos = nn.Embedding(W, d)
        layer = nn.TransformerEncoderLayer(d, heads, 4 * d, dropout=0.1, batch_first=True)
        self.enc = nn.TransformerEncoder(layer, layers)
        self.out = nn.Linear(d, V)

    def forward(self, x):
        n = x.shape[1]
        mask = torch.triu(torch.full((n, n), float("-inf")), 1)
        return self.out(self.enc(self.emb(x) + self.pos(torch.arange(n)), mask=mask))


class LSTM(nn.Module):
    def __init__(self, d=WIDTH):
        super().__init__()
        self.emb = nn.Embedding(V, d)
        self.rnn = nn.LSTM(d, d, num_layers=2, batch_first=True)
        self.out = nn.Linear(d, V)

    def forward(self, x, h=None):
        y, h = self.rnn(self.emb(x), h)
        return self.out(y), h


def train_transformer(model):
    opt = torch.optim.Adam(model.parameters(), lr=1e-3 if W <= 256 else 3e-4)
    g = torch.Generator().manual_seed(1)
    start, steps = time.time(), 0
    while time.time() - start < BUDGET:
        xs = []
        for _ in range(8):     # random windows of W+1 words
            d = train[torch.randint(len(train), (1,), generator=g).item()]
            s = torch.randint(max(1, len(d) - W), (1,), generator=g).item()
            piece = d[s : s + W + 1]
            xs.append(piece + [0] * (W + 1 - len(piece)))
        x = torch.tensor(xs)
        loss = nn.functional.cross_entropy(model(x[:, :-1]).reshape(-1, V), x[:, 1:].reshape(-1))
        opt.zero_grad(); loss.backward(); nn.utils.clip_grad_norm_(model.parameters(), 1.0); opt.step()
        steps += 1
    return steps


def train_lstm(model):
    budget = float(os.environ.get("LSTM_SECS", BUDGET))
    opt = torch.optim.Adam(model.parameters(), lr=1e-3)
    g = torch.Generator().manual_seed(1)
    start, steps = time.time(), 0
    while time.time() - start < budget:
        docs = [train[torch.randint(len(train), (1,), generator=g).item()] for _ in range(8)]
        n = max(len(d) for d in docs)
        x = torch.tensor([d + [0] * (n - len(d)) for d in docs])
        h = None
        for s in range(0, n - 1, 200):    # carry the state across pieces of 200 words
            if time.time() - start > budget:
                break
            inp, tgt = x[:, s : s + 200], x[:, s + 1 : s + 201]
            inp = inp[:, : tgt.shape[1]]
            out, h = model(inp, h)
            h = tuple(t.detach() for t in h)
            loss = nn.functional.cross_entropy(out.reshape(-1, V), tgt.reshape(-1))
            opt.zero_grad(); loss.backward(); nn.utils.clip_grad_norm_(model.parameters(), 1.0); opt.step()
            steps += 1
    return steps


def answers(name):
    docs, masks = load(f"test_{name}.txt"), load(f"test_{name}_mask.txt")
    return [(d, m) for d, m in zip(docs, masks)]


def score(predict, gap):
    nk = len(classes)
    right, n, bits = [0] * (nk + 1), [0] * (nk + 1), 0.0
    for d, m in answers(gap):
        lp_all = predict(d, [t for t, k in enumerate(m) if k])
        for t, lp in lp_all.items():
            k = m[t]
            cls = classes[k - 1]
            sub = lp[cls] - torch.logsumexp(lp[cls], 0)
            j = cls.index(d[t])
            bits -= sub[j].item() / math.log(2)
            right[k] += int(sub.argmax().item() == j)
            n[k] += 1
    tot = sum(n)
    return sum(right) / tot, [right[k] / n[k] if n[k] else 0 for k in range(1, nk + 1)], bits / tot


def transformer_predict(model):
    def f(d, positions):
        out = {}
        with torch.no_grad():
            for t in positions:
                ctx = d[max(0, t - W) : t]
                out[t] = torch.log_softmax(model(torch.tensor([ctx]))[0, -1], -1)
        return out
    return f


def lstm_predict(model):
    def f(d, positions):
        with torch.no_grad():
            lp = torch.log_softmax(model(torch.tensor([d[:-1]]))[0][0], -1)
        return {t: lp[t - 1] for t in positions}
    return f


for name, model, fit, pred in (("transformer", Transformer(), train_transformer, transformer_predict),
                               ("lstm", LSTM(), train_lstm, lstm_predict)):
    t0 = time.time()
    steps = fit(model)
    model.eval()
    names = ["city", "country", "who-likes"] if len(classes) == 3 else ["city", "country", "continent", "who-likes"]
    for gap in ("50", "500", "2000", "8000", "new_50", "new_2000"):
        if not os.path.exists(f"{D}/test_{gap}.txt"):
            continue
        r, by, b = score(pred(model), gap)
        per = ", ".join(f"{nm} {v:.3f}" for nm, v in zip(names, by))
        print(f"RESULT {name} set {gap} right {r:.3f} ({per}) answer_bits {b:.3f} steps {steps} train_secs {BUDGET:.0f} "
              f"window {W if name == 'transformer' else 'all'} trained_on {TRAIN}", flush=True)
