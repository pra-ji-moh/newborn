/*
 * ob_novel.c -- the conditions for novelty: ideas nobody writes. See ob_novel.h.
 */

#include "ob_novel.h"

#include <stdlib.h>
#include <string.h>

/* the DLPFC's holding, as ob_frontal.c's: self-excitation through myelinated
   axons, one shared pool of inhibition */
#define SELF 7.2f
#define SHARED 0.088f
#define LIT 0.5f
#define STRIKE 10.0f
#define CUE 8.0f

#define CHECK_TESTS 6u                /* tests an idea must pass, once held alone, before it is trusted */
#define QUIET 12u                     /* ideas in a row that fail against memory, before it trusts the one it holds */

unsigned on_area(const ob_neuron_t *n) {
  /* front to back (sx), floor to roof (sy), as ob_frontal.c's areas, with the
     research's: the anteromedial superior frontal gyrus (high, toward the
     front: the default mode network's), and in the band between floor and
     roof Broca's area and the middle frontal gyrus. (The frontal sheet's
     neurons sit in columns and bands; these bounds follow them) */
  if (n->region != OB_FRONTAL) return ON_OTHER;
  if (n->sx > 0.8f) return ON_FEF;
  if (n->sy < 0.3f) return n->sx < 0.35f ? ON_OFC : ON_ACC;
  /* the middle band: Broca's area toward the back (the inferior frontal
     gyrus, by the motor strip), the middle frontal gyrus in front of it */
  if (n->sy >= 0.3f && n->sy < 0.6f) return n->sx > 0.5f ? ON_BROCA : ON_MFG_AREA;
  if (n->sy > 0.85f && n->sx < 0.45f) return ON_SFG_DMN;
  return ON_DLPFC;
}

static unsigned popcount(unsigned x) {
  unsigned n = 0u;
  for (; x; x &= x - 1u) n++;
  return n;
}

int on_says(const on_idea_t *i, const of_test_t *t) {
  return (i->places >> t->place & 1u) && popcount(t->acts & i->acts) >= i->need;
}

static unsigned rnd(on_mind_t *M) {
  M->rng ^= M->rng << 13;
  M->rng ^= M->rng >> 17;
  M->rng ^= M->rng << 5;
  return M->rng;
}

static float unit(on_mind_t *M) { return (float)(rnd(M) >> 8) / 16777216.0f; }

static float clip(float x) { return x < 0.0f ? 0.0f : x > 1.0f ? 1.0f : x; }

/* ----- the DLPFC: holding ----- */

static float self_gain(const on_mind_t *M) {
  /* myelin carries the self-excitation; cortisol, high, weakens the hold
     (an inverted U: a little sharpens, much impairs) */
  float c = M->O->cortisol - 0.4f;
  return SELF * M->O->myelin[OB_FRONTAL] * (1.0f - (c > 0.0f ? 0.6f * c : 0.0f));
}

static void settle(const on_mind_t *M, float *lit, const float *push, int cue) {
  unsigned t, g, n = M->n_groups;
  float self = self_gain(M);
  for (t = 0u; t < 40u; t++) {
    float total = 0.0f;
    for (g = 0u; g < n; g++) total += lit[g];
    for (g = 0u; g < n; g++) {
      float in = self * lit[g] - SHARED * (total - lit[g]) - (push ? push[g] : 0.0f);
      if ((int)g == cue && t < 5u) in += CUE;
      lit[g] += 0.5f * (clip(in) - lit[g]);
      if (lit[g] < 0.01f) lit[g] = 0.0f;
    }
  }
}

static void show(on_mind_t *M) {
  unsigned g, k;
  for (g = 0u; g < M->n_groups; g++)
    for (k = 0u; k < 6u; k++) M->O->act[M->neuron[g][k]] = M->lit[g];
}

static void light_area(on_mind_t *M, unsigned area, float level) {
  unsigned k;
  for (k = 0u; k < M->area_n[area]; k++) M->O->act[M->area_list[M->area_first[area] + k]] = level;
}

static unsigned held(const on_mind_t *M) {
  unsigned g, n = 0u;
  for (g = 0u; g < M->n_groups; g++) n += M->holds[g] && M->lit[g] >= LIT;
  return n;
}

static void forget_dark(on_mind_t *M) {
  unsigned g;
  for (g = 0u; g < M->n_groups; g++)
    if (M->lit[g] < LIT) {
      M->lit[g] = 0.0f;
      M->holds[g] = 0;
    }
}

