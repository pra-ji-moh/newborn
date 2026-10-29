# The plan

**Goal:** a brain grown from the author's research (`research/RESEARCH.md`)
that competes with the frontier models (Anthropic's and OpenAI's) across the
board: reasoning, perception, language, and learning for life.

**Rule:** every part is built as the research describes it, at full fidelity,
and every big idea becomes a test that could fail. Results are written down
as they come, including losses.

## Focus: ARC-AGI-3

The headline target, because it measures exactly what a cognitive human has
and frontier models lack. ARC-AGI-3 (ARC Prize, March 2026): 135 interactive
game worlds with no instructions; the player must explore, work out the rules
by acting, and win, efficiently. Every environment was solved by untrained
humans; frontier models score under 1% (reported: Gemini 3.1 Pro 0.37%,
GPT-5.4 0.26%, Claude Opus 4.6 0.25%; best community agent 12.58%). Scores
from secondary reports; check ARC Prize's own leaderboard before quoting.

Why it fits this design: the newborn already acts of itself, is driven by
learning progress (curiosity), keeps every moment, and reasons by Hartley
elimination; baby Ciall's mind (`mind/`) already plays practice worlds of
movers, walls, colours and level endings. ARC-AGI-3 is the same kind of
problem.

**What counts as a result for investors:** a score on ARC Prize's public and
semi-private sets, run the way they run it, reproducible from this repo, with
the frontier models' scores beside it. Internal tests are for us; they are not
claims.

### Milestones
1. **Memory of results in the grown brain** (hidden-rule test): match the
   ideal solver at 11,700 rules. Measured in the Python model (Oct 23): with
   working memory of 39 and each idea checked against remembered results when
   it comes to mind, 15.5 tests (ideal 15, frontier model 14), 8/8 correct.
   Must be shown in the C brain, in hippocampal neurons.
2. **Novelty from conditions, never from a list.** Today it is handed the
   list of possible rules; so was the frontier model. No idea it claims may be
   written for it, and no idea-generator either. Instead, grow the conditions
   the research names, and let ideas arise from its own activity:
   - **Middle frontal gyrus:** fluid reasoning, devising novel strategies;
     the hub where the DAN (top-down) and VAN (the unexpected) meet, and
     where the limbic system comes in.
   - **Broca's area and the FEF, hyper-direct** (frontal hypothesis 1):
     deeper, heavily myelinated connections straight into the central
     sulcus, for clarity of novel ideas, paired with eye speed.
   - **The larger picture:** the collateral sulcus / PPA feeding the middle
     frontal gyrus (problem solving by seeing the whole).
   - **The default mode network** (anteromedial SFG, angular gyrus): idle
     activity that recombines what it has lived.
   - **The pull:** dopamine as learning progress, so what is new and
     learnable draws it and what it knows bores it.
   - **Selection, not authorship:** whatever arises is held by the DLPFC and
     tested by the ACC against remembered results (Hartley); what survives,
     stays.
   **Tests that could fail:** handed no list, it finds hidden rules it was
   never told the form of; switching the middle frontal gyrus off (coded
   optogenetics) stops that; without the hyper-direct links, it is slower.
   This is the core breakthrough to show. The handed list stays only as a
   yardstick in the old test.

   **First result (Oct 23, `brain/ob_novel.c`, `brain/test_novel.c`, 8/8
   checks).** One life to three years, chemistry dopamine / oxytocin /
   cortisol (no reward critic). Handed no list:

   | | found | tests (mean) |
   |---|---|---|
   | whole, six familiar forms | 6/6 | 34 |
   | new forms (two of three acts; an act anywhere on the wall) | 2/2 | 41, 36 |
   | "this act but not that" (one idea cannot stand for it) | not claimed | |
   | no middle frontal gyrus | 4/6 | 27, at 21.5 moments a test (whole: 7.9) |
   | no default mode network | 0/6 | |
   | no scene area | 6/6 | 40 |
   | no hyper-direct links | 5/6 | 36, at 8.0 moments a test |
   | no ACC / no memory of results | 0/6 / 0/6 | |
   | old circuit, handed the list of 144 | 6/6 | 11 |

   Honest limits: one life, one box per rule, so small numbers; the
   hyper-direct links' effect on speed is barely there (8.0 against 7.9
   moments a test); not having the list costs about 3x the tests; ideas'
   content is formed from features in code, held and tested in neurons
   (ob_novel.h says which is which). The search was improved three times
   by watching where it failed (exploring a silent new box; checking an idea
   at both edges; merging thoughts that foretold a chime); each was a
   condition from the research, not an answer written in, and the boxes it
   was improved on are the boxes it is scored on. A fresh set of rules is
   needed to score it fairly.

   **Held out** (`brain/fresh_rules.c`, six rules never used in
   development): 4/6 found (act 2 at a quarter, 48 tests; acts 0 and 3
   together, 28; act 1 or 2, 38; act 1 anywhere, 45). Not found: two of
   three acts at a quarter, and two acts together anywhere. No wrong claims.
   This 4/6, not the 8/8, is the number to quote.

   **Second result: ideas on synapses, scored wide (Oct 23,
   `brain/test_wide.c`).** An idea is now the synapses from real neurons
   (each act's brainstem zone, each quarter of the tectum's gaze map) onto
   a group of DLPFC neurons with its own threshold; failure (cortisol) can
   teach inhibition ("not with that"). Three lives grown from three genome
   seeds, 35 boxes each drawn at random from a seed never used in
   development (`test_wide LIFE GENOME_SEED 35 2026`), 105 boxes in all:

   | form | found | wrong claims | median tests |
   |---|---|---|---|
   | one act at a quarter | 9/9 | 0 | 43 to 57 |
   | both of two | 14/15 | 0 | 42 to 51 |
   | either of two | 18/18 | 0 | 39 to 51 |
   | two of three | 6/12 | 0 | 52 to 59 |
   | one act anywhere | 14/18 | 0 | 53 to 65 |
   | one act but not with another | 5/15 | 0 | 59 to 78 |
   | exactly one of two (cannot be thought) | 0/18, never claimed | 0 | |

   66 of the 87 thinkable rules found (76%); 0 wrong claims in 105. The
   number to quote. Against the yardstick: told the family and handed the
   list, the ideal solver needs about 7 to 15 tests on these sizes and the
   frontier model matched it; this, told nothing, needs about 50.
