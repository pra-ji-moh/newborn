/*
 * ob_novel.h -- the conditions for novelty: ideas nobody writes.
 *
 * Nothing hands it what the hidden rule could be: no list of rules, no
 * generator of them. Ideas arise from its own activity, in the places the
 * research names, and are kept only if they survive what it remembers.
 *
 * WHAT AN IDEA IS
 *   What one group of its DLPFC neurons can hold: some of the acts it did,
 *   some of the places it looked, and how many of those acts must come
 *   together (a group fires when enough of its inputs do, as any neuron
 *   does). So an idea can be "this act, there", "these two together, there",
 *   "either of these, anywhere on the wall", "two of these three". Which of
 *   them it ever thinks of depends only on what it has lived. What one group
 *   cannot stand for (an order in time, "this but not that") it cannot think
 *   yet: a limit, said plainly.
 *
 * WHERE IDEAS ARISE (the research, Parts 4 to 6)
 *   MFG     the middle frontal gyrus: fluid reasoning, devising novel
 *           strategies. Where the dorsal attention network (where its eyes
 *           were: the DAN) and the ventral (what was unexpected: the VAN)
 *           meet, with what it just did. When something unexpected happens,
 *           it binds them into a new idea, loosely: some of what it did, and
 *           a guess at how much of it mattered
 *   PPA     the collateral sulcus's scene area: the larger picture. It feeds
 *           the MFG the whole of a scene, not one spot: an idea's place can
 *           widen to every part of the wall it has seen things happen in
 *   DMN     the default mode network (the anteromedial superior frontal
 *           gyrus, with the angular gyrus): when it is not busy testing,
 *           its hippocampus replays moments it has lived, two at a time, and
 *           what they share or what either holds becomes an idea. The common
 *           sense pivot: ideas made from its own life
 *   Broca   Broca's area and the frontal eye fields, wired hyper-directly
 *   + FEF   (deeper, heavily myelinated, straight into the motor strip at
 *           the central sulcus: frontal hypothesis 1). An idea becomes the
 *           act that tests it at full strength and its eyes go where it says
 *           in fewer moments. Without the links, its signal arrives as weak
 *           as the frontal cortex's myelin: acts it meant drop out
 *
 * SELECTION, NOT AUTHORSHIP
 *   Whatever arises is checked by the ACC against every result its
 *   hippocampus recalls; one it contradicts never takes hold. What survives is
 *   held by a group of DLPFC neurons that keeps itself firing (the same
 *   self-excitation and shared inhibition as ob_frontal.c: how many it can
 *   hold grows with myelin). The IPS picks the test that splits what it
 *   holds most evenly. Each outcome: the ACC darkens what it contradicts.
 *   It knows when one idea is held, has predicted everything it remembers,
 *   and the ideas that keep arising all fail against memory.
 *
 * ITS CHEMISTRY (no reward, no punishment)
 *   cortisol   failure: an idea it held said the box would chime and it did
 *              not. It strengthens the memory of that result; it makes the
 *              VAN quicker to flag (more ideas arise, wider ones); high, it
 *              weakens the DLPFC's hold (an inverted U); it fades slowly
 *   dopamine   success: the box chimed as an idea said. It steadies that
 *              idea's group, strengthens the memory, and strengthens the
 *              pathway the idea came from (MFG, PPA or DMN): it learns which
 *              way of thinking pays
 *   oxytocin   success with its mother (she answers the chime): calms
 *              cortisol, strengthens the memory
 *
 * Coded optogenetics: any of MFG, PPA, DMN, the hyper-direct links, the ACC
 * or the hippocampus's memory can be switched off, to see what each is worth.
 *
 * What is neural and what is not, plainly: the DLPFC's holding is its
 * neurons' activity (ob_frontal.c's dynamics). The MFG, PPA and DMN form an
 * idea's content from the features active in a moment or a recalled moment;
 * that content is computed here from those features, and the areas' neurons
 * are lit while they do it, but the binding is not yet in their synapses.
 * The results are kept as a list (the hippocampus's part), as before.
 */

#ifndef OB_NOVEL_H
#define OB_NOVEL_H

#include "ob_box.h"
#include "ob_one.h"

#define ON_HELD 256                   /* DLPFC groups */
#define ON_MEMORY 4096                /* results remembered in one meeting */

enum { ON_MFG, ON_PPA, ON_DMN, ON_SOURCES };

/* frontal areas, as ob_frontal's by position, with the research's added */
enum { ON_OTHER, ON_DLPFC, ON_ACC, ON_OFC, ON_FEF, ON_MFG_AREA, ON_BROCA, ON_SFG_DMN, ON_AREAS };

/* switched off, for coded optogenetics */
enum {
  ON_OFF_MFG = 1, ON_OFF_PPA = 2, ON_OFF_DMN = 4, ON_OFF_HYPER = 8, ON_OFF_ACC = 16, ON_OFF_MEMORY = 32
};

typedef struct {
  unsigned acts;                      /* acts it binds (bits, as ob_one's) */
  unsigned places;                    /* places it binds (bits: quarters of the wall) */
  unsigned need;                      /* how many of the acts must come together */
  unsigned source;                    /* where it arose */
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
  /* the DLPFC's groups */
  unsigned n_groups;
  unsigned neuron[ON_HELD][6];
  float lit[ON_HELD];
  on_idea_t idea[ON_HELD];
  int holds[ON_HELD];
  /* the areas' neurons, lit while they work (for watching) */
  unsigned area_n[ON_AREAS];
  unsigned area_first[ON_AREAS];
  unsigned *area_list;
  /* the hippocampus's results */
  on_result_t mem[ON_MEMORY];
  unsigned n_mem;
  /* how much each pathway's ideas have paid (dopamine strengthens it) */
  float pathway[ON_SOURCES];
  /* counts */
  unsigned arisen[ON_SOURCES], rejected, struck, tests, quiet;
} on_mind_t;

/* which frontal area a frontal neuron is in (by position) */
unsigned on_area(const ob_neuron_t *n);

/* does idea i say test t brings the chime */
int on_says(const on_idea_t *i, const of_test_t *t);

typedef struct {
  unsigned room;                      /* how much its DLPFC could hold */
  int found;                          /* it came to know the rule, and rightly */
  unsigned tries;                     /* tests it made, to know it */
  unsigned long moments;              /* moments that took */
  on_idea_t known;                    /* what it came to know */
  unsigned arisen[ON_SOURCES], rejected, struck;
  float cortisol, dopamine, oxytocin; /* means, while it worked */
  double chime_rate;                  /* chimes per moment, in the last 300 moments */
} on_meeting_t;

/*
 * A box with rule `rule` (any rule the box can have: ob_box.h's, or a form it
 * has never met) is in the crib for `moments` moments. It is told nothing of
 * what rules there could be. `off`: ON_OFF_* to switch parts off.
 */
typedef int (*on_rule_t)(const of_test_t *t);
void on_box_meet(ob_world_t *W, ob_one_t *O, on_rule_t rule, unsigned long moments, unsigned off, unsigned seed,
                 on_meeting_t *out);

#endif