static unsigned room(const on_mind_t *M) {
  float lit[ON_HELD], trial[ON_HELD];
  unsigned g, k, n = M->n_groups, r = 0u;
  memset(lit, 0, sizeof lit);
  for (g = 0u; g < n; g++) {
    unsigned now = 0u;
    memcpy(trial, lit, sizeof(float) * n);
    settle(M, trial, 0, (int)g);
    for (k = 0u; k < n; k++) now += trial[k] >= LIT;
    if (now <= r) break;
    memcpy(lit, trial, sizeof(float) * n);
    r = now;
  }
  return r;
}

/* the same idea, as far as anything it could do would show */
static int same(const on_idea_t *a, const on_idea_t *b) {
  unsigned acts, place;
  for (place = 0u; place < 4u; place++)
    for (acts = 1u; acts < 64u; acts++) {
      of_test_t t = {acts, place};
      if (on_says(a, &t) != on_says(b, &t)) return 0;
    }
  return 1;
}

/* hold it, if it will stay lit without putting out another */
static int hold(on_mind_t *M, const on_idea_t *idea) {
  float trial[ON_HELD];
  unsigned g, n = M->n_groups, free_g = n;
  for (g = 0u; g < n && free_g == n; g++)
    if (!M->holds[g]) free_g = g;
  if (free_g == n) return 0;
  memcpy(trial, M->lit, sizeof(float) * n);
  trial[free_g] = 0.0f;
  settle(M, trial, 0, (int)free_g);
  if (trial[free_g] < LIT) return 0;
  for (g = 0u; g < n; g++)
    if (g != free_g && M->lit[g] >= LIT && trial[g] < LIT) return 0;
  memcpy(M->lit, trial, sizeof(float) * n);
  M->holds[free_g] = 1;
  M->idea[free_g] = *idea;
  show(M);
  return 1;
}

/* ----- the hippocampus and the ACC ----- */

static void remember(on_mind_t *M, const of_test_t *t, int outcome) {
  on_result_t *r;
  float boost = 2.0f * M->O->cortisol + M->O->dopamine + M->O->oxytocin;
  if (M->n_mem >= ON_MEMORY) return;
  r = &M->mem[M->n_mem++];
  r->t = *t;
  r->outcome = outcome;
  /* how well it is laid down: the chemistry of the moment strengthens it */
  r->strength = 0.7f + 0.3f * clip(boost);
}

/* the ACC, against what the hippocampus recalls: 1 if the idea survives */
static int survives_memory(on_mind_t *M, const on_idea_t *idea) {
  unsigned k;
  if (M->off & (ON_OFF_ACC | ON_OFF_MEMORY)) return 1;
  for (k = 0u; k < M->n_mem; k++) {
    const on_result_t *r = &M->mem[k];
    if (on_says(idea, &r->t) != r->outcome && unit(M) < r->strength) return 0;
  }
  return 1;
}

/* an idea arises: the ACC checks it; it is held if it survives and is new */
static void arise(on_mind_t *M, on_idea_t idea) {
  unsigned g;
  if (idea.acts == 0u || idea.places == 0u || idea.need == 0u) return;
  if (idea.need > popcount(idea.acts)) idea.need = popcount(idea.acts);
  M->arisen[idea.source]++;
  if (!survives_memory(M, &idea)) {
    M->rejected++;
    M->quiet++;
    return;
  }
  for (g = 0u; g < M->n_groups; g++)
    if (M->holds[g] && same(&M->idea[g], &idea)) {
      M->quiet++;
      return;
    }
  if (hold(M, &idea)) M->quiet = 0u;
}

/* ----- where ideas arise ----- */

/* some of what it did: each act kept or let go */
static unsigned some_of(on_mind_t *M, unsigned acts) {
  unsigned a, out = 0u;
  for (a = 0u; a < OL_MOTORS; a++)
    if ((acts >> a & 1u) && unit(M) < 0.6f) out |= 1u << a;
  if (!out && acts) {
    /* at least one stays */
    unsigned n = popcount(acts), pick = rnd(M) % n;
    for (a = 0u; a < OL_MOTORS; a++)
      if (acts >> a & 1u) {
        if (pick == 0u) {
          out = 1u << a;
          break;
        }
        pick--;
      }
  }
  return out;
}

static unsigned guess_need(on_mind_t *M, unsigned acts) {
  unsigned n = popcount(acts);
  return n ? 1u + rnd(M) % n : 0u;
}

