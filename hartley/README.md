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

## Memory: Shannon stores, Hartley boils down, Hopfield recalls (Oct 28, 2026)

`memory.c`, the author's design: Shannon decides what to store (words that
tell about what comes later, measured by counting); Hartley crosses items
off when their job is done and, when memory is full, rules out the one
that tells least; recall is a dense associative (Hopfield) memory whose
sharpness is the Hartley-Shannon dial; a strong recall is chained one step
("Ann" brings "Paris", which brings "France"); Shannon decides from the
validation text how much to trust memory after each word. Mixed with the
trigram for the next word. No gradients. Prior art: trigger and cache
models (Rosenfeld 1996; Kuhn and De Mori 1990), dense associative memory
(Krotov and Hopfield 2016; Demircigil et al. 2017; Ramsauer et al. 2020).

| Test | Memory model | Grouping (before) | LSTM | Transformer |
|---|---|---|---|---|
| Memory, 5-20 back (right, from 300 / 1,000 / 10,000) | **100% / 100% / 100%** | 11% | 100% (1k+) | 100% |
| Memory, 50-100 back | **100%** at every size | 11% | 61-91% | 100% |
| Facts never asked about | 66% | 26% | **85%** | **86%** |
| Nesting | 64% (no gain over trigram) | 70% | **98%** | 78% |

Real English (Penn Treebank), perplexity, lower is better:

| Training sentences | 2,000 | 2,000 noisy | 10,000 | 10,000 noisy | 42,068 | 42,068 noisy |
|---|---|---|---|---|---|---|
| Memory model | **368** | **381** | **237** | **246** | 165 | 171 |
| Transformer | 431 | 451 | 240 | 252 | **148** | - |
| LSTM | 458 | 485 | 272 | 280 | 161 | - |
| Trigram | 444 | 465 | 280 | 292 | 187 | - |
| Grouping (before) | 639 | 650 | 458 | 473 | 354 | - |

- **Memory solved:** the far-back word is kept and recalled at any gap,
  from 300 sentences.
- **Real English: best at small and middle data, behind at full data.**
  The nets here got 3, 6 and 15 minutes of CPU; well-trained LSTMs reach
  about 80-115 on this data in published work, so at full size the gap to a
  properly trained net is large.
- **Facts:** 66%, up from 26%, still below the nets' 85%.
- **Nesting is not helped:** memory keeps each bracket kind once and does
  not know which was opened last. That needs a stack.

## Nesting: Shannon proposes pairs, Hartley confirms, a stack keeps order (Oct 28, 2026)

Added to `memory.c`. Shannon proposes a pair when a word is nearly always
followed later by one particular word. Hartley confirms or rules out: the
pair must nest in the training text (no closer without an open opener, all
closed by the end) and must never cross a pair already confirmed ("( [ ) ]"),
each allowing 1% of sentences for noise; the end of a sentence is not a
closer. Confirmed openers go on a stack; the top says, with certainty, which
closer comes next.

| Test (key words right) | Memory + stack | Before | LSTM | Transformer |
|---|---|---|---|---|
| Nesting up to 4 deep (300 / 1,000 / 10,000 sentences) | **100% / 100% / 100%** | 64% | 97-98% | 74-78% |
| Nesting 5-8 deep, never seen | **100%** at every size | 60% | 91-93% | 66-69% |
| Memory, 5-20 and 50-100 back | **100%** (more sure: 0.01 bits) | 100% | 61-100% | 100% |
| Facts never asked about | 66% | 66% | **85%** | **86%** |

Real English is unchanged (perplexity 368 / 237 / 165 clean at 2,000 /
10,000 / 42,068). The pairs it confirms there are names ("fannie ... mae",
"freddie ... mac"), not nesting; English nesting (clauses inside clauses)
has no fixed bracket words, which this does not reach.

## Long text through relations (Oct 28, 2026)

