/*
 * ob_novel.h -- the conditions for novelty: ideas nobody writes, formed on
 * synapses.
 *
 * Nothing hands it what the hidden rule could be: no list of rules, no
 * generator of them, no form an idea must take. Ideas are formed on
 * synapses, in the places the research names, from its own activity, and
 * kept only if they survive what it remembers.
 *
 * WHAT AN IDEA IS
 *   A group of its DLPFC neurons and the synapses that reach it:
 *     from what it did     the brainstem's act zones (each act a population
 *                          of its own neurons, the efference copy), onto
 *                          the group's basal dendrites; excitatory, or
 *                          inhibitory through the group's local interneurons
 *     from where it looked the tectum's map of where its eyes point (each
 *                          quarter of the wall a population), onto the
 *                          group's apical dendrites: a context that gates
 *                          (a pyramidal neuron's apical input lets its basal
 *                          input fire it)
 *   The group fires for a moment, lived or imagined, when the context is
 *   there and its basal drive crosses its own threshold. The threshold is
 *   the group's, not the idea's: groups differ by the layer their neurons
 *   sit in (here, set by layer: a modelling choice, said plainly). So an
 *   idea is whatever these synapses and this threshold make the group fire
 *   for: "this act there", "both", "either", "two of three", "this but not
 *   that one", "this one with either of those". No list of these exists
 *   anywhere; they are what synapses onto one group can do.
 *   What one group cannot fire for (exactly one of two acts; an order in
 *   time) it cannot think yet.
 *
 * WHERE IDEAS FORM (the research, Parts 4 to 6)
 *   MFG     the middle frontal gyrus: fluid reasoning, devising novel
 *           strategies. Where the dorsal attention network (where its eyes
 *           were) and the ventral (what was unexpected) meet. When the VAN
 *           flags something unexpected, the MFG opens a binding window:
 *           synapses from the populations active now onto a free group
 *           potentiate, each population's burst reaching it or not, and
 *           each synapse taking or not. When something new does nothing,
 *           it binds what it just did, varied. When an idea it held failed,
 *           it binds the failed idea again, with inhibition from what was
 *           there besides (cortisol: failure teaches "not when that")
 *   PPA     the collateral sulcus's scene area: the larger picture. It can
 *           widen an idea's context to every quarter it has heard the box in
 *   DMN     the default mode network (anteromedial superior frontal gyrus,
 *           angular gyrus): idle, its hippocampus replays two moments into
 *           the populations; synapses take where both were active (what they
 *           share) or where either was; or two groups that once foretold a
 *           chime are reactivated together and a free group takes their
 *           synapses. The common sense pivot: ideas made of its own life
 *   Broca   Broca's area and the frontal eye fields, wired hyper-directly
 *   + FEF   into the motor strip: an idea becomes the act that tests it at
 *           full strength. Without the links, as the frontal myelin carries it
 *
 * SELECTION, NOT AUTHORSHIP
 *   A group just bound is tested before it is held: the hippocampus replays
 *   each remembered test into the populations, and the ACC compares what the
 *   group does with what came; contradicted, its synapses are let go. What
 *   survives is held by the group's self-excitation (ob_frontal.c's
 *   dynamics: how many it can hold grows with myelin). The IPS picks the
 *   test that splits what is held most evenly; holding one, it tests its
 *   edges. Each outcome: the ACC darkens what it contradicts.
 *
 * ITS CHEMISTRY (no reward, no punishment)
 *   cortisol   failure: it laid down the memory more strongly; the VAN flags
 *              more (more bindings); high, it weakens the DLPFC's hold;
 *              and its failures are what teach inhibition
 *   dopamine   success (a chime foretold): steadies that group, makes its
 *              synapses' binding stronger, strengthens the pathway it came by
 *   oxytocin   success with its mother: calms cortisol, strengthens memory
 *
 * Coded optogenetics: MFG, PPA, DMN, the hyper-direct links, the ACC, the
 * hippocampus's memory of results, or the inhibition can be switched off.
 *
 * What is in neurons and what is not, plainly: an idea's content is its
 * synapses (from real neurons of its brainstem and tectum onto real DLPFC
 * neurons), and what it predicts is what those synapses make the group do.
 * Holding is the groups' activity. The binding synapses are kept here beside
 * the brain, not among its grown synapses; the remembered tests are a list
 * (the hippocampus's part); when to bind is decided by the events named
 * above, not by neurons of the VAN.
 */