/* the PPA: the larger picture. Every place it has heard the box chime */
static unsigned scene(const on_mind_t *M, unsigned place) {
  unsigned k, places = 1u << place;
  for (k = 0u; k < M->n_mem; k++)
    if (M->mem[k].outcome) places |= 1u << M->mem[k].t.place;
  return places;
}

static unsigned maybe_wider(on_mind_t *M, unsigned place, unsigned *source) {
  if (M->off & ON_OFF_PPA) return 1u << place;
  if (unit(M) < 0.3f * M->pathway[ON_PPA]) {
    *source = ON_PPA;
    return scene(M, place);
  }
  return 1u << place;
}

/* the MFG: something unexpected. What it did, where its eyes were (the DAN),
   bound loosely into an idea; cortisol makes the VAN quicker, so more arise */
static void mfg(on_mind_t *M, const of_test_t *t, int outcome) {
  unsigned k, n;
  if (M->off & ON_OFF_MFG) return;
  n = 1u + (unsigned)(3.0f * M->O->cortisol * M->pathway[ON_MFG]);
  light_area(M, ON_MFG_AREA, 1.0f);
  for (k = 0u; k < n; k++) {
    on_idea_t idea;
    idea.source = ON_MFG;
    if (outcome) {
      /* it chimed: some of what it did, there */
      idea.acts = some_of(M, t->acts);
    } else if (held(M) == 0u) {
      /* something new in front of it, and nothing happened: what it just
         did, varied (an act added or let go, its eyes elsewhere). Memory
         turns away whatever it has already tried */
      idea.acts = t->acts ^ (1u << (rnd(M) % OL_MOTORS));
      idea.need = guess_need(M, idea.acts);
      idea.places = 1u << (unit(M) < 0.5f ? t->place : rnd(M) % 4u);
      arise(M, idea);
      continue;
    } else {
      /* it did not, though something said it would: what was held, varied by
         what it just did (an act added, or one let go) */
      unsigned g, pick = rnd(M) % (M->n_groups ? M->n_groups : 1u), tries = 0u;
      idea.acts = t->acts;
      for (g = pick; tries < M->n_groups; g = (g + 1u) % M->n_groups, tries++)
        if (M->holds[g]) {
          idea.acts = M->idea[g].acts ^ (1u << (rnd(M) % OL_MOTORS));
          break;
        }
    }
    idea.need = guess_need(M, idea.acts);
    idea.places = maybe_wider(M, t->place, &idea.source);
    arise(M, idea);
  }
}

/* the DMN: idle, its hippocampus replays two moments it has lived (the ones
   the box chimed in most of all), and what they share, or what either holds,
   becomes an idea */
static void dmn(on_mind_t *M) {
  unsigned a = 0u, b = 0u, k, tries;
  on_idea_t idea;
  if ((M->off & ON_OFF_DMN) || M->n_mem == 0u) return;
  if (unit(M) > M->pathway[ON_DMN]) return;
  light_area(M, ON_SFG_DMN, 1.0f);
  if (M->n_good >= 2u && unit(M) < 0.4f) {
    /* two thoughts that once foretold a chime, merged: what either binds,
       needing as few as the readier of them */
    const on_idea_t *x = &M->good[rnd(M) % M->n_good], *y = &M->good[rnd(M) % M->n_good];
    idea.source = ON_DMN;
    idea.acts = x->acts | y->acts;
    idea.need = x->need < y->need ? x->need : y->need;
    idea.places = (x->places & y->places) ? x->places & y->places : x->places | y->places;
    arise(M, idea);
    return;
  }
  for (k = 0u; k < 2u; k++) {
    unsigned pick = 0u;
    for (tries = 0u; tries < 20u; tries++) {
      pick = rnd(M) % M->n_mem;
      if (M->mem[pick].outcome || unit(M) < 0.1f) break;
    }
    if (k == 0u) a = pick;
    else b = pick;
  }
  idea.source = ON_DMN;
  if (M->mem[a].outcome && M->mem[b].outcome && unit(M) < 0.5f) {
    /* what both moments share */
    idea.acts = M->mem[a].t.acts & M->mem[b].t.acts;
    idea.need = popcount(idea.acts);
  } else {
    /* what either holds; how many of it mattered, a guess no bigger than the
       smaller moment */
    unsigned sa = popcount(M->mem[a].t.acts), sb = popcount(M->mem[b].t.acts);
    idea.acts = M->mem[a].t.acts | M->mem[b].t.acts;
    idea.need = 1u + rnd(M) % (sa < sb ? sa : sb ? sb : 1u);
  }
  idea.places = (1u << M->mem[a].t.place) | (1u << M->mem[b].t.place);
  if (!(M->mem[a].outcome && M->mem[b].outcome)) {
    /* one of them was silence: the idea takes the chiming one's place */
    idea.places = 1u << (M->mem[a].outcome ? M->mem[a].t.place : M->mem[b].t.place);
  }
  arise(M, idea);
}

