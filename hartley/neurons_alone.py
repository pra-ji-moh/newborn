"""Ablation: the neuron layer on its own, with no counting model around it.
Trained exactly as grow.py trains it (first nine tenths of the training text,
starting numbers from likeness, 5 times through), scored on the test text.

Run: python3 neurons_alone.py DATA_DIR TRAIN_FILE [mode]   (mode: learn, dfa, bp)
"""
import sys
import numpy as np
import grow
import local

d, train = sys.argv[1], sys.argv[2]
mode = sys.argv[3] if len(sys.argv) > 3 else "learn"
V = int(open(f"{d}/meta.txt").readline().split()[1])
text = grow.Text(grow.load(f"{d}/{train}", V), V)
held = grow.Text(grow.load(f"{d}/test.txt", V), V)
NV = V + 2
A = text.cut(0, text.n * 9 // 10)
ctx = lambda t: np.stack(t.prev[1:], 1)
like = [grow.likeness(A.prev[j], A.w, NV) for j in range(1, grow.BACK + 1)]
net = local.train(ctx(A), A.w, NV, like, epochs=5, mode=mode, say=print)
p = net(ctx(held), held.w)
print(f"RESULT neurons alone ({mode}) {train}: perplexity {2 ** -np.log2(p).mean():.1f}")