The author's design for long text: divide it into pieces (sentences);
Hartley cuts each piece to its surprising words; every ordered pair of kept
words becomes a relation named by the words between them ("lives in",
"likes"), direction kept (non-abelian); Shannon values each relation by its
evidence and by how useful its kind has been, and the memory (64 relations)
drops the least valued when full; questions are answered by following the
chain of relations that has answered that kind of question before, in
order ("lives in" then "is in"). `longtext.c`; test `gen_long.py`: real
newspaper text with facts about people made up per document, questions
50 to 8,000 words later; `run_long.sh`; raw `bench/results/long_sweep.txt`.

| Answers right | 50 words | 500 | 2,000 | 8,000 |
|---|---|---|---|---|
| **Relations (full design)** | **100%** | **100%** | **100%** | **100%** |
| Without relations (trigram only) | 5% | 5% | 4% | 7% |
| Order ignored (abelian) | 82% | 76% | 79% | 79% |
| No priority (forget the oldest) | 19% | 21% | 19% | 18% |
| Transformer, 1,024-word window, 20 min CPU (179 steps) | 4% | 5% | 4% | 6% |
| LSTM, whole stream, 20 min CPU | 7% | 5% | 5% | 6% |

- The full design answers every question (city, country in two steps, who
  likes whom) at every gap, with a fixed memory; about 8 seconds in all.
- The controls show each part is needed: order for "who likes" (a coin
  flip without it), Shannon's priority to keep facts through filler text.
- **The nets did not learn the task in 20 CPU minutes, so this is not a
  fair comparison.** Trained transformers do this kind of look-up within
  their window; beyond it (8,000 here, past 1,024) they cannot by design.
- The facts are in fixed wording ("X lives in Y"); real text says things
  many ways, which this test does not cover.

### Long text, the fairer run (`run_long_fair.sh`, raw `bench/results/long_fair.txt`)

The nets got their best chance: 6,000 short training documents (fact and
question within 0-120 words), a 256-word window and a small transformer
(6,668 steps in 30 CPU minutes), an LSTM (18,314 steps).

| Answers right | 50 words | 500 | 2,000 | 8,000 |
|---|---|---|---|---|
| **Relations (full design, seconds of CPU)** | **100%** | **100%** | **100%** | **100%** |
| Transformer, 256-word window | 12% | 4% | 4% | 3% |
| LSTM | 7% | 6% | 5% | 7% |

- Inside its window (50 words) the transformer begins to learn
  ("who likes" 24%), then falls to chance outside it, as a fixed window
  must. The LSTM did not learn to look back.
- What this shows: on this task the design learns from far less data and
  compute than these nets on this machine. It does not show that a fully
  trained large transformer fails inside its window: such models do this
  kind of look-up well. Beyond the window, a windowed model cannot answer.

## Harder long text (Oct 28, 2026)

`gen_long2.py`: facts in many wordings (some reversed: "C is where P
lives", "Q is liked by P", "X contains K"), moves ("P moved to C"; the
answer is the latest), three-step questions (continent), eight people per
document, and a test in wordings never seen in training. `longtext.c` now
follows chains of up to three relations; a learned Hartley cut keeps words
that turn out to be answers (so the four continents stay though common);
Shannon trusts a chain by times right times how often it is right when
present; a relation in unseen wording counts as "some relation, this way
round". Moves need no rule: the counts learn that "moved to" is the
stronger evidence. `run_long2.sh`; raw `bench/results/long2.txt`.

| Answers right | 50 words | 500 | 2,000 | 8,000 |
|---|---|---|---|---|
| **Relations, seen wordings** | **98.0%** | **97.9%** | **98.1%** | **98.5%** |
| Relations, never-seen wordings | 74.3% | | 71.6% | |
| Transformer, 512-word window, 1 h CPU (4,340 steps) | 9.5% | 7.6% | 7.8% | 8.0% |
| LSTM, 20 min CPU (22,138 steps) | 11.6% | 8.5% | 9.7% | 10.9% |