/* ----- the IPS: which test ----- */

static of_test_t choose(on_mind_t *M) {
  unsigned g, acts, place, h = held(M);
  int best_split = -1;
  of_test_t best = {1u, 0u};
  unsigned ties = 0u;
  if (h == 1u) {
    /* one idea: try what it says will chime, and what it says will not, in turn
       (a child checks both ways) */
    /* one idea: check it at its edges, where it would be wrong if it were
       wrong. Near what it binds, what it says will not chime (another act
       where it looks; its own acts elsewhere) and the least it says will
       (just as many of its acts as it needs). And now and then anything it
       says will chime. Each such test new to it, if it can find one */
    const on_idea_t *only = 0;
    of_test_t cand[64];
    unsigned n = 0u, a, p, fresh, k;
    for (g = 0u; g < M->n_groups; g++)
      if (M->holds[g] && M->lit[g] >= LIT) only = &M->idea[g];
    for (p = 0u; p < 4u; p++) {
      for (a = 0u; a < OL_MOTORS; a++) {
        of_test_t t = {1u << a, p};
        if (!on_says(only, &t) && ((only->places >> p & 1u) || (only->acts >> a & 1u)) && n < 64u) cand[n++] = t;
      }
      {
        of_test_t t = {only->acts, p};
        if (!on_says(only, &t) && n < 64u) cand[n++] = t;
      }
      /* and the least it says would chime: just `need` of its acts, there */
      if (only->places >> p & 1u)
        for (a = 1u; a < 64u; a++) {
          of_test_t t = {a, p};
          if ((a & ~only->acts) == 0u && popcount(a) == only->need && n < 64u) cand[n++] = t;
        }
      /* and the nearest of all: the least it says would chime, with one of
         its acts swapped for another */
      if (only->places >> p & 1u)
        for (a = 1u; a < 64u; a++) {
          of_test_t t = {a, p};
          if (popcount(a) == only->need && popcount(a & ~only->acts) == 1u && n < 64u) cand[n++] = t;
        }
    }
    if (n == 0u || (M->tests % 4u) == 0u) {
      for (ties = 0u; ties < 400u; ties++) {
        of_test_t t = {1u + rnd(M) % 63u, rnd(M) % 4u};
        if (on_says(only, &t)) return t;
      }
      return best;
    }
    /* prefer one it has not tried */
    for (fresh = 0u; fresh < 2u; fresh++) {
      unsigned start = rnd(M) % n;
      for (k = 0u; k < n; k++) {
        of_test_t t = cand[(start + k) % n];
        unsigned m, tried = 0u;
        for (m = 0u; m < M->n_mem && !tried; m++) tried = M->mem[m].t.acts == t.acts && M->mem[m].t.place == t.place;
        if (!tried || fresh) return t;
      }
    }
    return cand[0];
  }
  for (place = 0u; place < 4u; place++)
    for (acts = 1u; acts < 64u; acts++) {
      of_test_t t = {acts, place};
      unsigned yes = 0u, split;
      for (g = 0u; g < M->n_groups; g++)
        if (M->holds[g] && M->lit[g] >= LIT && on_says(&M->idea[g], &t)) yes++;
      split = yes < h - yes ? yes : h - yes;
      if ((int)split > best_split) {
        best_split = (int)split;
        best = t;
        ties = 1u;
      } else if ((int)split == best_split && rnd(M) % (++ties) == 0u) {
        best = t;
      }
    }
  return best;
}

/* ----- its body: Broca's area and the frontal eye fields ----- */

static unsigned quarter_of(const ob_crib_t *c) { return (c->gx >= 0.5f) + 2u * (c->gy >= 0.5f); }

/* how strongly an intention reaches the motor strip: whole, through the
   hyper-direct links; else as the frontal cortex's myelin carries it */
static float reach(const on_mind_t *M) { return (M->off & ON_OFF_HYPER) ? M->O->myelin[OB_FRONTAL] : 1.0f; }

static unsigned through(on_mind_t *M, unsigned intent) {
  unsigned a, out = 0u;
  float r = reach(M);
  for (a = 0u; a < OL_MOTORS; a++)
    if ((intent >> a & 1u) && unit(M) < r) out |= 1u << a;
  return out;
}

