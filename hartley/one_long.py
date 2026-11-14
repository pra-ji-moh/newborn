"""The one rule (one.py) on long text: facts about made-up people set in
newspaper text, asked about 50 to 8,000 words later (gen_long2.py and
gen_long3.py's folder). A document is read sentence by sentence; what was
read in it stays in the memory of episodes until the next document begins
(the one thing told to the model here: where a document starts). One pass over the training documents, every answer tried before it
is seen; then the model, frozen, reads the held-out documents.

Run: python3 one_long.py DATA_DIR [DOCS] [key=value ...]
"""
import sys

from one import One

d = sys.argv[1]
ndocs = int(sys.argv[2]) if len(sys.argv) > 2 and sys.argv[2].isdigit() else 1500
opts = {k: int(v) for k, v in (x.split("=") for x in sys.argv[2:] if "=" in x)}
V = int(open(f"{d}/meta.txt").readline().split()[1])
DOT = int(open(f"{d}/dot_q.txt").read().split()[0])
classes = [None] + [list(map(int, line.split())) for line in open(f"{d}/classes.txt")]
names = ["", "city", "country", "continent", "who-likes"]


def docs(name, limit=None):
    with open(f"{d}/{name}.txt") as f, open(f"{d}/{name}_mask.txt") as g:
        for i, (line, ml) in enumerate(zip(f, g)):
            if limit is not None and i >= limit:
                break
            yield list(map(int, line.split())), list(map(int, ml.split()))


def read(m, toks, mask, learning, tally):
    m.begin()
    start = 0
    for t, w in enumerate(toks):
        if w == DOT:
            mk = mask[start:t]
            if any(mk):
                ta = [0, 0]
                m.read(toks[start:t] + [V], learning, mk, classes, ta)
                k = max(mk)
                tally[k][0] += ta[0]
                tally[k][1] += ta[1]
            else:
                m.read(toks[start:t] + [V], learning)
            start = t + 1


def show(what, tally):
    n = sum(x[0] for x in tally)
    r = sum(x[1] for x in tally)
    parts = ", ".join(f"{names[k]} {tally[k][1] / max(1, tally[k][0]):.3f}" for k in range(1, 5))
    print(f"{what} right {r / max(1, n):.3f} ({parts}) of {n}", flush=True)


m = One(V, **opts)
run = [[0, 0] for _ in range(5)]
for i, (toks, mask) in enumerate(docs("train", ndocs)):
    read(m, toks, mask, True, run)
    if i + 1 in (30, 100, 300, 1000, 1500):
        show(f"first tries, up to document {i + 1}:", run)
        run = [[0, 0] for _ in range(5)]
print(f"after {ndocs} documents" + (f" [{' '.join(f'{k}={v}' for k, v in opts.items())}]" if opts else ""))
for name in ("50", "500", "2000", "8000", "new_2000", "pas_2000"):
    tally = [[0, 0] for _ in range(5)]
    for toks, mask in docs(f"test_{name}"):
        read(m, toks, mask, False, tally)
    show(f"  test {name:9s}", tally)
