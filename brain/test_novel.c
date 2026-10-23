/*
 * test_novel.c -- ideas nobody writes: one life to three years, then boxes
 * whose rules it is never told the form of.
 *
 * One being, as in test_frontal.c: grown from its genes, the womb, born, and
 * living in the crib as its brain ages, a thousand moments a month, its
 * chemistry dopamine, oxytocin and cortisol (no reward, no punishment). At
 * three years, copies of it, with the same life behind them, meet boxes. No
 * list of rules is given: ideas arise in its middle frontal gyrus, scene area
 * and default mode network from what it lives, and are kept if they survive
 * what its hippocampus remembers (ob_novel.c).
 *
 * The boxes:
 *   familiar forms   the 144 of test_frontal.c: one act, both of two, either
 *                    of two, with its eyes on one quarter of the wall
 *   new forms        never in that family: two of three acts; an act
 *                    anywhere on the wall
 *   beyond it        "this act, but not that one": what one group of neurons
 *                    cannot stand for. It should not claim to know it
 *
 * The same boxes with parts switched off (coded optogenetics): the middle
 * frontal gyrus, the default mode network, the scene area, the hyper-direct
 * links, the ACC, the hippocampus's memory of results. And the old circuit
 * (ob_frontal.c), handed the list of 144, as the yardstick.
 *
 * Checked (each could fail):
 *   familiar      handed nothing, it finds every familiar rule
 *   new forms     it finds rules of forms it was never shown
 *   beyond        it does not claim a rule it cannot think
 *   MFG           without the middle frontal gyrus it finds fewer, or needs
 *                 more tests
 *   DMN           without the default mode network, likewise
 *   hyper-direct  without the links, each test takes more moments
 *   ACC           without it, it finds none
 *   cortisol      rises where it fails; dopamine where it succeeds
 *
 *   test_novel              the life, then the boxes (many minutes)
 *   test_novel SNAPSHOT     keep the life at three years in SNAPSHOT, or
 *                           start from it if it is there
 *   test_novel SNAPSHOT q   the boxes only, no switching off and no checks
 */

#include <stdio.h>
#include <string.h>

#include "ob.h"
#include "ob_box.h"
#include "ob_novel.h"

static unsigned n_checks = 0u, n_fail = 0u;

static void check(const char *name, int ok) {
  n_checks++;
  if (!ok) n_fail++;
  printf("  %s  %s\n", ok ? "pass" : "FAIL", name);
}

static ob_brain_t BRAIN;
static ob_one_t ONE, SAVED;
static ob_world_t WORLD, WORLD_SAVED;
static ob_womb_stats_t TOUCH;
static ob_smell_stats_t SMELL;
static ob_heart_stats_t HEART;
static ob_vision_stats_t VISION;
static ob_reflex_stats_t REFLEX;

#define PER_MONTH 1000ul
#define WITH_BOX 3000ul

/* the boxes' rules: acts are bits (eyes left, right, up, down, voice, jerk) */
static unsigned bits_of(unsigned acts) {
  unsigned n = 0u;
  for (; acts; acts &= acts - 1u) n++;
  return n;
}
#define A(x) (1u << (x))
static int f_one(const of_test_t *t) { return t->place == 1u && (t->acts & A(4)); }
static int f_one_b(const of_test_t *t) { return t->place == 2u && (t->acts & A(5)); }
static int f_both(const of_test_t *t) { return t->place == 3u && (t->acts & (A(4) | A(5))) == (A(4) | A(5)); }
static int f_both_b(const of_test_t *t) { return t->place == 0u && (t->acts & (A(1) | A(4))) == (A(1) | A(4)); }
static int f_either(const of_test_t *t) { return t->place == 2u && (t->acts & (A(0) | A(5))) != 0u; }
static int f_either_b(const of_test_t *t) { return t->place == 1u && (t->acts & (A(3) | A(4))) != 0u; }
static int n_two_of_three(const of_test_t *t) { return t->place == 0u && bits_of(t->acts & (A(1) | A(4) | A(5))) >= 2u; }
static int n_anywhere(const of_test_t *t) { return (t->acts & A(4)) != 0u; }
static int b_but_not(const of_test_t *t) { return t->place == 3u && (t->acts & A(4)) && !(t->acts & A(5)); }

typedef struct {
  const char *name;
  on_rule_t rule;
  int old_index;                      /* in the 144 of ob_box (-1: not one of them) */
} box_t;

