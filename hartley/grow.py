"""Ways of looking that nobody writes (Nov 14, 2026).

one.py learns which of its ways of looking back to trust, but the ways
themselves were written by hand. Here the model is given no ways of looking
at all, only what it can point at (the word one, two, three or four places
back; a word earlier in the sentence) and it builds the rest:

  kinds     It invents its own kinds of words: words are of a kind when the
            same things come after them (the author's "group by their
            future", Oct 27), split again and again into finer kinds. A
            kind is a new symbol the model can then look at like a word.
  ways      A way of looking is a set of things pointed at, read as the word
            or as its kind at some fineness. The model starts with none. It
            proposes a new way by adding one thing to a way it already
            keeps, tries it on the latest tenth of what it has read (using
            only what came before), and keeps it if it saves bits there.
            Shannon decides what is kept; what saves nothing is ruled out
            for good (Hartley) and never grown from.
  surprise  A way of looking leans on the way it grew from. The coarser way
            learns only from the finer one's surprises: the times the finer
            way met a word it had never seen there.
  credit    The kept ways are blended by the share of credit each earns
            after a given last word, as in one.py.

No gradients. The test text is never looked at until the end; the choice of
ways and their weights comes from the training text alone.

Prior art, plainly: kinds by future are distributional clustering (Brown et
al. 1992; Pereira, Tishby and Lee 1993); growing ways by adding one question
is feature induction and decision-tree language models (Bahl et al. 1989;
Della Pietra et al. 1997; Xu and Jelinek 2004); learning from the finer
way's surprises is Kneser-Ney smoothing (1995); trigger pairs (Rosenfeld
1996). What is the author's here is the frame: one model that is handed
nothing but pointers, invents its symbols and its ways of looking, and keeps
them by experience.

Run: python3 grow.py DATA_DIR TRAIN_FILE [key=value ...]
     (folder from real_prep.py; options: kinds=0 far=0 ways=N quiet=1)
"""
import sys
import time

import numpy as np

U = np.uint64
DEPTHS = (4, 7, 10)           # fineness of kinds a way may read (16, 128, 1,024 kinds)
BACK = 4                      # how far back a way may point
KEEP = 0.001                  # bits a word a new way must save to be kept (and twice its own uncertainty)
RULE_OUT = 0.00005            # below this it is ruled out for good
STEPS = 4                     # ways not kept yet that may still be grown from, each round
MAXWAYS = 40


def load(path, END):
    with open(path) as f:
        return [list(map(int, line.split())) + [END] for line in f]


class Text:
    """A stretch of text as columns: the word that came, and the words
    1..BACK places before it (BOS before the sentence starts)."""

    def __init__(self, sents, V):
        BOS = V + 1
        flat, keep = [], []
        for s in sents:
            flat += [BOS] * BACK + s
            keep += [False] * BACK + [True] * len(s)
        flat = np.array(flat, dtype=np.int64)
        idx = np.nonzero(np.array(keep))[0]
        self.w = flat[idx]
        self.prev = [None] + [flat[idx - j] for j in range(1, BACK + 1)]
        self.n = len(idx)
        # sentence number and place in the sentence, for words earlier in the sentence
        ends = np.cumsum(self.w == V)
        self.sent = np.concatenate(([0], ends[:-1]))
        starts = np.concatenate(([0], np.nonzero(self.w == V)[0] + 1))
        self.place = np.arange(self.n) - starts[self.sent]

    def cut(self, a, b):
        t = object.__new__(Text)
        t.w, t.n = self.w[a:b], b - a
        t.prev = [None] + [p[a:b] for p in self.prev[1:]]
        t.sent, t.place = self.sent[a:b] - self.sent[a], self.place[a:b]
        return t

    def rows(self):
        """Every (place, earlier place) where the earlier word is the first of
        its kind in the sentence and at least two back."""
        if not hasattr(self, "_rows"):
            NV = int(self.w.max()) + 3
            _, p = np.unique(self.sent * NV + self.w, return_index=True)
            ends = np.zeros(int(self.sent.max()) + 2, dtype=np.int64)
            np.maximum.at(ends, self.sent, np.arange(self.n))
            cnt = np.maximum(ends[self.sent[p]] - p - 1, 0)
            p, cnt = p[cnt > 0], cnt[cnt > 0]
            off = np.arange(cnt.sum()) - np.repeat(np.cumsum(cnt) - cnt, cnt)
            self._rows = (np.repeat(p + 2, cnt) + off, np.repeat(p, cnt))
        return self._rows


# ---------- kinds: words grouped by their future, split finer and finer ----------

