"""A layer of neurons that learns with no backprop (Nov 14, 2026).

Counting cannot say that two words are *nearly* alike; kinds (grow.py) say
it only crudely. This is the missing piece tried the way the newborn's brain
does it (research: the delta rule of the cerebellum, a chemical broadcast):

  numbers   each word, at each place back, has a short list of numbers; they
            start as the likeness numbers counted in grow.py
  hidden    one layer of neurons reads the numbers of the last BACK words
  guess     an output neuron for every word; their activity is the guess
  learning  the word comes. Each output neuron's error (its guess minus what
            happened) changes its own synapses: the delta rule, local. The
            same errors are broadcast to the hidden neurons and to the
            word numbers through fixed random wiring, never through the
            output synapses and never backwards layer by layer (direct
            feedback alignment: Nokland 2016; Lillicrap et al. 2016). Each
            synapse changes by what is at its two ends and the broadcast
            it receives.

mode="bp" trains the same layers by backprop instead: the yardstick, to see
what the broadcast costs. Nothing else differs.
"""
import numpy as np

F = np.float32


SIG = 0.05          # team game: size of each trial change
DECAY = 1e-3        # learn: forgetting per step
ELIM_EVERY, ELIM_N = 200, 8   # elim: how often, and how many players go


def train(ctx, w, NV, init, hidden=256, epochs=4, batch=256, rate=0.3, mode="dfa", seed=0, say=None, also=()):
    """ctx: (n, places) word ids; w: (n,) the word that came; init: for each
    place, (NV, dims) starting numbers. Returns a function giving the chance
    of each word that came on another text."""
    rng = np.random.RandomState(seed)
    places, dims = ctx.shape[1], init[0].shape[1]
    E = [x.astype(F).copy() for x in init]
    nin = places * dims
    W1 = (rng.standard_normal((nin, hidden)) / np.sqrt(nin)).astype(F)
    b1 = np.zeros(hidden, dtype=F)
    W2 = np.zeros((hidden, NV), dtype=F)
    b2 = np.log(np.bincount(w, minlength=NV) + 1.0).astype(F)
    b2 -= b2.mean()
    Bh = (rng.standard_normal((NV, hidden)) / np.sqrt(hidden)).astype(F)       # fixed wiring for the broadcast
    Bx = (rng.standard_normal((NV, nin)) / np.sqrt(nin)).astype(F)
    n = len(w)
    g2 = np.full((hidden, NV), 1e-3, dtype=F)                                  # each synapse keeps its own pace
    B1 = (rng.standard_normal((hidden, nin)) / np.sqrt(hidden)).astype(F)     # "learn": the guess for the layer below
    paid, steps, eliminated = np.zeros(hidden, dtype=F), 0, 0                  # "elim": each player's payoff
    for ep in range(epochs):
        order = rng.permutation(n)
        lr = rate / (1 + ep)
        loss = 0.0
        for a in range(0, n, batch):
            i = order[a:a + batch]
            m = len(i)
            x = np.hstack([E[j][ctx[i, j]] for j in range(places)])
            h = np.tanh(x @ W1 + b1)
            z = h @ W2 + b2
            z -= z.max(1, keepdims=True)
            p = np.exp(z)
            p /= p.sum(1, keepdims=True)
            p_y = p[np.arange(m), w[i]].copy()
            loss -= np.log(p_y).sum()
            e = p
            e[np.arange(m), w[i]] -= 1.0                                       # each output neuron's error
            if mode == "bp":
                dh = (e @ W2.T) * (1 - h * h)
                dx = dh @ W1.T
            elif mode == "learn":
                dh = (e @ Bh) * (1 - h * h)                                    # each player's guess at its payoff, learned below
                dx = dh @ B1
            elif mode == "team":
                # every neuron is paid the same: how well the whole net did. Each tries a small
                # random change and keeps the direction that paid (node perturbation).
                zx = rng.standard_normal(x.shape).astype(F)
                zh = rng.standard_normal(h.shape).astype(F)
                h2 = np.tanh((x + SIG * zx) @ W1 + b1) + SIG * zh
                z2 = h2 @ W2 + b2
                z2 -= z2.max(1, keepdims=True)
                lse2 = np.log(np.exp(z2).sum(1))
                r = (lse2 - z2[np.arange(m), w[i]]) + np.log(p_y)               # the team's payoff change, one number per moment
                dh = (r[:, None] * zh / SIG) * (1 - h * h)
                dx = r[:, None] * zx / SIG
            else:
                dh = (e @ Bh) * (1 - h * h)                                    # the broadcast, through fixed wiring
                dx = e @ Bx
            gw2 = h.T @ e / m
            g2 += gw2 * gw2
            s2 = lr * gw2 / np.sqrt(g2)
            W2 -= s2
            b2 -= lr * e.mean(0)
            s1 = lr * 0.5 * (x.T @ dh) / m
            W1 -= s1
            b1 -= lr * 0.5 * dh.mean(0)
            if mode == "learn":
                # each player sees the same local signals as its synapses (its own firing and the
                # broadcast), so it changes its guess by the same step; forgetting a little each
                # time removes the wrong starting guess (Kolen and Pollack 1994).
                Bh -= s2.T
                B1 -= s1.T
                for M in (W1, W2, Bh, B1):
                    M *= 1 - DECAY
            if mode == "elim":
                # each player's marginal contribution: how much worse the guess would be without it
                paid += -(h * (e @ W2.T)).sum(0)
                steps += 1
                if steps % ELIM_EVERY == 0 and ep < epochs - 1:
                    out = np.argsort(paid)[:ELIM_N]                            # dominated: contributed least
                    W1[:, out] = (rng.standard_normal((nin, len(out))) / np.sqrt(nin)).astype(F)
                    b1[out] = 0
                    W2[out] = 0
                    g2[out] = 1e-3
                    eliminated += len(out)
                    paid[:] = 0
            for j in range(places):
                np.subtract.at(E[j], ctx[i, j], lr * 0.5 * dx[:, j * dims:(j + 1) * dims])
        if say:
            extra = "".join(f"; {name} {2 ** -np.log2(chance(E, W1, b1, W2, b2, c2, w2)).mean():.1f}" for name, c2, w2 in also)
            say(f"    neurons ({mode}{f', {eliminated} eliminated' if mode == 'elim' else ''}), time through {ep + 1}: {np.exp(loss / n):.1f} on what it has read{extra}")
    return lambda c2, w2: chance(E, W1, b1, W2, b2, c2, w2)


def chance(E, W1, b1, W2, b2, ctx, w, batch=2048):
    out = np.empty(len(w))
    for a in range(0, len(w), batch):
        c = ctx[a:a + batch]
        x = np.hstack([E[j][c[:, j]] for j in range(len(E))])
        z = np.tanh(x @ W1 + b1) @ W2 + b2
        z -= z.max(1, keepdims=True)
        p = np.exp(z)
        out[a:a + batch] = p[np.arange(len(c)), w[a:a + batch]] / p.sum(1)
    return out