3. **ARC-AGI-3 public set**, through its own eyes and acts, then the
   semi-private set.
4. Everything else in the tracks below, on the way.

## Where it starts (Oct 23, 2026)

| | |
|---|---|
| Grown brain | 13,000 neurons, 396,000 connections, grown from a genome; lives through the womb, then in the crib |
| Mind | baby Ciall's cognition (`mind/`), still separate from the brain |
| Hidden-rule test, 11,700 rules | frontier model 14 tests; ideal 15; our frontal cortex grown 369; **with memory of past results, 13** (Python model, not yet in the C brain) |
| Phone | runs the small life brain, not the grown one |

The memory result matters: with the parahippocampal gateway and hippocampus
keeping every result and checking each idea against them, the design matches
or beats the frontier model on this test. It has to be built into the grown
brain to count.

## Hartley Non-Abelian Model (Oct 27, 2026)

The author's idea: possibilities held as ordered transformations, not sets,
eliminated Hartley-style. First test in `hartley/` (plain C, not in neurons
yet): hidden S5 acts, start and end of each episode only.
**Result (50 worlds, episodes of 3 to 5 acts):** ordered pins every act in
50/50 worlds after 19 episodes on average, then tracks 1000-act sequences
exactly; 0 wrong claims in 57,600. The same model with order thrown away
contradicts itself in 50/50 worlds, and 285 of its 300 claims were wrong.
**Limits:** zero wrong claims is by construction (noise-free, sound
elimination); the hypothesis shape (permutations of five) is handed to it;
one noisy observation breaks pure elimination; no frontier comparison yet.
**Next:** the same test against a frontier model told nothing; then noise;
then into the DLPFC's neurons.

## Tracks, all at once

Each track lists the research it builds and the test that decides it.

### A. Growth: the timeline at full fidelity (Part 2)
BMP blocking by Noggin/Chordin/Follistatin; apical constriction and zipper
closure; neuropores on days 25 and 27; 3 then 5 vesicles; the ventricular
zone with interkinetic nuclear migration; symmetric then asymmetric division
at week 7; radial glia climbing (astrotactin); inside-out layers; growth
cones guided by netrins, semaphorins, ephrins and CAMs; the neurexin–neuroligin
handshake; 50% overproduction and death by neurotrophin competition
(NGF, BDNF); folding from axon tension; oligodendrocytes and the myelination
wave; thalamocortical loops at weeks 26–28.
**Tests:** the grown anatomy shows each mechanism's signature (layer order by
birth date, about half of neurons pruned, folds where tension is highest,
myelin order brainstem → sensory → motor → prefrontal); and the brain grown
this way does at least as well on every later test as the current grown brain.
Coded optogenetics: grow chosen parts more, and measure what each is worth.

### B. Reasoning: parietal and frontal (Parts 4–5)
IPS (number line, estimation, saliency, saccade priority), SMG, angular gyrus;
the frontal hub hypotheses (deep links between the superior frontal sulcus,
inferior frontal groove, dorsolateral SFG and DLPFC; Broca and the FEF
hyper-direct into the central sulcus); Hartley in the DLPFC; the ACC;
parahippocampal and hippocampal memory of results; proposing its own
hypotheses.
**Tests:** hidden-rule ladder at or under the frontier model's test counts,
in the grown brain; then ARC-style puzzles, against frontier scores.

### C. Perception: temporal and occipital (Part 6)
ITG and inferior temporal sulcus (isolating an object's configuration),
postcentral sulcus by touch (stereognosis), fusiform, parahippocampal place
area, STS, Heschl's gyrus / A1, STG.
**Tests:** viewpoint-invariant recognition and object identity by touch
against standard small vision tasks; sound and phoneme recognition.

### D. Language (Part 6, frontal)
Wernicke's area, Broca's area (pars opercularis, triangularis, orbitalis),
MTG lexicon, SMG phonological loop, angular gyrus reading, visual word form
area; Sylvian fissure as the language highway.
**Tests:** learning words from heard and seen examples, then sentence
structure ("the dog chased the cat"), then standard language benchmarks
against frontier models. This is the longest track: frontier models learn
from trillions of words. The brain must learn far more from far less.

### E. Learning for life (Parts 3, 7)
Cerebellar folia with LTD and climbing-fibre error, predictive schemas;
myelination with use; pruning; the muscle effect.
**Tests:** keeps learning new tasks without losing old ones, where frontier
models are frozen.

### F. Scale
The brain grows past 13,000 neurons as the tracks need. Experiments run on a
server; the phone receives the grown brain.

## Order of work

1. Move everything here (done: `brain/`, `mind/`, `phone/`, `bench/`).
2. Memory of results into the grown brain (B), since it is the one place the
   design already matches the frontier model.
3. Growth mechanisms (A), stage by stage, re-running every test after each.
4. Parietal and frontal regions (B), then the mind as the cortex.
5. Perception (C) and language (D) in parallel, cerebellum (E) throughout.
6. Benchmark against frontier models after every step.