def kinds_by_future(first, then, NV, depth, min_count=40, seed=0):
    """A tree of kinds. Two words fall on the same side of a split when what
    comes after them is alike (each goes to the side whose mix of following
    words gives its own followers the better odds). Returns, for each word,
    its path down the tree as bits; the first d bits name its kind at depth d."""
    pair, val = np.unique(first * NV + then, return_counts=True)
    row, col = pair // NV, pair % NV
    val = val.astype(np.float64)
    start = np.searchsorted(row, np.arange(NV + 1))
    total = np.bincount(row, weights=val, minlength=NV)
    path = np.zeros(NV, dtype=np.int64)
    nodes = [np.nonzero(total > 0)[0]]
    rng = np.random.RandomState(seed)
    for d in range(depth):
        nxt = []
        for words in nodes:
            path[words] <<= 1
            if len(words) < 2 or total[words].sum() < min_count:
                nxt.append(words)
                continue
            ent = np.concatenate([np.arange(start[w], start[w + 1]) for w in words])
            local = np.repeat(np.arange(len(words)), start[words + 1] - start[words])
            c, v = col[ent], val[ent]
            whole = np.bincount(c, weights=v, minlength=NV) + 1e-3
            # begin from the two most common words that differ most, then let every word choose
            order = np.argsort(-total[words])
            side = rng.randint(0, 2, len(words))
            side[order[0]] = 0
            if len(order) > 1:
                side[order[1]] = 1
            for _ in range(12):
                q = []
                for s in (0, 1):
                    m = side[local] == s
                    x = np.bincount(c[m], weights=v[m], minlength=NV) + 0.05 * whole / whole.sum() * max(1.0, v[m].sum())
                    q.append(np.log(x / x.sum()))
                score = np.bincount(local, weights=v * (q[0][c] - q[1][c]), minlength=len(words))
                new = (score < 0).astype(np.int64)
                if (new == side).all():
                    break
                side = new
            if side.min() == side.max():
                nxt.append(words)
                continue
            path[words] |= side
            nxt += [words[side == 0], words[side == 1]]
        nodes = nxt
    return path


# ---------- likeness: how alike two words are, as numbers ----------

def likeness(first, then, NV, dims=32, seed=0):
    """For each word, a short list of numbers such that words followed by the
    same things get nearly the same numbers. Counted, then boiled down in one
    step of linear algebra (the main directions of the table of square roots
    of 'how often x follows w'); no gradients. Rows have length 1 (or 0 for a
    word never met)."""
    pair, val = np.unique(first * NV + then, return_counts=True)
    row, col = pair // NV, pair % NV
    tot = np.bincount(row, weights=val, minlength=NV)
    M = np.zeros((NV, NV), dtype=np.float32)
    M[row, col] = np.sqrt(val / tot[row])
    rng = np.random.RandomState(seed)
    Q = M @ rng.standard_normal((NV, dims + 8)).astype(np.float32)
    for _ in range(3):                                           # sharpen the main directions
        Q, _ = np.linalg.qr(Q)
        Q = M @ (M.T @ Q)
    Q, _ = np.linalg.qr(Q)
    u, s_, _ = np.linalg.svd(Q.T @ M, full_matrices=False)
    vec = (Q @ u[:, :dims]) * np.sqrt(s_[:dims])
    norm = np.linalg.norm(vec, axis=1, keepdims=True)
    return (vec / np.maximum(norm, 1e-9)).astype(np.float32) * (norm > 1e-6)


# ---------- counting ----------

def table(key, w, NV):
    """What followed each key, how often; and how many different words."""
    kw = key * U(NV + 7) + w.astype(U)
    uk, n = np.unique(key, return_counts=True)
    ukw, first, c = np.unique(kw, return_index=True, return_counts=True)
    _, t = np.unique(key[first], return_counts=True)
    n1, n2 = (c == 1).sum(), (c == 2).sum()
    d = min(0.95, max(0.3, n1 / (n1 + 2.0 * n2 + 1e-9)))
    return dict(uk=uk, n=n, t=t, ukw=ukw, c=c, d=d, first=first)


def lookup(tab, key, w, back, NV):
    """Chance of each word that came, from a table, leaning on `back` for
    what the table has not seen (absolute discounting)."""
    i = np.minimum(np.searchsorted(tab["uk"], key), len(tab["uk"]) - 1)
    hit = tab["uk"][i] == key
    n = np.where(hit, tab["n"][i], 0).astype(np.float64)
    t = np.where(hit, tab["t"][i], 0)
    kw = key * U(NV + 7) + w.astype(U)
    j = np.minimum(np.searchsorted(tab["ukw"], kw), len(tab["ukw"]) - 1)
    c = np.where(tab["ukw"][j] == kw, tab["c"][j], 0)
    d = tab["d"]
    safe = np.maximum(n, 1.0)
    return np.where(n > 0, (np.maximum(c - d, 0.0) + d * t * back) / safe, back)