static unsigned did(const ob_one_t *O) {
  unsigned m, bits = 0u;
  for (m = 0u; m < OL_MOTORS; m++) bits |= O->fired[m] ? 1u << m : 0u;
  return bits;
}

static void look_to(on_mind_t *M, ob_world_t *W, unsigned q) {
  unsigned k;
  float tx = q & 1u ? 0.75f : 0.25f, ty = q & 2u ? 0.75f : 0.25f;
  light_area(M, ON_FEF, 1.0f);
  for (k = 0u; k < 48u; k++) {
    unsigned want = 0u;
    if (W->crib.gx > tx + 0.06f) want |= 1u << OL_LOOK_LEFT;
    if (W->crib.gx < tx - 0.06f) want |= 1u << OL_LOOK_RIGHT;
    if (W->crib.gy > ty + 0.06f) want |= 1u << OL_LOOK_UP;
    if (W->crib.gy < ty - 0.06f) want |= 1u << OL_LOOK_DOWN;
    if (!want) return;
    ob_world_moment(W, M->O, 1u, through(M, want));
  }
}

/* a test, through its body. What it actually did (its efference copy, which
   may be less than it meant), where, and what it heard */
static int try_it(on_mind_t *M, ob_world_t *W, const of_test_t *meant, of_test_t *done) {
  look_to(M, W, meant->place);
  light_area(M, ON_BROCA, 1.0f);
  done->place = quarter_of(&W->crib);
  ob_world_moment(W, M->O, 1u, through(M, meant->acts));
  done->acts = did(M->O);
  return ob_world_moment(W, M->O, 1u, 0u) > 0.5f;
}

/* ----- the meeting ----- */

static void begin(on_mind_t *M, ob_one_t *O, unsigned off, unsigned seed) {
  static unsigned LIST[OB_MAX_NEURONS];
  const ob_brain_t *b = &O->brain;
  unsigned i, a, in_group = 0u, n = 0u;
  memset(M, 0, sizeof(*M));
  M->O = O;
  M->off = off;
  M->rng = seed * 2654435761u + 12345u;
  if (!M->rng) M->rng = 1u;
  for (a = 0u; a < ON_SOURCES; a++) M->pathway[a] = 0.5f;
  for (i = 0u; i < b->n_neurons && M->n_groups < ON_HELD; i++) {
    if (b->neuron[i].inhibitory || on_area(&b->neuron[i]) != ON_DLPFC) continue;
    M->neuron[M->n_groups][in_group] = i;
    O->external[i] = 1u;
    if (++in_group == 6u) {
      in_group = 0u;
      M->n_groups++;
    }
  }
  for (a = 0u; a < ON_AREAS; a++) {
    M->area_first[a] = n;
    for (i = 0u; i < b->n_neurons; i++)
      if (on_area(&b->neuron[i]) == a && a != ON_OTHER) LIST[n++] = i;
    M->area_n[a] = n - M->area_first[a];
  }
  M->area_list = LIST;
}

static void end(on_mind_t *M) {
  const ob_brain_t *b = &M->O->brain;
  unsigned i;
  for (i = 0u; i < b->n_neurons; i++)
    if (on_area(&b->neuron[i]) == ON_DLPFC) M->O->external[i] = 0u;
}

/* the outcome of a test: the ACC darkens what it contradicts; the chemistry */
static void outcome(on_mind_t *M, const of_test_t *t, int heard) {
  float push[ON_HELD];
  unsigned g, expected_chime = 0u, foretold = 0u, wrong = 0u;
  memset(push, 0, sizeof push);
  for (g = 0u; g < M->n_groups; g++) {
    if (!M->holds[g] || M->lit[g] < LIT) continue;
    if (on_says(&M->idea[g], t)) expected_chime++;
    if (on_says(&M->idea[g], t) == heard) {
      if (heard) {
        foretold++;
        push[g] = -1.0f;               /* dopamine steadies the group that foretold it */
        if (M->n_good < 64u) M->good[M->n_good++] = M->idea[g];
        else M->good[rnd(M) % 64u] = M->idea[g];
        M->pathway[M->idea[g].source] += 0.05f * (1.0f - M->pathway[M->idea[g].source]);
      }
    } else {
      wrong++;
      if (!(M->off & ON_OFF_ACC)) push[g] = STRIKE;
    }
  }
  /* cortisol: it expected the chime and it did not come */
  if (!heard && expected_chime) M->O->failure_now = 1.0f;
  remember(M, t, heard);
  settle(M, M->lit, push, -1);
  for (g = 0u; g < M->n_groups; g++)
    if (M->holds[g] && push[g] >= STRIKE && M->lit[g] < LIT) M->struck++;
  forget_dark(M);
  show(M);
  /* the unexpected (the VAN): a chime nothing foretold, or a silence where one
     was foretold. The MFG binds it */
  if ((heard && !foretold) || (!heard && expected_chime) || wrong) mfg(M, t, heard);
  (void)foretold;
}