/* ob_box's rule h: form h / 4 (6 single, 15 pairs both, 15 pairs either), quarter h % 4 */
static const box_t FAMILIAR[] = {
    {"act 4, quarter 1", f_one, 4 * 4 + 1},
    {"act 5, quarter 2", f_one_b, 5 * 4 + 2},
    {"acts 4 and 5, quarter 3", f_both, (6 + 14) * 4 + 3},
    {"acts 1 and 4, quarter 0", f_both_b, (6 + 7) * 4 + 0},
    {"act 0 or 5, quarter 2", f_either, (21 + 4) * 4 + 2},
    {"act 3 or 4, quarter 1", f_either_b, (21 + 12) * 4 + 1},
};
#define N_FAM (sizeof FAMILIAR / sizeof FAMILIAR[0])
static const box_t NEW[] = {
    {"two of acts 1, 4, 5, quarter 0", n_two_of_three, -1},
    {"act 4, anywhere", n_anywhere, -1},
};
#define N_NEW (sizeof NEW / sizeof NEW[0])
static const box_t BEYOND = {"act 4 but not 5, quarter 3", b_but_not, -1};

typedef struct {
  unsigned found, boxes;
  double tries, moments_per_test, cortisol, dopamine, oxytocin;
  unsigned arisen[ON_SOURCES], rejected;
} tally_t;

static void restore(void) {
  memcpy(&ONE, &SAVED, sizeof ONE);
  memcpy(&WORLD, &WORLD_SAVED, sizeof WORLD);
}

static void meet_all(const box_t *boxes, unsigned n, unsigned off, tally_t *t, int print) {
  unsigned k, s;
  memset(t, 0, sizeof(*t));
  for (k = 0u; k < n; k++) {
    on_meeting_t m;
    restore();
    on_box_meet(&WORLD, &ONE, boxes[k].rule, WITH_BOX, off, 7u + k, &m);
    t->boxes++;
    t->found += (unsigned)m.found;
    if (m.found) {
      t->tries += m.tries;
      t->moments_per_test += m.tries ? (double)m.moments / m.tries : 0.0;
    }
    t->cortisol += m.cortisol / n;
    t->dopamine += m.dopamine / n;
    t->oxytocin += m.oxytocin / n;
    for (s = 0u; s < ON_SOURCES; s++) t->arisen[s] += m.arisen[s];
    t->rejected += m.rejected;
    if (print)
      printf("    %-34s %s  tests %3u  ideas arisen MFG %4u PPA %3u DMN %4u, failed memory %4u  "
             "cortisol %.2f dopamine %.2f oxytocin %.2f\n",
             boxes[k].name, m.found ? "found    " : "not found", m.tries, m.arisen[ON_MFG], m.arisen[ON_PPA],
             m.arisen[ON_DMN], m.rejected, m.cortisol, m.dopamine, m.oxytocin);
    fflush(stdout);
  }
  if (t->found) {
    t->tries /= t->found;
    t->moments_per_test /= t->found;
  }
}

static void row(const char *name, const tally_t *t) {
  printf("  %-30s %u/%u found, %6.1f tests, %5.1f moments a test\n", name, t->found, t->boxes, t->tries,
         t->moments_per_test);
  fflush(stdout);
}

static int load(const char *path) {
  FILE *f = path ? fopen(path, "rb") : 0;
  int ok;
  if (!f) return 0;
  ok = fread(&SAVED, sizeof SAVED, 1, f) == 1 && fread(&WORLD_SAVED, sizeof WORLD_SAVED, 1, f) == 1;
  fclose(f);
  return ok;
}

static void save(const char *path) {
  FILE *f = fopen(path, "wb");
  if (!f) return;
  fwrite(&SAVED, sizeof SAVED, 1, f);
  fwrite(&WORLD_SAVED, sizeof WORLD_SAVED, 1, f);
  fclose(f);
}