class Mind:
    def __init__(self, V, kinds=1, far=1, ways=MAXWAYS, quiet=0, recall=1):
        self.V, self.NV = V, V + 2
        self.o = dict(kinds=kinds, far=far, ways=ways, quiet=quiet, recall=recall)
        self.cache = {}

    def say(self, *a):
        if not self.o["quiet"]:
            print(*a, flush=True)

    def kinds(self, text):
        """Kinds of words, one tree for each place a way may point at: for the
        word j places back, by what comes j places after such a word; for a
        word earlier in the sentence (place 0), by what comes later in it."""
        depth = DEPTHS[-1]
        out = []
        for j in range(BACK + 1):
            k = np.zeros(self.NV, dtype=np.int64)
            if self.o["kinds"] and (j or self.o["far"]):
                if j:
                    k = kinds_by_future(text.prev[j], text.w, self.NV, depth)
                else:
                    pos, src = text.rows()
                    k = kinds_by_future(text.w[src], text.w[pos], self.NV, depth)
            k[self.V + 1] = (1 << depth) - 1                     # the start of a sentence is its own kind
            out.append(k)
        return out

    # ----- what a way of looking reads -----
    # One thing pointed at: ('w', j) the word j places back, ('k', j, d) its kind
    # at depth d; place 0 is a word earlier in the sentence (each of them in turn).
    def key(self, text, feats, kind, pos=None, src=None):
        n = text.n if pos is None else len(pos)
        k = np.zeros(n, dtype=U)
        for f in feats:
            if f[1]:
                x = text.prev[f[1]] if pos is None else text.prev[f[1]][pos]
            else:
                x = text.w[src]
            if f[0] == "k":
                x = kind[f[1]][x] >> (DEPTHS[-1] - f[2])
            code = (f[1] * 31 + (f[2] if f[0] == "k" else 0)) * 977 + 13
            k = k * U(1000003) + x.astype(U) + U(code)
        return k

    def chance(self, way, tr, q, kind, tag):
        """The chance a way gives each word of `q`, counted on `tr`. A way is
        the line it grew along; it leans on the way before it in that line,
        and that way learns from this one's surprises."""
        ck = (way, tag)
        if ck in self.cache:
            return self.cache[ck]
        NV = self.NV
        if not way:
            tab = table(np.zeros(tr.n, dtype=U), tr.w, NV)
            p = lookup(tab, np.zeros(q.n, dtype=U), q.w, np.full(q.n, 1.0 / NV), NV)
        else:
            own, parent = way[-1], way[:-1]
            pown = parent[-1] if parent else ()
            pp = self.chance(parent, tr, q, kind, tag)
            if any(f[1] == 0 for f in own):
                # a word earlier in the sentence: one guess for each such word, then their mean
                pos, src = tr.rows()
                tab = table(self.key(tr, own, kind, pos, src), tr.w[pos], NV)
                pos, src = q.rows()
                pr = lookup(tab, self.key(q, own, kind, pos, src), q.w[pos], pp[pos], NV)
                cnt = np.bincount(pos, minlength=q.n)
                p = np.where(cnt > 0, np.bincount(pos, weights=pr, minlength=q.n) / np.maximum(cnt, 1), pp)
            else:
                tab = table(self.key(tr, own, kind), tr.w, NV)
                surprise = np.zeros(tr.n, dtype=bool)
                surprise[tab["first"]] = True
                stab = table(self.key(tr, pown, kind)[surprise], tr.w[surprise], NV)
                back = lookup(stab, self.key(q, pown, kind), q.w, pp, NV)
                p = lookup(tab, self.key(q, own, kind), q.w, back, NV)
        self.cache[ck] = p
        return p

    # ----- recall: the moments of its life most like now -----
    def moment(self, text, like):
        """Now, as numbers: the words 2..BACK places back (nearer ones count
        more) and the words earlier in the sentence, by their likeness."""
        parts = [like[j][text.prev[j]] * (0.75 ** (j - 2)) for j in range(2, BACK + 1)]
        cs = np.cumsum(like[0][text.w], axis=0)
        cs = np.vstack([np.zeros((2, cs.shape[1]), dtype=cs.dtype), cs])
        i = np.arange(text.n)
        bag = cs[i] - cs[i - np.minimum(text.place, i)]          # words of this sentence up to two back
        bag /= np.maximum(np.linalg.norm(bag, axis=1, keepdims=True), 1e-9)
        f = np.hstack(parts + [0.5 * bag])
        return f / np.maximum(np.linalg.norm(f, axis=1, keepdims=True), 1e-9)

    def recall(self, tr, q, like, back, settings):
        """For each word of `q`: among the moments of `tr` that had the same
        last word, weigh each by how like now it was, and see what came next.
        Returns the chance of the word that came, for each (sharpness, lean)
        setting; `back` is what it leans on."""
        NV = self.NV
        ft, fq = self.moment(tr, like), self.moment(q, like)
        order = np.argsort(tr.prev[1], kind="stable")
        start = np.searchsorted(tr.prev[1][order], np.arange(NV + 1))
        qorder = np.argsort(q.prev[1], kind="stable")
        qstart = np.searchsorted(q.prev[1][qorder], np.arange(NV + 1))
        out = [back.copy() for _ in settings]
        for b in np.nonzero(np.diff(qstart))[0]:
            ti = order[start[b]:start[b + 1]]
            if not len(ti):
                continue
            X, wt = ft[ti], tr.w[ti]
            qi_all = qorder[qstart[b]:qstart[b + 1]]
            step = max(1, int(4e7 // len(ti)))
            for a in range(0, len(qi_all), step):
                qi = qi_all[a:a + step]
                S = fq[qi] @ X.T - 1.0
                match = wt[None, :] == q.w[qi][:, None]
                for n_, (sharp, lean) in enumerate(settings):
                    W = np.exp(S * sharp)
                    out[n_][qi] = ((W * match).sum(1) + lean * back[qi]) / (W.sum(1) + lean)
        return out

    def likes(self, text):
        like = []
        for j in range(BACK + 1):
            if j:
                like.append(likeness(text.prev[j], text.w, self.NV))
            else:
                pos, src = text.rows()
                like.append(likeness(text.w[src], text.w[pos], self.NV))
        return like

    # ----- credit: the blend of the kept ways, after each last word -----
    def share(self, P, b, rounds=12, lean=10.0):
        """P: one row per way, the chance it gave each word. Returns the
        weights after each last word (rows) and the blend's chances."""
        k, n = P.shape
        glob = np.full(k, 1.0 / k)
        lam = np.tile(glob, (self.NV, 1))
        cnt = np.bincount(b, minlength=self.NV).astype(np.float64)
        for _ in range(rounds):
            r = lam[b].T * P
            r /= r.sum(0)
            glob = r.mean(1)
            s = np.stack([np.bincount(b, weights=r[i], minlength=self.NV) for i in range(k)], 1)
            lam = (s + lean * glob) / (cnt[:, None] + lean)
        return lam, (lam[b].T * P).sum(0)

    @staticmethod
    def gain(pc, pm):
        """Bits a word saved by blending a new way into the blend, at its best
        share; and how unsure that number is."""
        lam = 0.2
        for _ in range(8):
            r = lam * pc / (lam * pc + (1 - lam) * pm)
            lam = min(0.999, max(1e-4, r.mean()))
        g = np.log2((lam * pc + (1 - lam) * pm) / pm)
        return g.mean(), g.std() / np.sqrt(len(g))

    # ----- growing ways of looking -----
    def moves(self, own, far_ok):
        """What a way may become: point at one more place, or read one place finer."""
        out = []
        used = {f[1] for f in own}
        for j in range(0 if (self.o["far"] and far_ok) else 1, BACK + 1):
            if j not in used:
                out.append(own + (("w", j),))
                if self.o["kinds"]:
                    out += [own + (("k", j, d),) for d in DEPTHS]
        for i, f in enumerate(own):
            if f[0] == "k":
                finer = [d for d in DEPTHS if d > f[2]]
                out.append(own[:i] + ((("k", f[1], finer[0]) if finer else ("w", f[1])),) + own[i + 1:])
        return [tuple(sorted(x, key=lambda f: (f[1], f[0], f[2:]))) for x in out]

    def grow(self, text):
        """text: all it has read. The latest tenth is where new ways are tried."""
        t0 = time.time()
        cut = int(text.n * 0.9)
        while text.place[cut]:
            cut += 1
        A, B = text.cut(0, cut), text.cut(cut, text.n)
        kind = self.kinds(A)
        self.say(f"kinds made ({time.time() - t0:.0f}s)")
        kept, seen, ruled, cands, steps = [()], set(), 0, {}, []
        P = np.stack([self.chance((), A, B, kind, "AB")])
        b = B.prev[1]
        lam, pm = self.share(P, b)
        while len(kept) < self.o["ways"]:
            for v in kept + steps:
                own = v[-1] if v else ()
                for new in self.moves(own, v in kept):
                    if (own, new) in seen:
                        continue
                    seen.add((own, new))
                    cands[v + (new,)] = self.chance(v + (new,), A, B, kind, "AB")
            scored = []
            for c, pc in list(cands.items()):
                g, se = self.gain(pc, pm)
                if g < RULE_OUT:                                 # Hartley: ruled out for good
                    ruled += 1
                    del cands[c]
                    self.cache.pop((c, "AB"), None)
                else:
                    scored.append((g, se, c))
            scored.sort(key=lambda x: -x[0])
            sure = [x for x in scored if x[0] >= KEEP and x[0] >= 2 * x[1]]
            if not sure:
                # nothing to keep: grow once more from the most promising ways not kept, then stop
                more = [c for _, _, c in scored[:STEPS] if c not in steps]
                if not more or len(steps) > 3 * STEPS:
                    break
                steps += more
                continue
            bg, _, best = sure[0]
            steps = [c for c in steps if c != best]
            kept.append(best)
            P = np.vstack([P, cands.pop(best)])
            lam, pm = self.share(P, b)
            self.say(f"  kept {self.name(best):58s} saved {bg:.4f} bits a word; blend on the latest tenth: {2 ** -np.log2(pm).mean():.1f}  ({time.time() - t0:.0f}s)")
        self.kept, self.lam, self.rec = kept, lam, None
        self.say(f"{len(kept) - 1} ways kept, {ruled} ruled out, {len(seen)} tried")
        if self.o["recall"]:
            # recall, tried like any other way: at each sharpness and lean, does it save bits on the latest tenth?
            one = (((("w", 1),),))
            back = self.chance(one, A, B, kind, "AB")
            settings = [(s_, l_) for s_ in (8.0, 16.0, 32.0) for l_ in (0.3, 1.0, 3.0)]
            ps = self.recall(A, B, self.likes(A), back, settings)
            gains = [self.gain(pc, pm)[0] for pc in ps]
            self.say("  recall gains:", " ".join(f"{s_:g}/{l_:g}:{g:.4f}" for (s_, l_), g in zip(settings, gains)))
            i = int(np.argmax(gains))
            self.say(f"  recall: best at sharpness {settings[i][0]:g}, lean {settings[i][1]:g}: saves {gains[i]:.4f} bits a word  ({time.time() - t0:.0f}s)")
            if gains[i] >= KEEP:
                self.rec = settings[i]
                P = np.vstack([P, ps[i]])
                self.lam, pm = self.share(P, b)
                self.say(f"  kept recall; blend on the latest tenth: {2 ** -np.log2(pm).mean():.1f}")
        return kept

    @staticmethod
    def name(way):
        def one(fs):
            return " + ".join(("earlier" if not f[1] else f"{f[1]}back") + ("" if f[0] == "w" else f"/kind{f[2]}") for f in fs) or "(nothing)"
        return one(way[-1]) + (f"  <- {one(way[-2])}" if len(way) > 1 else "")

    def test(self, text, held):
        """Count everything it has read, then guess a held-out text with the
        ways and the weights it settled on."""
        kind = self.kinds(text)
        P = np.stack([self.chance(v, text, held, kind, "T") for v in self.kept])
        if self.rec:
            back = self.chance(((("w", 1),),), text, held, kind, "T")
            P = np.vstack([P, self.recall(text, held, self.likes(text), back, [self.rec])[0]])
        p = (self.lam[held.prev[1]].T * P).sum(0)
        return -np.log2(p).mean()


def main():
    d, train = sys.argv[1], sys.argv[2]
    opts = {k: int(v) for k, v in (x.split("=") for x in sys.argv[3:] if "=" in x)}
    V = int(open(f"{d}/meta.txt").readline().split()[1])
    text = Text(load(f"{d}/{train}", V), V)
    held = Text(load(f"{d}/test.txt", V), V)
    m = Mind(V, **opts)
    t0 = time.time()
    m.grow(text)
    bits = m.test(text, held)
    opt = " ".join(f"{k}={v}" for k, v in opts.items())
    print(f"RESULT grow {train}: {text.n} words; ways {len(m.kept) - 1}; bits {bits:.4f} perplexity {2 ** bits:.1f}; {time.time() - t0:.0f}s" + (f" [{opt}]" if opt else ""), flush=True)


if __name__ == "__main__":
    main()