/* the one idea it holds, if it has earned trust */
static int knows(const on_mind_t *M, unsigned since_alone, on_idea_t *which) {
  unsigned g;
  if (held(M) != 1u || since_alone < CHECK_TESTS || M->quiet < QUIET) return 0;
  for (g = 0u; g < M->n_groups; g++)
    if (M->holds[g] && M->lit[g] >= LIT) *which = M->idea[g];
  return 1;
}

static int rule_equals(on_rule_t rule, const on_idea_t *idea) {
  unsigned acts, place;
  for (place = 0u; place < 4u; place++)
    for (acts = 1u; acts < 64u; acts++) {
      of_test_t t = {acts, place};
      if (rule(&t) != on_says(idea, &t)) return 0;
    }
  return 1;
}

void on_box_meet(ob_world_t *W, ob_one_t *O, on_rule_t rule, unsigned long moments, unsigned off, unsigned seed,
                 on_meeting_t *out) {
  static on_mind_t M;
  unsigned long start = W->moments, mark_t = 0u, mark_c = 0u, n_chem = 0u;
  unsigned since_alone = 0u;
  int known = 0, marked = 0;
  on_idea_t idea;
  memset(out, 0, sizeof(*out));
  W->box_on = 1;
  W->judge = rule;
  begin(&M, O, off, seed);
  out->room = room(&M);
  while (W->moments - start < moments) {
    if (!marked && W->moments - start >= moments - 300u) {
      marked = 1;
      mark_t = W->moments;
      mark_c = W->chimes;
    }
    if (known) {
      /* it uses what it knows: the simplest acts it says will chime */
      of_test_t t = {0u, 0u}, done;
      unsigned a, p;
      for (p = 0u; p < 4u && !t.acts; p++)
        for (a = 1u; a < 64u && !t.acts; a++) {
          of_test_t c = {a, p};
          if (on_says(&out->known, &c)) t = c;
        }
      try_it(&M, W, &t, &done);
      continue;
    }
    if (out->room == 0u) {
      ob_world_moment(W, O, 0u, 0u);   /* it holds nothing in mind: it lives as it would */
      continue;
    }
    if (held(&M) > 0u) {
      of_test_t meant = choose(&M), done;
      int heard = try_it(&M, W, &meant, &done);
      M.tests++;
      out->tries++;
      outcome(&M, &done, heard);
      since_alone = held(&M) == 1u ? since_alone + 1u : 0u;
      if (knows(&M, since_alone, &idea)) {
        known = 1;
        out->known = idea;
        out->moments = W->moments - start;
      }
    } else {
      /* nothing in mind: it lives a moment of itself; the hippocampus keeps
         what it did and what it heard */
      unsigned bits;
      of_test_t t;
      int heard;
      t.place = quarter_of(&W->crib);
      ob_world_moment(W, O, 0u, 0u);
      bits = did(O);
      heard = ob_world_moment(W, O, 0u, 0u) > 0.5f;
      if (bits) {
        t.acts = bits;
        remember(&M, &t, heard);
        mfg(&M, &t, heard);
      }
    }
    /* between tests, idle a moment: the default mode network */
    dmn(&M);
    out->cortisol += O->cortisol;
    out->dopamine += O->dopamine;
    out->oxytocin += O->oxytocin;
    n_chem++;
  }
  out->found = known && rule_equals(rule, &out->known);
  if (!known) out->tries = 0u;
  memcpy(out->arisen, M.arisen, sizeof out->arisen);
  out->rejected = M.rejected;
  out->struck = M.struck;
  if (n_chem) {
    out->cortisol /= (float)n_chem;
    out->dopamine /= (float)n_chem;
    out->oxytocin /= (float)n_chem;
  }
  out->chime_rate = marked && W->moments > mark_t ? (double)(W->chimes - mark_c) / (double)(W->moments - mark_t) : 0.0;
  end(&M);
  W->box_on = 0;
  W->judge = 0;
}
