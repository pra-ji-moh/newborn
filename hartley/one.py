"""One rule, learned from experience.

The author's idea (Nov 14, 2026): nothing is handed over as information; the
model learns from its own experience. This file is that idea as one rule, in
place of the hand-built pieces (memory.c's memory and stack, longtext.c's
relations, newborn_lang.c's chains).

The rule. Before every word, everything that has something to say makes its
guess. Then the word comes, and the credit for it is shared out by how right
each guess was. A guess that keeps earning credit counts for more next time;
one that keeps being wrong counts for less, down to nothing. That is all the
learning there is: one pass, every word guessed before it is seen, no
gradients, no second look, nothing tuned on held-out text.

What "has something to say" is not written per task. There are two
memories, and each offers its ways of looking back:

  what usually follows (counts)
      local      the last two words (the trigram; every other guess is
                 measured against it)
      longer     the last three words, whole and with gaps
      far        a word held from earlier in the sentence, alone, and
                 together with the last word
      newest     the newest held word whose promise is still open, alone,
                 and together with the last word
  what happened (episodes)
      bind       a word in the last three has been met before; the word
                 that stood in a certain place beside it then (named by the
                 words between) is guessed now. Two such steps can chain.

Which words are held, which promise, which may bind, which ways of looking
count: all of it comes from the same record of experience.
  Shannon     credit is shared by how right each guess was. Guesses of one
              kind are blended by the bits each has saved against the blend
              of its kind; each kind then counts, after a given last word,
              by the share of credit it has earned there beside the local
              guess. A word stays held (and a word goes on binding) only if
              its guesses have saved bits, judged apart from whatever else
              is held; what is dropped is tried again now and then.
  Hartley     a promise is a word that has come after another every time,
              far more than it usually comes. It is kept only while it is
              never broken: one answer for each time the word is held, never
              an answer unasked (1 in 100 is let through as noise). When
              the answer comes, the newest open promise is crossed off. A
              binding that has been tried and is nearly never right is
              ruled out.
Nothing says "bracket", "question", "relation" or "stack" anywhere below.

Prior art, plainly: blending specialists that speak only sometimes, by
their record, is the sleeping-experts mixture (Freund, Schapire, Singer and
Warmuth 1997), close to context mixing in compression; sharing credit to
set blend weights is EM for interpolation (Jelinek and Mercer 1980), done
here as it reads; trigger pairs (Rosenfeld 1996); skip n-grams; binding by
recall is a pointer or copy step.

Run: python3 one.py DATA_DIR TRAIN_FILE [task] [key=value ...]
     (folders from gen_struct.py or real_prep.py)
     controls: far=0 newest=0 bind=0 longer=0; trust=0 (every guess counts
     the same, whatever its record); life=1 (it goes on learning while it
     reads the test, each word still guessed before it is seen)
"""
import math
import sys
from collections import OrderedDict

LOG2 = math.log2
FLOOR, CEIL = -12.0, 40.0     # a guess's standing among its kind is kept between these (bits)
TRY = 30                      # the first times a word is held are a trial
WORTH = 0.01                  # bits a held word must save, each time held, to stay
CAP = 16                      # words held at once
SPAN = 6                      # how far apart two words of an episode may stand
KEEP = 32                     # newest partners remembered for a word
MAXCHAIN = 48                 # ways of binding kept for one context