The CPU-trained nets did not learn the task, so they are not the real
comparison; that is a chat model on the same documents (`bench/gpt_test`).

### Against ChatGPT, GPT-6 (bench/gpt_test)

All 15 documents (240 questions), run by the author in ChatGPT on GPT-6,
Oct 28, 2026, one fresh chat per document; replies scored against the key
(`score_gpt.py`). The relational model ran on exactly the same documents.

| Set | Relations | GPT-6 |
|---|---|---|
| 50 words (3 docs) | **46/48** | 39/48 |
| 500 words | **47/48** | 37/48 |
| 2,000 words | **46/48** | 40/48 |
| 8,000 words | **46/48** | 42/48 |
| Seen wordings, all | **185/192 (96.4%)** | 158/192 (82.3%) |
| Never-seen wordings, 2,000 words | 34/48 (70.8%) | **46/48 (95.8%)** |
| **All 240** | **219 (91.3%)** | 204 (85.0%) |

Where GPT-6 went wrong (`gpt_errors.py`, `bench/gpt_test/gpt6_by_wording.txt`):

| Kind of question | GPT-6 |
|---|---|
| "Who likes Q?" when the fact is active ("P likes Q") | 40/44 (91%) |
| "Who likes Q?" when the fact is passive ("Q is liked by P") | **6/14 (43%)** |
| One step (where someone lives), any wording, moves included | 62/62 (100%) |
| Two steps, plain | 11/11 (100%) |
| Two steps after a move and through a reversed fact | 1/4 |
| Three steps after a move | 15/25 (60%) |

- GPT-6 never missed a one-step question. Its errors cluster where a fact
  is worded in reverse (passive "is liked by": wrong more often than right)
  and where a chain of three facts runs through a move. Distance did not
  hurt it: 8,000 words was its best set.
- The relational model was 42/42 on "who likes" in the wordings it had
  seen, and 96% on seen wordings overall; on wordings never seen in
  training it fell to 71%, where GPT-6 scored 96%.
- Fair reading: the relational model learned from 1,500 documents of this
  kind and reads questions in its own short form; GPT-6 saw none and read
  natural questions. One run of GPT-6 per document; its answers may vary.

## Learning the newborn's way: from 0, new words, passives (Oct 29, 2026)

`newborn_lang.c`: the relational model, learning as the newborn does
(`brain/ob_novel.c`, `research/RESEARCH.md`).

- **From 0, serve and return.** Nothing known at the start. Documents come
  one at a time in a single pass; every question is answered before its
  answer is seen, then the answer comes and the counts take it in. Questions
  are marked, as a caregiver's are: a sentence holding "?" is a question and
  the next word its answer. Kept words: surprising ones (a word never met is
  surprising) and the ones questions ask for. A word's kind is which
  questions it has answered; each relation remembers the kinds at its ends.
- **New words: ideas nobody writes.** A relation in wording met fewer than 3
  times, between words that questions ask for: the DMN/MFG propose "it is
  like <a useful relation>, this way round or reversed"; the ACC rules out an
  idea whose ends don't fit the kinds that relation joins (Hartley); the
  DLPFC holds the strongest survivors, weighted by Shannon.
- **Passives: Wernicke.** Which way a wording runs is learned from its last
  word, by comparing each wording's direction with the main one for the
  same question. It learned that a wording ending in "by" runs reversed.
  When the kinds at the ends settle the direction (a city and a person),
  they decide; the frame word decides when they cannot (two people).

New test (`gen_long3.py`): every fact in wording never seen in training,
half of the "likes" facts passives of verbs never seen ("Q is adored by P",
"Q is respected by P"). Raw: `bench/results/newborn_lang.txt`
(`run_newborn.sh`).

