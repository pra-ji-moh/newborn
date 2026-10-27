# Hartley Non-Abelian Model: first test

The author's idea: possibilities held as *ordered transformations*, not sets,
and eliminated Hartley-style (1928): every survivor counts the same, nothing
is weighed, and what is still unknown is log2 of what is left. No gradients.

This folder is its smallest version that could fail. It is plain C with no
frameworks, and it is the algorithm on its own, not yet in neurons.

## The test

- **The world:** five slots, three colours. Three acts, each a hidden
  permutation of the five slots. Together the acts generate all of S5 (120
  elements, checked for every world), so order matters.
- **What it sees:** episodes of a start, the acts done, and the end. Never
  the steps in between.
- **What it holds:** for each act, the permutations it could still be (120
  each at birth). Each remembered episode rules out every combination that
  would not carry its start to its end *in the order the acts were done*.
  Remembered episodes are re-checked until nothing more falls.
- **Held-out test:** 100 fresh sequences each of 10, 100 and 1000 acts per
  checkpoint. It claims an end only when every surviving possibility agrees.
- **Control:** the identical model with order thrown away (an episode is a
  set of acts). That is the limit the grown brain hit.

Build and run: `cc -std=c99 -O2 hnam.c -o hnam -lm && ./hnam 50 0` (mode 1:
`./hnam 50 1`, about 5 minutes). Raw outputs: `bench/results/hnam_mode*.txt`.

## Results (Oct 27, 2026, 50 worlds per mode)

**Mode 1, the hard one: no single steps, episodes of 3 to 5 acts, start and
end only.**

| Episodes seen | Bits left (ordered) | Worlds tracked exactly (ordered) | Order-blind |
|---|---|---|---|
| 5 | 18.9 | 1/50 | 1/50 claimed, mostly wrong (15 of 300 right) |
| 20 | 6.4 | 34/50 | contradicted in 35/50, claims nothing |
| 40 | 1.2 | 47/50 | contradicted in 47/50 |
| 80 | 0 | 50/50 | contradicted in 50/50 |

- **Ordered:** every act pinned exactly in 50/50 worlds, after 19 episodes on
  average. From then on, 1000-act sequences are tracked exactly every time.
  0 wrong claims in all 90,000 test sequences.
- **Order-blind:** it reaches a contradiction in every world (its own
  evidence rules out every possibility), and before that it makes confident
  wrong claims (95% of its early claims are wrong).

**Mode 0, short episodes first (1 to 2 acts, then 1 to 5):** ordered pins
every act in 50/50 worlds after 10.4 episodes, 0 wrong claims. Order-blind
contradicts itself in 50/50 worlds and, before that, made 958 wrong claims
out of 1,047.

## What this does and doesn't show

- **Shows:** keeping order is the difference between solving these worlds
  exactly and failing every one. Throwing order away is not a small loss: it
  makes the model confidently wrong, then self-contradictory.
- **Zero wrong claims is by construction.** The world is noise-free and the
  elimination is sound, so the true answer is never ruled out. That is a
  property of the method, not an achievement to sell.
- **It is handed the shape of the hypotheses:** it knows each act is a
  permutation of five slots. Nobody wrote its answers, but the space was
  given. The grown brain's rule (no handed list) is not met here yet.
- **One wrong observation breaks it.** Pure elimination rules out the truth
  on noisy data. Real worlds are noisy; this needs an answer.
- **Not in neurons yet,** and not yet on ARC-AGI-3 style games.
- **No frontier comparison yet.** The reports that fixed-depth transformers
  can't exactly track non-abelian state (S5) are still unverified here; the
  head-to-head on this exact test is the next step.

# The four domains on language (Oct 27, 2026)

The author's idea: Hartley or Shannon, abelian or non-abelian, with Shannon
as the pivot. Tested on the task ChatGPT and Claude are trained on: predict
the next word. `lang.c` makes three hidden languages (a grammar; S4 tracking,
where after "?" the next word must say where item 0 is; Z6 counters, which
commute), clean and with 5% of words swapped, and knows the floor exactly.
Models with no gradients: Shannon non-abelian (words as matrices, a sentence
is their product in order, matrices from counts by spectral learning: Hsu,
Kakade and Zhang 2009, Balle et al. 2014), Hartley non-abelian, the pivot,
and trigram counting. Opponents trained by backprop (`lang_nets.py`): a
transformer and an LSTM, 90 s each on 4 CPU cores. Run all: `./run_lang.sh`.
Raw output: `bench/results/lang_sweep.txt`.

Bits per answer word above the floor (0 is perfect; answers are where the
hidden state must be tracked), clean data:

| Training sentences | 1,000 | 3,000 | 10,000 | 30,000 |
|---|---|---|---|---|
| S4: Shannon non-abelian | 1.43 | 0.38 | 0.07 | **0.011** |
| S4: LSTM | **0.05** | 0.010 | 0.001 | 0.002 |
| S4: transformer | 1.77 | 1.58 | 1.60 | 1.64 |
| Z6: Shannon non-abelian | 3.99 | 2.94 | 2.87 | 1.02 |
| Z6: LSTM | **0.06** | 0.015 | 0.001 | 0.002 |

Grammar, all words, clean, 30,000 sentences: Shannon non-abelian 0.49,
trigram 0.17, transformer 0.20, LSTM 0.005.

- **The pivots work.** Noise: clean and noisy were told apart in 30 of 30
  runs. Order: grammar and S4 called non-abelian in every run with enough
  data; Z6 called abelian in 6 of 8 runs from 1,000 sentences up (wrong at
  30,000 noisy, 6.6% against a 5% line). With 300 sentences there is too
  little to judge.
- **Order as matrix products, with no gradients, learns hidden state:** S4
  exactly at 30,000 sentences, 8 bands, 17 s of CPU, exact at 120 words.
  Counting without order (trigram) never does. The small transformer never
  does in this budget.
- **It loses to the LSTM everywhere.** The LSTM needs about 30 times less
  data on S4, solves Z6 and the grammar, and copes with noise; the spectral
  learner is worse than trigram on the grammar and degrades with noise.
- **Hartley on sampled language rules out real words** (thousands per run):
  in a sample, never seen is not impossible. Hartley is safe only where the
  evidence is exhaustive, as in the S5 tests.

**Verdict:** the four-domain frame is sound (the pivots fire correctly, and
order-as-product learns without gradients). It does not yet replace backprop:
the learning engine for the Shannon non-abelian corner (spectral learning
here) is far less data-efficient than an LSTM. That engine is the open
problem.
