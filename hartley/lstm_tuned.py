"""The fair opponent: an LSTM trained by backprop, tuned. Same data and task
as grow.py: each sentence predicted from its start, END included, scored on
the same test text. Modern practice for small data: tied input and output
weights, dropout, Adam, early stopping. A small grid of settings is tried and
the best on the validation text is reported on the test text (grow.py never
sees the validation text, so this favours the LSTM).

Run: python3 lstm_tuned.py DATA_DIR TRAIN_FILE [threads]
"""
import math
import sys
import time

import torch
import torch.nn as nn

D, TRAIN = sys.argv[1], sys.argv[2]
torch.set_num_threads(int(sys.argv[3]) if len(sys.argv) > 3 else 2)
V = int(open(f"{D}/meta.txt").readline().split()[1])
END, BOS, PAD = V, V + 1, V + 2


def load(name):
    with open(f"{D}/{name}") as f:
        return [[int(x) for x in line.split()] + [END] for line in f if line.strip()]


train, valid, test = load(TRAIN), load("valid.txt"), load("test.txt")


def batches(sents, size, shuffle, g=None):
    order = torch.randperm(len(sents), generator=g).tolist() if shuffle else list(range(len(sents)))
    for a in range(0, len(order), size):
        b = [sents[i] for i in order[a:a + size]]
        n = max(len(s) for s in b)
        x = torch.full((len(b), n), PAD, dtype=torch.long)
        y = torch.full((len(b), n), -1, dtype=torch.long)
        for i, s in enumerate(b):
            x[i, :len(s)] = torch.tensor([BOS] + s[:-1])
            y[i, :len(s)] = torch.tensor(s)
        yield x, y


class LSTM(nn.Module):
    def __init__(self, d, layers, drop):
        super().__init__()
        self.emb = nn.Embedding(V + 3, d)
        self.rnn = nn.LSTM(d, d, num_layers=layers, batch_first=True, dropout=drop if layers > 1 else 0)
        self.drop = nn.Dropout(drop)
        self.out = nn.Linear(d, V + 3)
        self.out.weight = self.emb.weight                      # tied weights

    def forward(self, x):
        return self.out(self.drop(self.rnn(self.drop(self.emb(x)))[0]))[..., :V + 1]


def score(m, sents):
    m.eval()
    tot, n = 0.0, 0
    with torch.no_grad():
        for x, y in batches(sents, 128, False):
            o = m(x)
            tot += nn.functional.cross_entropy(o.reshape(-1, V + 1), y.reshape(-1), ignore_index=-1, reduction="sum").item()
            n += (y >= 0).sum().item()
    return math.exp(tot / n)


def run(d, layers, drop, lr, max_epochs=40, patience=3):
    torch.manual_seed(0)
    g = torch.Generator().manual_seed(0)
    m = LSTM(d, layers, drop)
    opt = torch.optim.Adam(m.parameters(), lr=lr)
    best, best_state, bad, t0 = float("inf"), None, 0, time.time()
    for ep in range(max_epochs):
        m.train()
        for x, y in batches(train, 32, True, g):
            opt.zero_grad()
            loss = nn.functional.cross_entropy(m(x).reshape(-1, V + 1), y.reshape(-1), ignore_index=-1)
            loss.backward()
            nn.utils.clip_grad_norm_(m.parameters(), 0.5)
            opt.step()
        v = score(m, valid)
        print(f"  d={d} layers={layers} drop={drop} lr={lr} epoch {ep + 1}: valid {v:.1f}  ({time.time() - t0:.0f}s)", flush=True)
        if v < best:
            best, bad = v, 0
            best_state = {k: t.clone() for k, t in m.state_dict().items()}
        else:
            bad += 1
            if bad >= patience:
                break
    m.load_state_dict(best_state)
    return best, score(m, test)


results = []
for d, layers, drop, lr in [(256, 1, 0.5, 2e-3), (256, 2, 0.5, 2e-3), (400, 2, 0.5, 2e-3),
                            (400, 2, 0.65, 2e-3), (256, 1, 0.3, 2e-3), (650, 2, 0.65, 1e-3)]:
    v, t = run(d, layers, drop, lr)
    results.append((v, t, (d, layers, drop, lr)))
    print(f"SETTING d={d} layers={layers} drop={drop} lr={lr}: valid {v:.1f} test {t:.1f}", flush=True)
v, t, s = min(results)
print(f"RESULT tuned LSTM {TRAIN}: best on validation {s}: valid {v:.1f}, test perplexity {t:.1f}")