class One:
    def __init__(self, V, far=1, newest=1, bind=1, trust=1, longer=1, life=0):
        self.V, self.END = V, V
        self.o = dict(far=far, newest=newest, bind=bind, trust=trust, longer=longer, life=life)
        # what usually follows
        self.uni = [0] * (V + 1)
        self.total = 0
        self.n, self.ty, self.c = {}, {}, {}   # per key: times, kinds of outcome; per (key, word): times
        self.tr = {}                           # trust of a far / newest key among its kind, in bits
        self.kr = {}                           # a kind of guess after a last word: [its share of the credit, the local guess's share]
        self.kg = {}                           # the same for the kind as a whole
        self.cue = {}                          # held word -> [times held, bits saved this time, bits saved per time (lately), next retrial]
        # promises
        self.after = {}                        # word -> {later word: sentences}
        self.rest_n, self.pos_n = {}, 0        # how often a word is still to come in its sentence
        self.cand = {}                         # word -> [the word it may promise, kept, broken]
        self.promise = {}                      # word -> the word it promises (confirmed)
        self.ruled = set()                     # (word, later word) promises ruled out for good
        # what happened
        self.rel = {}                          # word -> newest partners {path: word}
        self.bx = {}                           # context -> {chain: [times, hits, trust]}
        self.bcue = {}                         # word -> [times tried as a cue, bits its bindings saved per time (lately), next retrial]

    # ---------- the local guess (counted, Witten-Bell) ----------
    def p_loc(self, a, b, w):
        p = (self.uni[w] + 1) / (self.total + self.V + 1)
        k = (1 << 60) | (b + 2)
        n = self.n.get(k)
        if n:
            t = self.ty[k]
            p = (self.c.get((k << 15) | w, 0) + t * p) / (n + t)
        k = (2 << 60) | ((a + 2) << 20) | (b + 2)
        n = self.n.get(k)
        if n:
            t = self.ty[k]
            p = (self.c.get((k << 15) | w, 0) + t * p) / (n + t)
        return p

    def _count(self, k, w):
        self.n[k] = self.n.get(k, 0) + 1
        kw = (k << 15) | w
        c = self.c.get(kw, 0)
        if not c:
            self.ty[k] = self.ty.get(k, 0) + 1
        self.c[kw] = c + 1

    # ---------- which words are held, and which may bind ----------
    def worth(self, k):
        if k in self.promise:
            return 1e9                     # a promise is open
        r = self.cue.get(k)
        if r is None or r[0] < TRY:
            return 2 * WORTH               # on trial
        if r[2] < WORTH and self.uni[k] >= r[3]:
            r[0], r[3] = TRY - TRY // 3, 2 * self.uni[k]     # dropped, but tried again now and then
            return 2 * WORTH
        return r[2]

    def may_bind(self, x):
        r = self.bcue.get(x)
        if r is None or r[0] < TRY or r[1] >= WORTH:
            return True
        if self.uni[x] >= r[2]:
            r[0], r[2] = TRY - TRY // 3, 2 * self.uni[x]     # dropped, but tried again now and then
            return True
        return False

    # ---------- everything that has something to say ----------
    def speakers(self, b, held, hist):
        return self._speakers(b, held, hist)

    def _speakers(self, b, held, hist):
        """Count keys awake now as (key, held word), and bindings as
        (chain, record, guessed word)."""
        keys, binds = [], []
        o = self.o
        if o["longer"] and len(hist) >= 2:      # the last three words, whole and with gaps
            a = hist[-2]
            keys.append(((9 << 60) | (a + 2), None))
            if len(hist) >= 3:
                z = hist[-3]
                keys.append(((8 << 60) | ((z + 2) << 40) | ((a + 2) << 20) | (b + 2), None))
                keys.append(((10 << 60) | ((z + 2) << 20) | (b + 2), None))
        if o["far"]:
            seen = set()
            for k, _ in held:
                if k not in seen:
                    seen.add(k)
                    keys.append(((3 << 60) | (k + 2), k))
                    keys.append(((4 << 60) | ((b + 2) << 20) | (k + 2), k))
        if o["newest"]:
            for i in range(len(held) - 1, -1, -1):
                k = held[i][0]
                if k in self.promise:
                    keys.append(((5 << 60) | (k + 2), k))
                    keys.append(((6 << 60) | ((b + 2) << 20) | (k + 2), k))
                    break
        if o["bind"]:
            ctx = tuple(hist[-3:])
            for p in range(1, len(ctx) + 1):
                x = ctx[-p]
                rx = self.rel.get(x)
                if not rx:
                    continue
                ex = self.bx.get((p,) + ctx[:len(ctx) - p] + ctx[len(ctx) - p + 1:])
                if not ex:
                    continue
                for chain, rec in ex.items():
                    y = rx.get(chain[0])
                    if y is not None and len(chain) == 2:
                        ry = self.rel.get(y)
                        y = ry.get(chain[1]) if ry else None
                    if y is not None:
                        binds.append((x, rec, y))
        return keys, binds

    def p_key(self, k, w, pl):
        n = self.n.get(k)
        if not n:
            return pl
        t = self.ty[k]
        if (k >> 60) in (3, 5):             # a held word alone knows nothing of the last words
            pl = (self.uni[w] + 1) / (self.total + self.V + 1)
        return (self.c.get((k << 15) | w, 0) + t * pl) / (n + t)

    def guess(self, a, b, keys, binds, w):
        """The trust-weighted blend for word w: guesses of one kind are
        blended by their own trust; each kind is then weighed against the
        local guess by the trust that kind has earned after this last word."""
        pl = self.p_loc(a, b, w)
        kn = {}
        use = self.o["trust"]
        for k, _ in keys:
            wt = 2.0 ** self.tr.get(k, 0.0) if use else 1.0
            sh = kn.get(k >> 60)
            if sh is None:
                sh = kn[k >> 60] = [0.0, 0.0]
            sh[0] += wt * self.p_key(k, w, pl)
            sh[1] += wt
        if binds:
            sh = kn[7] = [0.0, 0.0]
            for _, rec, y in binds:
                q = (rec[1] + 0.5) / (rec[0] + 1)
                wt = 2.0 ** rec[2] if use else 1.0
                sh[0] += wt * (q * (w == y) + (1 - q) * pl)
                sh[1] += wt
        num, den = pl, 1.0
        for kind, sh in kn.items():
            wt = self.weight(kind, b) if use else 0.25
            num += wt * sh[0] / sh[1]
            den += wt
        return num / den

    def weight(self, kind, b):
        """How much a kind of guess counts against the local guess (which
        counts 1) after this last word: the credit it has earned there
        against the credit the local guess earned beside it; a last word
        with little record leans on the kind's record as a whole."""
        g = self.kg.get(kind)
        go = g[0] / g[1] if g else 0.1
        r = self.kr.get((kind << 21) | (b + 2))
        return (r[0] + 5 * go) / (r[1] + 5) if r else go

    # ---------- experience: the word has come ----------
    def learn(self, a, b, keys, binds, hist, w):
        pl = self.p_loc(a, b, w)
        kn, part, ps = {}, {}, []
        for k, cw in keys:
            pk = self.p_key(k, w, pl)
            wt = 2.0 ** self.tr.get(k, 0.0)
            kind = k >> 60
            ps.append(pk)
            sh = kn.get(kind)
            if sh is None:
                sh = kn[kind] = [0.0, 0.0]
            sh[0] += wt * pk
            sh[1] += wt
            if cw is not None:
                sh = part.get((cw, kind))
                if sh is None:
                    sh = part[(cw, kind)] = [0.0, 0.0]
                sh[0] += wt * pk
                sh[1] += wt
        pbs = []
        if binds:
            sh = kn[7] = [0.0, 0.0]
            for x, rec, y in binds:
                q = (rec[1] + 0.5) / (rec[0] + 1)
                pb = q * (w == y) + (1 - q) * pl
                wt = 2.0 ** rec[2]
                pbs.append(pb)
                sh[0] += wt * pb
                sh[1] += wt
                sh2 = part.get((x, 7))
                if sh2 is None:
                    sh2 = part[(x, 7)] = [0.0, 0.0]
                sh2[0] += wt * pb
                sh2[1] += wt
        # what each held or binding word saved by itself: the local guess with its
        # guesses blended in, against the local guess alone
        solo = {}
        for (cw, kind), sh in part.items():
            wt = self.weight(kind, b)
            so = solo.get((cw, kind == 7))
            if so is None:
                so = solo[(cw, kind == 7)] = [pl, 1.0]
            so[0] += wt * sh[0] / sh[1]
            so[1] += wt
        for (cw, isbind), so in solo.items():
            if not isbind:
                r = self.cue.get(cw)
                if r is None:
                    r = self.cue[cw] = [0, 0.0, 0.0, 0]
                r[1] += LOG2(so[0] / so[1] / pl)
        # trust: each guess against the blend of its kind; each kind's weight against the local guess is set below
        for (k, _), pk in zip(keys, ps):
            sh = kn[k >> 60]
            t = self.tr.get(k, 0.0) + LOG2(pk * sh[1] / sh[0])
            self.tr[k] = FLOOR if t < FLOOR else CEIL if t > CEIL else t
            self._count(k, w)
        for (x, rec, y), pb in zip(binds, pbs):
            sh = kn[7]
            rec[0] += 1
            rec[1] += w == y
            rec[2] = max(FLOOR, min(CEIL, rec[2] + LOG2(pb * sh[1] / sh[0])))
        if kn:                                  # credit for the word that came, shared by how right each was
            ws = {kind: self.weight(kind, b) * sh[0] / sh[1] for kind, sh in kn.items()}
            tot = pl + sum(ws.values())
            for kind, x in ws.items():
                kk = (kind << 21) | (b + 2)
                r = self.kr.get(kk)
                if r is None:
                    r = self.kr[kk] = [0.0, 0.0]
                g = self.kg.get(kind)
                if g is None:
                    g = self.kg[kind] = [0.1, 1.0]
                r[0] += x / tot
                r[1] += pl / tot
                g[0] += x / tot
                g[1] += pl / tot
        if self.o["bind"] and w != self.END:    # did the word that came stand beside something present, before?
            ctx = tuple(hist[-3:])
            for p in range(1, len(ctx) + 1):
                x = ctx[-p]
                rx = self.rel.get(x)
                if not rx:
                    continue
                if not self.may_bind(x):
                    continue
                r = self.bcue.get(x)
                if r is None:
                    r = self.bcue[x] = [0, 0.0, 0]
                so = solo.get((x, True))        # what binding from this word saved, this time
                r[0] += 1
                r[1] += ((LOG2(so[0] / so[1] / pl) if so else 0.0) - r[1]) / min(r[0], 50)
                if r[0] == TRY:
                    r[2] = 2 * self.uni[x]
                found = [(path,) for path, y in rx.items() if y == w]
                if not found:
                    for path, y in rx.items():
                        ry = self.rel.get(y)
                        if ry and y != x:
                            found += [(path, p2) for p2, z in ry.items() if z == w]
                T = (p,) + ctx[:len(ctx) - p] + ctx[len(ctx) - p + 1:]
                ex = self.bx.get(T)
                if ex:                          # Hartley: ruled out for good
                    for chain in [ch for ch, rec in ex.items() if rec[0] >= 20 and rec[1] < 0.1 * rec[0]]:
                        del ex[chain]
                if found:
                    if ex is None:
                        ex = self.bx[T] = {}
                    for chain in found:
                        if chain not in ex:
                            if len(ex) >= MAXCHAIN:
                                del ex[min(ex, key=lambda ch: ex[ch][2])]
                            ex[chain] = [0, 0, 0.0]
        self.uni[w] += 1
        self.total += 1
        self._count((1 << 60) | (b + 2), w)
        self._count((2 << 60) | ((a + 2) << 20) | (b + 2), w)

    # ---------- holding words ----------
    def hold(self, held, w, t, started):
        for i in range(len(held) - 1, -1, -1):          # Hartley: the promise is kept; the newest is crossed off
            if self.promise.get(held[i][0]) == w:
                held.pop(i)
                break
        if not (self.o["far"] or self.o["newest"]) or self.worth(w) < WORTH:
            return
        if w not in self.promise:                       # one that promises is held each time it comes
            for i, (k, _) in enumerate(held):
                if k == w:
                    held.append(held.pop(i))
                    return
        held.append((w, t))
        started.append((w, t))
        if len(held) > CAP:                             # Shannon: the one that has saved least goes
            held.pop(min(range(len(held) - 1, -1, -1), key=lambda j: self.worth(held[j][0])))

    def told(self, sent, started):
        """The sentence is over. For each word that was held: what it saved,
        what came after it, and whether its promise was kept."""
        n = len(sent) - 1
        last = {}
        for i in range(n):
            last[sent[i]] = i
        for w, i in last.items():
            self.rest_n[w] = self.rest_n.get(w, 0) + i
        self.pos_n += n
        first = {}
        for k, t0 in started:
            first.setdefault(k, t0)
        for k, t0 in first.items():
            r = self.cue.get(k)
            if r is None:
                r = self.cue[k] = [0, 0.0, 0.0, 0]
            r[0] += 1
            r[2] += (r[1] - r[2]) / min(r[0], 50)
            r[1] = 0.0
            if r[0] == TRY:
                r[3] = 2 * self.uni[k]
            d = self.after.get(k)
            if d is None:
                d = self.after[k] = {}
            h = d[-1] = d.get(-1, 0) + 1
            for w in set(sent[t0 + 1:n]):
                d[w] = d.get(w, 0) + 1
            c = self.cand.get(k)
            if c is not None:                           # was the promise kept? one answer for each, none unasked
                e, open_, ok = c[0], 0, True
                for x in sent:
                    if x == k:
                        open_ += 1
                    elif x == e:
                        open_ -= 1
                        if open_ < 0:
                            ok = False
                            break
                if ok and open_ == 0:
                    c[1] += 1
                    if c[1] >= 10 and c[2] <= 0.01 * (c[1] + c[2]):
                        self.promise[k] = e
                else:
                    c[2] += 1
                    if c[2] > 1 and c[2] > 0.01 * (c[1] + c[2]):    # Hartley: broken, ruled out
                        self.ruled.add((k, e))
                        self.promise.pop(k, None)
                        del self.cand[k]
            elif h >= 8 and h % 4 == 0:
                # a word that has nearly always come later, and far more than it usually comes
                best, bl = None, 1.0
                for w, cw in d.items():
                    if w < 0 or w == k or cw < 0.9 * h or (k, w) in self.ruled:
                        continue
                    base = (self.rest_n.get(w, 0) + 1) / (self.pos_n + 2)
                    e = h * base
                    if cw >= e + 2.0 * math.sqrt(e * (1 - base)) + 1 and (cw / h) / base > bl:
                        best, bl = w, (cw / h) / base
                if best is not None:
                    self.cand[k] = [best, 0, 0]

    def remember(self, sent):
        """The sentence as an episode: each word's neighbours, named by the
        words between, the newest replacing the old."""
        n = len(sent) - 1                               # END is left out
        for i in range(n):
            x = sent[i]
            bx = self.may_bind(x)
            for j in range(i + 1, min(n, i + SPAN + 1)):
                y = sent[j]
                between = tuple(sent[i + 1:j])
                if bx:
                    self._pair(x, (1, between), y)
                if self.may_bind(y):
                    self._pair(y, (0, between), x)

    def _pair(self, x, path, y):
        d = self.rel.get(x)
        if d is None:
            d = self.rel[x] = OrderedDict()
        elif path in d:
            del d[path]
        d[path] = y
        if len(d) > KEEP:
            d.popitem(last=False)

    # ---------- reading ----------
    def read(self, sent, learning, mask=None, classes=None, tally=None):
        """One sentence (END added by the caller). Returns the bits it cost.
        With a mask, the marked words are also guessed within their class."""
        a = b = -1
        held, hist, started, bits = [], [], [], 0.0
        for t, w in enumerate(sent):
            keys, binds = self.speakers(b, held, hist)
            bits -= LOG2(self.guess(a, b, keys, binds, w))
            if mask is not None and t < len(mask) and mask[t]:
                ps = [self.guess(a, b, keys, binds, cw) for cw in classes[mask[t]]]
                best = max(range(len(ps)), key=ps.__getitem__)
                tally[0] += 1
                tally[1] += classes[mask[t]][best] == w
            if learning:
                self.learn(a, b, keys, binds, hist, w)
            self.hold(held, w, t, started)
            hist.append(w)
            a, b = b, w
        if learning:
            self.told(sent, started)
        if self.o["bind"]:
            self.remember(sent)
        return bits