int main(int argc, char **argv) {
  const char *snap = argc > 1 ? argv[1] : 0;
  tally_t fam, nw, beyond, no_mfg, no_dmn, no_ppa, no_hyper, no_acc, no_mem;
  unsigned k, old_found = 0u;
  double old_tries = 0.0;
  printf("IDEAS NOBODY WRITES: ONE LIFE TO THREE YEARS, THEN BOXES OF UNTOLD FORM\n\n");
  if (load(snap)) {
    printf("  its life at three years, from %s\n", snap);
  } else {
    ob_genome_t g;
    ob_genome_default(&g);
    ob_grow(&BRAIN, &g);
    ob_womb_touch(&BRAIN, 20u, 300u, &TOUCH);
    ob_womb_smell(&BRAIN, 20u, 50u, &SMELL);
    ob_womb_heartbeat(&BRAIN, 20u, 400u, &HEART);
    ob_womb_vision(&BRAIN, 20u, 200u, 1, &VISION);
    ob_womb_reflex(&BRAIN, 20u, 100u, 1, &REFLEX);
    ob_one_born(&ONE, &BRAIN, g.seed, OL_PROGRESS);
    ob_one_age(&ONE, 0.0);
    ob_world_make(&WORLD, g.seed);
    ob_grow_up(&WORLD, &ONE, 36.0, PER_MONTH);
    memcpy(&SAVED, &ONE, sizeof SAVED);
    memcpy(&WORLD_SAVED, &WORLD, sizeof WORLD_SAVED);
    if (snap) save(snap);
    printf("  grown from its genes, the womb, and 36 months in the crib\n");
  }
  {
    unsigned area[ON_AREAS], i;
    memset(area, 0, sizeof area);
    for (i = 0u; i < SAVED.brain.n_neurons; i++) area[on_area(&SAVED.brain.neuron[i])]++;
    printf("  its frontal areas (neurons): DLPFC %u, MFG %u, Broca %u, SFG (DMN) %u, FEF %u, ACC %u, OFC %u\n",
           area[ON_DLPFC], area[ON_MFG_AREA], area[ON_BROCA], area[ON_SFG_DMN], area[ON_FEF], area[ON_ACC],
           area[ON_OFC]);
    printf("  its chemistry at three years: dopamine %.2f, oxytocin %.2f, cortisol %.2f; frontal myelin %.2f\n\n",
           SAVED.dopamine, SAVED.oxytocin, SAVED.cortisol, SAVED.myelin[OB_FRONTAL]);
  }

  printf("  familiar forms, told nothing:\n");
  meet_all(FAMILIAR, N_FAM, 0u, &fam, 1);
  printf("  new forms:\n");
  meet_all(NEW, N_NEW, 0u, &nw, 1);
  printf("  beyond what one idea can stand for:\n");
  meet_all(&BEYOND, 1u, 0u, &beyond, 1);
  if (argc > 2) return 0;   /* "quick": the boxes only */

  /* the yardstick: the old circuit, handed the 144 */
  for (k = 0u; k < N_FAM; k++) {
    ob_box_meeting_t m;
    restore();
    ob_box_meet(&WORLD, &ONE, (unsigned)FAMILIAR[k].old_index, WITH_BOX, 0u, &m);
    old_found += (unsigned)m.found;
    old_tries += m.found ? m.tries : 0.0;
  }
  if (old_found) old_tries /= old_found;

  printf("\n  switched off (coded optogenetics), on the familiar forms:\n");
  meet_all(FAMILIAR, N_FAM, ON_OFF_MFG, &no_mfg, 0);
  meet_all(FAMILIAR, N_FAM, ON_OFF_DMN, &no_dmn, 0);
  meet_all(FAMILIAR, N_FAM, ON_OFF_PPA, &no_ppa, 0);
  meet_all(FAMILIAR, N_FAM, ON_OFF_HYPER, &no_hyper, 0);
  meet_all(FAMILIAR, N_FAM, ON_OFF_ACC, &no_acc, 0);
  meet_all(FAMILIAR, N_FAM, ON_OFF_MEMORY, &no_mem, 0);
  row("whole", &fam);
  row("no middle frontal gyrus", &no_mfg);
  row("no default mode network", &no_dmn);
  row("no scene area (PPA)", &no_ppa);
  row("no hyper-direct links", &no_hyper);
  row("no ACC", &no_acc);
  row("no memory of results", &no_mem);
  printf("  %-30s %u/%u found, %6.1f tests\n", "old circuit, handed the 144", old_found, (unsigned)N_FAM, old_tries);
  printf("\n");

  check("familiar: handed nothing, it finds every familiar rule", fam.found == N_FAM);
  check("new forms: it finds rules of forms it was never shown", nw.found == N_NEW);
  check("beyond: it does not claim a rule it cannot think", beyond.found == 0u);
  check("MFG: without it, fewer found or more tests",
        no_mfg.found < fam.found || (no_mfg.found == fam.found && no_mfg.tries > fam.tries));
  check("DMN: without it, fewer found or more tests",
        no_dmn.found < fam.found || (no_dmn.found == fam.found && no_dmn.tries > fam.tries));
  check("hyper-direct: without the links, more moments a test",
        no_hyper.found == 0u || no_hyper.moments_per_test > fam.moments_per_test);
  check("ACC: without it, none found", no_acc.found == 0u);
  check("chemistry: cortisol and dopamine both rise while it works", fam.cortisol > 0.01 && fam.dopamine > 0.01);
  printf("\n%u checks, %u failed\n", n_checks, n_fail);
  return n_fail != 0u;
}