#ifndef OB_NOVEL_H
#define OB_NOVEL_H

#include "ob_box.h"
#include "ob_one.h"

#define ON_HELD 64                    /* DLPFC groups */
#define ON_MEMORY 4096                /* results remembered in one meeting */
#define ON_K 8                        /* neurons in each population an idea's synapses come from */
#define ON_FEAT (OL_MOTORS + 4)       /* the populations: each act, each quarter of the wall */

enum { ON_MFG, ON_PPA, ON_DMN, ON_SOURCES };

/* frontal areas, as ob_frontal's by position, with the research's added */
enum { ON_OTHER, ON_DLPFC, ON_ACC, ON_OFC, ON_FEF, ON_MFG_AREA, ON_BROCA, ON_SFG_DMN, ON_AREAS };

/* switched off, for coded optogenetics */
enum {
  ON_OFF_MFG = 1, ON_OFF_PPA = 2, ON_OFF_DMN = 4, ON_OFF_HYPER = 8, ON_OFF_ACC = 16, ON_OFF_MEMORY = 32,
  ON_OFF_INHIBIT = 64
};

/* an idea: the synapses onto one group, and the group's threshold */
typedef struct {
  unsigned char exc[ON_FEAT][ON_K];   /* potentiated excitatory synapses, from each population's neurons */
  unsigned char inh[OL_MOTORS][ON_K]; /* inhibitory, through local interneurons, from the acts */
  float theta;                        /* the group's own threshold */
  unsigned source;                    /* where it formed */
} on_idea_t;

typedef struct {
  of_test_t t;
  int outcome;
  float strength;                     /* how well it was laid down (cortisol, dopamine, oxytocin) */
} on_result_t;

typedef struct {
  ob_one_t *O;
  unsigned off;
  unsigned rng;
  /* the populations: real neurons */
  unsigned feat[ON_FEAT][ON_K];
  /* the DLPFC's groups */
  unsigned n_groups;
  unsigned neuron[ON_HELD][6];
  float theta[ON_HELD];
  float lit[ON_HELD];
  on_idea_t idea[ON_HELD];
  int holds[ON_HELD];
  /* the hippocampus's results */
  on_result_t mem[ON_MEMORY];
  unsigned n_mem;
  /* groups' synapses that once foretold a chime (dopamine marked them), for the DMN */
  on_idea_t good[32];
  unsigned n_good;
  /* how much each pathway's ideas have paid (dopamine strengthens it) */
  float pathway[ON_SOURCES];
  /* counts */
  unsigned arisen[ON_SOURCES], rejected, struck, tests, quiet, inhibitions;
} on_mind_t;

/* which frontal area a frontal neuron is in (by position) */
unsigned on_area(const ob_neuron_t *n);

/* does the group with these synapses fire for test t (imagined: the
   populations driven as t would drive them) */
int on_fires(const on_idea_t *i, const of_test_t *t);

/* an idea in words, from its synapses */
void on_describe(const on_idea_t *i, char *out, unsigned size);

typedef struct {
  unsigned room;                      /* how much its DLPFC could hold */
  int found;                          /* it came to know the rule, and rightly */
  int claimed;                        /* it came to know a rule, rightly or not */
  unsigned tries;                     /* tests it made, to know it */
  unsigned long moments;              /* moments that took */
  on_idea_t known;                    /* what it came to know */
  unsigned arisen[ON_SOURCES], rejected, struck, inhibitions;
  float cortisol, dopamine, oxytocin; /* means, while it worked */
  double chime_rate;                  /* chimes per moment, in the last 300 moments */
} on_meeting_t;

/*
 * A box whose rule is `rule` (any rule at all) is in the crib for `moments`
 * moments. It is told nothing of what rules there could be. `off`: ON_OFF_*.
 */
typedef int (*on_rule_t)(const of_test_t *t);
void on_box_meet(ob_world_t *W, ob_one_t *O, on_rule_t rule, unsigned long moments, unsigned off, unsigned seed,
                 on_meeting_t *out);

#endif
