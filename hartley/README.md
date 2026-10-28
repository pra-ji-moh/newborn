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

## The abelian corners (Oct 27, 2026)

The switch now decides per pair of words whether order matters (both
orders seen at least 200 times, next two words never differ beyond chance).
Pairs it lets go of are pooled: every ordering of a history counts as the
same evidence. Order kept everywhere is the non-abelian model; no order
anywhere is fully abelian.

Bits per answer word above the floor (0 is perfect):

| | Z6 clean 10k | Z6 clean 30k | S4 clean 30k | Z6 noisy 30k |
|---|---|---|---|---|
| Shannon, order kept | 2.87 | 1.02 | **0.011** | 1.72 |
| Shannon, switch per pair | **0.98** | **0.29** | 0.037 | 1.72 |
| Shannon, order never kept | 0.99 | 0.32 | 0.037 | |
| LSTM (backprop), for scale | 0.001 | 0.002 | 0.002 | 0.08 |

- **Shannon abelian gains about 3x data efficiency** where order truly
  doesn't matter: 10,000 sentences pooled match 30,000 unpooled.
- **The per-pair switch:** on Z6 it found 2 to 3 of the 3 order-free
  pairs; on the grammar it found none, correctly. On S4 it wrongly freed 3
  pairs at 30,000 sentences, which cost a little (0.037 against 0.011). With
  noise it keeps order, and the gain is gone: noise really does make order
  matter a little, because a corrupted word may have been a different one.
- **Hartley abelian fails like Hartley non-abelian:** on sampled language it
  rules out real words.
- The LSTM is still far ahead.

## Grouping histories by their future (Oct 27, 2026)

The author's direction for the Shannon non-abelian corner: the same set
rules as the abelian corner, laid out by Shannon. Histories are pooled when
they have the same future (state merging, after Carrasco and Oncina's
ALERGIA, 1994): Shannon's test says when rates differ beyond chance,
Hartley's veto says when a word seen after one is impossible after the
other. Same data as the sweep above. Raw: `bench/results/lang_grouped.txt`.

Error in bits per word above the floor (answers for S4 and Z6, all words
for the grammar; 0 is perfect), clean data:

| | 300 | 1,000 | 3,000 | 10,000 |
|---|---|---|---|---|
| Grammar: grouped, Shannon + Hartley | **0.005** | **0.0006** | **0.0000** | **0.0000** |
| Grammar: LSTM | 0.080 | 0.025 | 0.011 | 0.004 |
| S4: grouped, Shannon + Hartley | 2.00 | **0.0004** | **0.0001** | **0.0000** |
| S4: LSTM | 1.78 | 0.050 | 0.010 | 0.001 |
| Z6: grouped, Shannon + Hartley | 2.18 | 0.99 | **0.0002** | 0.010 |
| Z6: grouped, Shannon only | 2.34 | 1.88 | **0.0002** | **0.0001** |
| Z6: LSTM | 2.02 | **0.062** | 0.015 | 0.0006 |

With 5% noise, 30,000 sentences: grammar 0.065 (LSTM 0.012), S4 0.16
(LSTM 0.06), Z6 0.37 (LSTM 0.08), best of the two grouped versions.

- **Clean: it beats the LSTM on data efficiency** in most cells, by about
  10 to 30 times on the grammar and S4 (the LSTM needs 10,000 sentences for
  what grouping does with 300 to 1,000). It loses on Z6 at 1,000 and wins
  from 3,000. It stays exact at 120 words. No gradients; seconds of CPU.
- **Hartley's veto is what makes small data work** (S4 at 1,000: 0.24
  without, 0.0004 with). With lots of data it over-splits (Z6 at 30,000:
  44 groups for 12 true states, 0.05 against 0.0000), so the switch should
  lean on it when evidence is thin and let go as evidence grows.
- **Noise is now the gap:** the LSTM is 2 to 5 times better on noisy data.
- **Limits:** the languages are tiny and finite-state; state merging is
  known to suit exactly these. The LSTM got 90 s of CPU per run. Real
  language is far larger, with nesting and long-range meaning.

## Real language: Penn Treebank (Oct 27, 2026)

Newspaper English, 10,000-word vocabulary, 42,068 training sentences.
Training clean or with web-like noise (an estimate; labs don't publish
theirs): 2% of words garbled, 3% of sentences boilerplate, 2% scrambled.
Test always clean. Perplexity (lower is better). LSTM and transformer got
3, 6 and 15 minutes of CPU. Raw: `bench/results/real_sweep.txt`
(`run_real.sh`; the last noisy run was cut off by the 2-hour limit).

| Training sentences | 2,000 clean | 2,000 noisy | 10,000 clean | 10,000 noisy | 42,068 clean |
|---|---|---|---|---|---|
| Grouped, Shannon + Hartley | 639 | 650 | 458 | 473 | 354 |
| Grouped + trigram | 465 | 484 | 284 | 296 | 190 |
| Trigram counting | 444 | 465 | 280 | 292 | 187 |
| LSTM | 458 | 485 | 272 | 280 | 161 |
| Transformer | **431** | **451** | **240** | **252** | **148** |

- **On real language grouping loses to everything**, plain counting
  included, at every size. Real sentence beginnings rarely repeat, so there
  is little evidence to group histories by their future; with the trigram
  as backup it only ties the trigram.
- The web-like noise costs every model about the same (3 to 5%).
- The toy-language advantage does not carry over: those languages had 8 to
  12 hidden situations; real language has no small set.

## Tests 7-10: long-range, nesting, scale, knowledge and writing (Oct 28, 2026)

`gen_struct.py` writes the tests; `run_struct.sh` runs them. The key words
are scored within their class (share right; guessing gets 1 in the class
size). Raw: `bench/results/struct_sweep.txt`, sentences in
`bench/results/struct_samples.txt`.

| Test (share of key words right) | Grouped, Shannon + Hartley | Trigram | LSTM | Transformer |
|---|---|---|---|---|
| Memory: answer set by the first word, 5-20 words back (10,000 sentences) | 11% (chance 12.5%) | 12% | **100%** | **100%** |
| Memory, 50-100 words back | 11% | 15% | 61% | **100%** |
| Nesting: right closing bracket, up to 4 deep (10,000) | 70% | 64% | **98%** | 78% |
| Nesting, 5-8 deep | 64% | 60% | **93%** | 69% |
| Facts: questions about people never asked about (city, or country in two steps) | 26% | 17% | **85%** | **86%** |

- **Long-range meaning:** it cannot carry a word across a gap. Grouping
  by future merges the histories that differ only in the far-back word,
  because the next few words don't depend on it. At chance at every size.
- **Nesting:** a little better than counting, far below the LSTM.
- **Scale** (Penn Treebank, 2,000 to 42,068 sentences): perplexity 639 to
  354 (1.8 times better) while the groups grew 13 times; transformer 431
  to 148 (2.9 times). The gap widens with data.
- **Knowledge and reasoning:** it cannot use a fact from one sentence to
  answer a question in another; each sentence starts from the root.
- **Writing:** sentences drawn from it read like the trigram's: local
  phrases, no sense; the small nets here are not much better.
- **Images:** the model has no way to take in an image. Not testable.
