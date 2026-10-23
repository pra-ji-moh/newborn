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
2. **Its own hypotheses.** Today it is handed the list of possible rules; so
   was the frontier model. A cognitive human invents them. Without this,
   nothing generalises to ARC-AGI-3. This is the core breakthrough to show.
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