| Answers right, held-out tests | after 30 docs | 100 | 300 | 1,000 | 1,500 |
|---|---|---|---|---|---|
| Seen wordings, 50 words away | 57% | 75% | **99.5%** | 99.1% | 98.5% |
| Seen wordings, 8,000 words away | 46% | 72% | **99.9%** | 99.6% | 99.0% |
| Passives and wordings never seen | 9% | 6% | 32% | 83% | **97.0%** |
| Other wordings never seen | 8% | 11% | 35% | 62% | **73.3%** |

First tries during the stream (answering before the answer comes): 74% on
documents 31-100, 91% on 101-300, 99.7% on 301-1,000.

| At 1,500 documents | Passives never seen | "Who likes" there | Other new wordings |
|---|---|---|---|
| Full design | **97.0%** | **93.7%** | **73.3%** |
| No ideas for new wordings | 22.3% | 5.5% | 24.5% |
| Ideas, but no Wernicke frame word | 88.0% | 60.2% | 69.8% |

- From nothing, one pass: 300 documents bring seen wordings to 99-100% at
  every distance (better than longtext.c's 98% with several passes).
- Passives of verbs never seen: 97%. For comparison, GPT-6 answered "who
  likes" right 6 times in 14 when the fact was a passive it *had* seen
  wording for (bench/gpt_test); this set has not been run on GPT-6.
- Wernicke's frame word is what gets the direction of new passives: without
  it "who likes" on them is a coin flip (60%); with it, 94%.
- Weakest: continents in other new wordings (45%): "X holds K" and "K lies
  in X" have no frame word that tells their direction, and the kinds at
  both ends are places.

### Against Claude (bench/gpt_test, bench/gpt_test_pas)

The same 15 documents GPT-6 answered, plus 3 from the passive set, each
given to a fresh Claude agent (Oct 29, 2026), which read only its prompt file - the answer keys were in
another folder. Replies in `claude_*.txt`, scored with
`score_gpt.py <dir> claude_`.

| | Relations / newborn model | GPT-6 | Claude |
|---|---|---|---|
| Seen wordings (12 docs, 192 questions) | 185 (96.4%) | 158 (82.3%) | **192 (100%)** |
| Never-seen wordings (3 docs, 48) | 34 (70.8%) | 46 (95.8%) | **48 (100%)** |
| Passives and wordings never seen (3 docs, 48) | 47 (97.9%, newborn_lang.c) | not run | **48 (100%)** |
| **All** | | 204/240 | **288/288** |

Claude answered every question right, passives and moves included. On
these tests the models here beat GPT-6 but not Claude.

### Checking the test, and a rerun (bench/rerun)

Claude's full marks made us check the test itself:

- **The agents saw only their prompt.** Each agent's record shows a single
  read of its own prompt file and nothing else. The keys were in another folder.
- **The keys are right.** `verify_key.py` reads every fact back out of each
  prompt's text, in every wording. It applies the latest move, answers the
  questions on its own and compares with `key.txt`. It found 0 disagreements
  on `gpt_test` (240 answers), `gpt_test_pas` (48) and `rerun` (288).
- **Rerun on new documents.** We took documents 4-6 of every set (none used
  before, `START=3 python3 gpt_export.py ...`) and gave them to 18 fresh
  agents and to the newborn model:

| set | newborn model | Claude |
|---|---|---|
| 50 | 48/48 | 48/48 |
| 500 | 48/48 | 48/48 |
| 2,000 | 47/48 | 48/48 |
| 8,000 | 48/48 | 48/48 |
| new wordings, 2,000 | 37/48 | 48/48 |
| passives, 2,000 | 48/48 | 48/48 |
| **all** | **276/288 (95.8%)** | **288/288** |

Nothing was wrong with the test. Claude simply answers these documents
perfectly. The newborn model matches it on wordings it has met, and on
passives, which it learns from 0 in one pass. It still loses on new
wordings, its one clear gap. GPT-6 was not rerun; its scores are from the
first 15 documents only.