def load(path, END):
    with open(path) as f:
        return [list(map(int, line.split())) + [END] for line in f]


def load_mask(path):
    try:
        with open(path) as f:
            return [list(map(int, line.split())) for line in f]
    except FileNotFoundError:
        return None


def main():
    d, train = sys.argv[1], sys.argv[2]
    task = "task" in sys.argv[3:]
    opts = {k: int(v) for k, v in (x.split("=") for x in sys.argv[3:] if "=" in x)}
    V = int(open(f"{d}/meta.txt").readline().split()[1])
    m = One(V, **opts)
    tr = load(f"{d}/{train}", V)
    words = 0
    for s in tr:                    # one pass: every word guessed before it is seen
        m.read(s, True)
        words += len(s)
    out = f"RESULT one {train}: {len(tr)} sentences, {words} words;"
    if task:
        classes = [None] + [list(map(int, line.split())) for line in open(f"{d}/classes.txt")]
        for name in ("test", "test_long"):
            mk = load_mask(f"{d}/{name}_mask.txt")
            if mk is None:
                continue
            ta = [0, 0]
            for s, k in zip(load(f"{d}/{name}.txt", V), mk):
                m.read(s, False, k, classes, ta)
            out += f" {name}_right {ta[1] / ta[0]:.4f}"
    te = load(f"{d}/test.txt", V)
    bits = sum(m.read(s, bool(m.o['life'])) for s in te)     # life=1: it goes on learning as it reads the test
    n = sum(len(s) for s in te)
    out += f" bits {bits / n:.4f} perplexity {2 ** (bits / n):.1f}"
    out += f" | promises {len(m.promise)} bindings {sum(len(x) for x in m.bx.values())}"
    opt = " ".join(f"{k}={v}" for k, v in opts.items())
    print(out + (f" [{opt}]" if opt else ""), flush=True)


if __name__ == "__main__":
    main()
