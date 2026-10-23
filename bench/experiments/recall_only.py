# Does memory work if each idea is only checked against remembered results when it comes to mind
# (recall), without sweeping all 11,700 rules after every outcome?
import sys, numpy as np
sys.path.insert(0, __import__('os').path.join(__import__('os').path.dirname(__file__), '..'))
import hiddenrule as H
from hiddenrule import Family, LEVELS, Oracle, best_query

def run(fam, secret, rng, cap, sweep, max_tries=3000):
    o = Oracle(fam, secret); order = rng.permutation(fam.N)
    standing = np.ones(fam.N, bool); held, nxt = [], 0; hq, hp, hr = [], [], []
    def ok(h):
        if not hq: return True
        return bool((fam.predict(np.array([h]), np.array(hq), np.array(hp))[0] == np.array(hr)).all())
    def refill():
        nonlocal nxt
        sc = 0
        while len(held) < cap and sc < fam.N:
            h = order[nxt % fam.N]; nxt += 1; sc += 1
            if not standing[h] or h in held: continue
            if not ok(h): standing[h] = False; continue
            held.append(h)
    refill()
    while o.used < max_tries and held:
        # known: it holds one, and nothing else unchecked survives recall
        q, p = best_query(fam, np.array(held), rng)
        if len(held) == 1:
            # one in mind: does any other remembered-consistent idea remain? recall the rest
            rest = [h for h in np.flatnonzero(standing) if h != held[0] and ok(h)]
            for h in np.flatnonzero(standing):
                if h != held[0] and h not in rest: standing[h] = False
            if not rest: break
            held.append(rest[0])
            if len(held) > cap: held.pop()
            q, p = best_query(fam, np.array(held), rng)
        r = o.ask(q, p); hq.append(q); hp.append(p); hr.append(r)
        pr = fam.predict(np.array(held), q[None, :], np.array([p]))[:, 0]
        for h, x in zip(list(held), pr):
            if x != r: standing[h] = False; held.remove(h)
        if sweep:
            alive = np.flatnonzero(standing)
            standing[alive[fam.predict(alive, q[None, :], np.array([p]))[:, 0] != r]] = False
            held[:] = [h for h in held if standing[h]]
        refill()
    w = held[0] if held else -1
    return o.used, w == secret

lv = sys.argv[1]; cap = int(sys.argv[2])
fam = Family(*LEVELS[lv]); rng = np.random.default_rng(1)
for sweep in (1, 0):
    res = [run(fam, int(rng.integers(fam.N)), np.random.default_rng(s), cap, sweep) for s in range(8)]
    print(lv, 'cap', cap, 'sweep' if sweep else 'recall-only', 'median tests', np.median([r[0] for r in res]), 'correct', sum(r[1] for r in res), '/8')
