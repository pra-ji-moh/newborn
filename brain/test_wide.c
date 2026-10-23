/*
 * test_wide.c -- ideas nobody writes, scored wide: many rules it was never
 * developed on, drawn at random, met by a life grown from a given genome seed.
 *
 *   test_wide LIFE GENOME_SEED RULES RULE_SEED
 *
 * LIFE is the three-year life on file (grown and kept there if missing).
 * RULES boxes are drawn from RULE_SEED, each a random form with random acts
 * and quarters:
 *   one        one act, at a quarter
 *   both       two acts together, at a quarter
 *   either     either of two acts, at a quarter
 *   two of 3   two of three acts, at a quarter
 *   anywhere   one act, anywhere on the wall
 *   but not    one act, but not with another, at a quarter
 *   one of 2   exactly one of two acts, at a quarter: what one group of
 *              neurons cannot fire for. Found would be impossible; a claim
 *              would be wrong
 * Printed per form: found, claimed wrongly, not claimed, median tests; and
 * one line per box, for the record. It checks nothing: it measures.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ob.h"
#include "ob_box.h"
#include "ob_novel.h"

static ob_brain_t BRAIN;
static ob_one_t ONE, SAVED;
static ob_world_t WORLD, WORLD_SAVED;

enum { F_ONE, F_BOTH, F_EITHER, F_TWO3, F_ANY, F_BUTNOT, F_ONEOF2, F_FORMS };
static const char *const FORM[F_FORMS] = {"one", "both", "either", "two of 3", "anywhere", "but not", "one of 2"};

/* the rule now in the box */
static unsigned R_FORM, R_A, R_B, R_C, R_Q;


static int rule(const of_test_t *t) {
  unsigned a = t->acts >> R_A & 1u, b = t->acts >> R_B & 1u, c = t->acts >> R_C & 1u;
  if (R_FORM == F_ANY) return a;
  if (t->place != R_Q) return 0;
  switch (R_FORM) {
    case F_ONE: return a;
    case F_BOTH: return a && b;
    case F_EITHER: return a || b;
    case F_TWO3: return a + b + c >= 2u;
    case F_BUTNOT: return a && !b;
    default: return a + b == 1u;
  }
}

static unsigned R;
static unsigned rnd(void) {
  R ^= R << 13;
  R ^= R >> 17;
  R ^= R << 5;
  return R;
}

static int load(const char *path) {
  FILE *f = fopen(path, "rb");
  int ok;
  if (!f) return 0;
  ok = fread(&SAVED, sizeof SAVED, 1, f) == 1 && fread(&WORLD_SAVED, sizeof WORLD_SAVED, 1, f) == 1;
  fclose(f);
  return ok;
}

static int cmp(const void *x, const void *y) { return (int)*(const unsigned *)x - (int)*(const unsigned *)y; }

int main(int argc, char **argv) {
  unsigned genome_seed, n_rules, k, f;
  unsigned found[F_FORMS], wrong[F_FORMS], none[F_FORMS], tries[F_FORMS][256], n_tries[F_FORMS];
  if (argc < 5) {
    fprintf(stderr, "test_wide LIFE GENOME_SEED RULES RULE_SEED\n");
    return 2;
  }
  genome_seed = (unsigned)strtoul(argv[2], 0, 10);
  n_rules = (unsigned)strtoul(argv[3], 0, 10);
  R = (unsigned)strtoul(argv[4], 0, 10) * 2654435761u + 99u;
  if (!R) R = 1u;
  if (load(argv[1])) {
    ob_one_resume(&SAVED);
  } else {
    ob_genome_t g;
    ob_womb_stats_t a;
    ob_smell_stats_t b;
    ob_heart_stats_t c;
    ob_vision_stats_t d;
    ob_reflex_stats_t e;
    FILE *out;
    ob_genome_default(&g);
    g.seed = genome_seed;
    ob_grow(&BRAIN, &g);
    ob_womb_touch(&BRAIN, 20u, 300u, &a);
    ob_womb_smell(&BRAIN, 20u, 50u, &b);
    ob_womb_heartbeat(&BRAIN, 20u, 400u, &c);
    ob_womb_vision(&BRAIN, 20u, 200u, 1, &d);
    ob_womb_reflex(&BRAIN, 20u, 100u, 1, &e);
    ob_one_born(&ONE, &BRAIN, g.seed, OL_PROGRESS);
    ob_one_age(&ONE, 0.0);
    ob_world_make(&WORLD, g.seed);
    ob_grow_up(&WORLD, &ONE, 36.0, 1000ul);
    memcpy(&SAVED, &ONE, sizeof SAVED);
    memcpy(&WORLD_SAVED, &WORLD, sizeof WORLD_SAVED);
    out = fopen(argv[1], "wb");
    if (out) {
      fwrite(&SAVED, sizeof SAVED, 1, out);
      fwrite(&WORLD_SAVED, sizeof WORLD_SAVED, 1, out);
      fclose(out);
    }
  }
  printf("WIDE: genome seed %u, %u rules from rule seed %s\n", genome_seed, n_rules, argv[4]);
  memset(found, 0, sizeof found);
  memset(wrong, 0, sizeof wrong);
  memset(none, 0, sizeof none);
  memset(n_tries, 0, sizeof n_tries);
  for (k = 0u; k < n_rules; k++) {
    on_meeting_t m;
    char said[160];
    R_FORM = rnd() % F_FORMS;
    R_A = rnd() % OL_MOTORS;
    do R_B = rnd() % OL_MOTORS; while (R_B == R_A);
    do R_C = rnd() % OL_MOTORS; while (R_C == R_A || R_C == R_B);
    R_Q = rnd() % 4u;
    memcpy(&ONE, &SAVED, sizeof ONE);
    memcpy(&WORLD, &WORLD_SAVED, sizeof WORLD);
    on_box_meet(&WORLD, &ONE, rule, 3000ul, 0u, 1000u + k, &m);
    f = R_FORM;
    if (m.found) {
      found[f]++;
      if (n_tries[f] < 256u) tries[f][n_tries[f]++] = m.tries;
    } else if (m.claimed) {
      wrong[f]++;
    } else {
      none[f]++;
    }
    said[0] = 0;
    if (m.claimed) on_describe(&m.known, said, sizeof said);
    printf("  %-9s a%u b%u c%u q%u  %-13s tests %3u  %s\n", FORM[f], R_A, R_B, R_C, R_Q,
           m.found ? "found" : m.claimed ? "WRONG CLAIM" : "not claimed", m.tries, said);
    fflush(stdout);
  }
  printf("\n  %-9s %6s %6s %6s %8s\n", "form", "found", "wrong", "none", "median");
  for (f = 0u; f < F_FORMS; f++) {
    unsigned med = 0u;
    if (n_tries[f]) {
      qsort(tries[f], n_tries[f], sizeof(unsigned), cmp);
      med = tries[f][n_tries[f] / 2u];
    }
    printf("  %-9s %6u %6u %6u %8u\n", FORM[f], found[f], wrong[f], none[f], med);
  }
  return 0;
}
