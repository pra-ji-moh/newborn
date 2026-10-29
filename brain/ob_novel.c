/*
 * ob_novel.c -- the conditions for novelty: ideas nobody writes, formed on
 * synapses. See ob_novel.h.
 */

#include "ob_novel.h"

#include <stdio.h>
#include <string.h>

/* the DLPFC's holding, as ob_frontal.c's: self-excitation through myelinated
   axons, one shared pool of inhibition */
#define SELF 7.2f
#define SHARED 0.088f
#define LIT 0.5f
#define STRIKE 10.0f
#define CUE 8.0f

#define INH_GAIN 2.0f                 /* an inhibitory synapse, through its interneuron, outweighs an excitatory one */
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

static unsigned rnd(on_mind_t *M) {
  M->rng ^= M->rng << 13;
  M->rng ^= M->rng >> 17;
  M->rng ^= M->rng << 5;
  return M->rng;
}

static float unit(on_mind_t *M) { return (float)(rnd(M) >> 8) / 16777216.0f; }

static float clip(float x) { return x < 0.0f ? 0.0f : x > 1.0f ? 1.0f : x; }

/* ----- what a group with these synapses does ----- */

static float frac(const unsigned char *syn) {
  unsigned k, n = 0u;
  for (k = 0u; k < ON_K; k++) n += syn[k];
  return (float)n / (float)ON_K;
}

int on_fires(const on_idea_t *i, const of_test_t *t) {
  float basal = 0.0f;
  unsigned a;
  /* the context: where its eyes are reaches the apical dendrites */
  if (frac(i->exc[OL_MOTORS + t->place]) < 0.5f) return 0;
  /* what it does reaches the basal dendrites */
  for (a = 0u; a < OL_MOTORS; a++)
    if (t->acts >> a & 1u) basal += frac(i->exc[a]) - INH_GAIN * frac(i->inh[a]);
  return basal > i->theta - 0.5f;
}

void on_describe(const on_idea_t *i, char *out, unsigned size) {
  unsigned a, q, n = 0u;
  int w;
  w = snprintf(out, size, "fires on %.0f of acts {", (double)i->theta);
  n = w > 0 ? (unsigned)w : 0u;
  for (a = 0u; a < OL_MOTORS && n < size; a++)
    if (frac(i->exc[a]) >= 0.5f) n += (unsigned)snprintf(out + n, size - n, " %u", a);
  if (n < size) n += (unsigned)snprintf(out + n, size - n, " }");
  for (a = 0u; a < OL_MOTORS && n < size; a++)
    if (frac(i->inh[a]) >= 0.25f) n += (unsigned)snprintf(out + n, size - n, " not with %u", a);
  if (n < size) n += (unsigned)snprintf(out + n, size - n, ", at quarters");
  for (q = 0u; q < 4u && n < size; q++)
    if (frac(i->exc[OL_MOTORS + q]) >= 0.5f) n += (unsigned)snprintf(out + n, size - n, " %u", q);
}

/* the same, as far as anything it could do would show */
static int same(const on_idea_t *a, const on_idea_t *b) {
  unsigned acts, place;
  for (place = 0u; place < 4u; place++)
    for (acts = 1u; acts < 64u; acts++) {
      of_test_t t = {acts, place};
      if (on_fires(a, &t) != on_fires(b, &t)) return 0;
    }
  return 1;
}

static int fires_ever(const on_idea_t *i) {
  unsigned place;
  for (place = 0u; place < 4u; place++) {
    of_test_t t = {63u, place};
    unsigned acts;
    for (acts = 1u; acts < 64u; acts++) {
      t.acts = acts;
      if (on_fires(i, &t)) return 1;
    }
  }
  return 0;
}

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

/* a free group, the one that wins this moment (excitability and noise) */
static int free_group(on_mind_t *M) {
  unsigned g, n = 0u;
  int pick = -1;
  for (g = 0u; g < M->n_groups; g++)
    if (!M->holds[g] && rnd(M) % (++n) == 0u) pick = (int)g;
  return pick;
}

/* a free group as readily fired as `theta`, if there is one */
static int free_group_like(on_mind_t *M, float theta) {
  unsigned g, n = 0u;
  int pick = -1;
  for (g = 0u; g < M->n_groups; g++)
    if (!M->holds[g] && M->theta[g] == theta && rnd(M) % (++n) == 0u) pick = (int)g;
  return pick >= 0 ? pick : free_group(M);
}

/* the group keeps itself lit, if it will without putting out another */
static int hold(on_mind_t *M, unsigned g0, const on_idea_t *idea) {
  float trial[ON_HELD];
  unsigned g, n = M->n_groups;
  memcpy(trial, M->lit, sizeof(float) * n);
  trial[g0] = 0.0f;
  settle(M, trial, 0, (int)g0);
  if (trial[g0] < LIT) return 0;
  for (g = 0u; g < n; g++)
    if (g != g0 && M->lit[g] >= LIT && trial[g] < LIT) return 0;
  memcpy(M->lit, trial, sizeof(float) * n);
  M->holds[g0] = 1;
  M->idea[g0] = *idea;
  show(M);
  return 1;
}

/* ----- the hippocampus and the ACC ----- */

static void remember(on_mind_t *M, const of_test_t *t, int outcome) {
  on_result_t *r;
  float boost = 2.0f * M->O->cortisol + M->O->dopamine + M->O->oxytocin;
  if (M->n_mem >= ON_MEMORY || t->acts == 0u) return;   /* nothing done: nothing to keep */
  r = &M->mem[M->n_mem++];
  r->t = *t;
  r->outcome = outcome;
  /* how well it is laid down: the chemistry of the moment strengthens it */
  r->strength = 0.7f + 0.3f * clip(boost);
}

/* the hippocampus replays each remembered test into the populations; the
   ACC compares what the group does with what came. 1: it survives */
static int survives_memory(on_mind_t *M, const on_idea_t *idea) {
  unsigned k;
  if (M->off & (ON_OFF_ACC | ON_OFF_MEMORY)) return 1;
  for (k = 0u; k < M->n_mem; k++) {
    const on_result_t *r = &M->mem[k];
    if (on_fires(idea, &r->t) != r->outcome && unit(M) < r->strength) return 0;
  }
  return 1;
}

/* a group has just been bound: the ACC tests it; it is held if it survives
   and is new; otherwise its synapses are let go */
static void arise(on_mind_t *M, int g, on_idea_t *idea) {
  unsigned h;
  if (g < 0) return;
  idea->theta = M->theta[g];
  if (!fires_ever(idea)) return;     /* nothing took: no idea */
  M->arisen[idea->source]++;
  if (!survives_memory(M, idea)) {
    M->rejected++;
    M->quiet++;
    return;
  }
  for (h = 0u; h < M->n_groups; h++)
    if (M->holds[h] && same(&M->idea[h], idea)) {
      M->quiet++;
      return;
    }
  if (hold(M, (unsigned)g, idea)) M->quiet = 0u;
}

/* ----- binding: synapses from the active populations take ----- */

/* each population's burst reaches the group or not (p_burst); each of its
   synapses takes or not */
static void bind_acts(on_mind_t *M, on_idea_t *i, unsigned acts, float p_burst) {
  unsigned a, k;
  for (a = 0u; a < OL_MOTORS; a++)
    if ((acts >> a & 1u) && unit(M) < p_burst)
      for (k = 0u; k < ON_K; k++) i->exc[a][k] = unit(M) < 0.9f;
}

static void bind_places(on_mind_t *M, on_idea_t *i, unsigned places) {
  unsigned q, k;
  for (q = 0u; q < 4u; q++)
    if (places >> q & 1u)
      for (k = 0u; k < ON_K; k++) i->exc[OL_MOTORS + q][k] = unit(M) < 0.95f;
}

static float burst_p(const on_mind_t *M) { return clip(0.6f + 0.3f * M->O->dopamine); }

/* the PPA: the larger picture. Every quarter it has heard the box in */
static unsigned scene(const on_mind_t *M, unsigned place) {
  unsigned k, places = 1u << place;
  for (k = 0u; k < M->n_mem; k++)
    if (M->mem[k].outcome) places |= 1u << M->mem[k].t.place;
  return places;
}

static unsigned context(on_mind_t *M, unsigned place, unsigned *source) {
  if (!(M->off & ON_OFF_PPA) && unit(M) < 0.3f * M->pathway[ON_PPA]) {
    *source = ON_PPA;
    return scene(M, place);
  }
  return 1u << place;
}

/* the MFG: the VAN flags something unexpected; a binding window opens */
static void mfg(on_mind_t *M, const of_test_t *t, int outcome, const on_idea_t *failed) {
  unsigned k, n;
  if (M->off & ON_OFF_MFG) return;
  n = 1u + (unsigned)(3.0f * M->O->cortisol * M->pathway[ON_MFG]);
  for (k = 0u; k < n; k++) {
    on_idea_t idea;
    /* a failed idea is bound again onto a group as readily fired as its own */
    int g = failed && !outcome ? free_group_like(M, failed->theta) : free_group(M);
    if (g < 0) return;
    memset(&idea, 0, sizeof idea);
    idea.source = ON_MFG;
    if (outcome) {
      /* it chimed and nothing foretold it: what it did, where it looked */
      bind_acts(M, &idea, t->acts, burst_p(M));
      bind_places(M, &idea, context(M, t->place, &idea.source));
    } else if (failed) {
      /* an idea it held said chime, and none came. Either something it binds
         does not matter (let one of them go), or something there besides
         spoils it (inhibition from that: failure teaches "not with that").
         Both are formed, each on a group of its own; memory decides */
      const on_idea_t *was = failed;
      unsigned a, besides = 0u, own = 0u, j, pick, v;
      for (a = 0u; a < OL_MOTORS; a++) {
        if (!(t->acts >> a & 1u)) continue;
        if (frac(was->exc[a]) < 0.5f) besides |= 1u << a;
        else own |= 1u << a;
      }
      for (v = 0u; v < 2u; v++) {
        on_idea_t var;
        int gv = v == 0u ? g : free_group_like(M, was->theta);
        if (gv < 0) break;
        memcpy(&var, was, sizeof var);
        var.source = ON_MFG;
        if (v == 0u && besides && !(M->off & ON_OFF_INHIBIT)) {
          for (a = 0u; a < OL_MOTORS; a++)
            if ((besides >> a & 1u) && unit(M) < burst_p(M))
              for (j = 0u; j < ON_K; j++) var.inh[a][j] = unit(M) < 0.9f;
          M->inhibitions++;
        } else if (v == 1u && popcount(own) > 1u) {
          /* one of its own acts let go */
          pick = rnd(M) % popcount(own);
          for (a = 0u; a < OL_MOTORS; a++)
            if (own >> a & 1u) {
              if (pick == 0u) break;
              pick--;
            }
          memset(var.exc[a], 0, ON_K);
        } else if (v == 0u) {
          /* nothing besides: one act more or less */
          a = rnd(M) % OL_MOTORS;
          for (j = 0u; j < ON_K; j++) var.exc[a][j] = frac(was->exc[a]) >= 0.5f ? 0u : unit(M) < 0.9f;
        } else {
          break;
        }
        arise(M, gv, &var);
      }
      continue;
    } else {
      /* something new in front of it, and nothing happened: what it just
         did, varied (an act more or less, its eyes elsewhere). Memory
         turns away whatever it has already tried */
      bind_acts(M, &idea, t->acts ^ (1u << (rnd(M) % OL_MOTORS)), 0.9f);
      bind_places(M, &idea, 1u << (unit(M) < 0.5f ? t->place : rnd(M) % 4u));
    }
    arise(M, g, &idea);
  }
}

/* the DMN: idle, it replays its own life into the populations */
static void dmn(on_mind_t *M) {
  unsigned a = 0u, b = 0u, k, tries;
  on_idea_t idea;
  int g;
  if ((M->off & ON_OFF_DMN) || M->n_mem == 0u) return;
  if (unit(M) > M->pathway[ON_DMN]) return;
  g = free_group(M);
  if (g < 0) return;
  memset(&idea, 0, sizeof idea);
  idea.source = ON_DMN;
  if (M->n_good >= 2u && unit(M) < 0.4f) {
    /* two groups that once foretold a chime, active together: a free group
       takes what either excites, and what both inhibit */
    const on_idea_t *x = &M->good[rnd(M) % M->n_good], *y = &M->good[rnd(M) % M->n_good];
    unsigned f, j;
    for (f = 0u; f < ON_FEAT; f++)
      for (j = 0u; j < ON_K; j++) idea.exc[f][j] = x->exc[f][j] | y->exc[f][j];
    for (f = 0u; f < OL_MOTORS; f++)
      for (j = 0u; j < ON_K; j++) idea.inh[f][j] = x->inh[f][j] & y->inh[f][j];
    /* where both look, if they share a place */
    {
      unsigned q, shared = 0u;
      for (q = 0u; q < 4u; q++)
        if (frac(x->exc[OL_MOTORS + q]) >= 0.5f && frac(y->exc[OL_MOTORS + q]) >= 0.5f) shared |= 1u << q;
      if (shared)
        for (q = 0u; q < 4u; q++)
          if (!(shared >> q & 1u))
            for (j = 0u; j < ON_K; j++) idea.exc[OL_MOTORS + q][j] = 0u;
    }
    arise(M, g, &idea);
    return;
  }
  /* two moments it lived, replayed (the ones the box chimed in, most) */
  for (k = 0u; k < 2u; k++) {
    unsigned pick = 0u;
    for (tries = 0u; tries < 20u; tries++) {
      pick = rnd(M) % M->n_mem;
      if (M->mem[pick].outcome || unit(M) < 0.1f) break;
    }
    if (k == 0u) a = pick;
    else b = pick;
  }
  if (M->mem[a].outcome && M->mem[b].outcome) {
    if (unit(M) < 0.5f) {
      /* synapses take only where both moments were active: what they share */
      bind_acts(M, &idea, M->mem[a].t.acts & M->mem[b].t.acts, 0.95f);
    } else {
      /* where either was */
      bind_acts(M, &idea, M->mem[a].t.acts | M->mem[b].t.acts, burst_p(M));
    }
    bind_places(M, &idea, (1u << M->mem[a].t.place) | (1u << M->mem[b].t.place));
  } else {
    /* one of them was silence: the chiming one, where it chimed */
    unsigned c = M->mem[a].outcome ? a : b;
    bind_acts(M, &idea, M->mem[c].t.acts, burst_p(M));
    bind_places(M, &idea, 1u << M->mem[c].t.place);
  }
  arise(M, g, &idea);
}

/* ----- the IPS: which test ----- */

static int tried(const on_mind_t *M, const of_test_t *t) {
  unsigned m;
  for (m = 0u; m < M->n_mem; m++)
    if (M->mem[m].t.acts == t->acts && M->mem[m].t.place == t->place) return 1;
  return 0;
}

static of_test_t choose(on_mind_t *M) {
  unsigned g, acts, place, h = held(M), ties = 0u;
  int best_split = -1;
  of_test_t best = {1u, 0u};
  if (h == 1u) {
    /* one idea: check it at its edges, where it would be wrong if it were
       wrong. The least it fires for (no fewer acts would do), and that with
       one act more; the nearest it does not fire for (one act more, less,
       or swapped; or elsewhere); and now and then anything it fires for. Each new to it, if it can find one */
    const on_idea_t *only = 0;
    unsigned least[64], n_least = 0u, i, j, start;
    of_test_t cand[256];
    unsigned n = 0u;
    for (g = 0u; g < M->n_groups; g++)
      if (M->holds[g] && M->lit[g] >= LIT) only = &M->idea[g];
    for (place = 0u; place < 4u; place++)
      for (acts = 1u; acts < 64u; acts++) {
        of_test_t t = {acts, place};
        int minimal = on_fires(only, &t);
        unsigned a;
        for (a = 0u; a < OL_MOTORS && minimal; a++)
          if (acts >> a & 1u) {
            of_test_t s = {acts & ~(1u << a), place};
            if (s.acts && on_fires(only, &s)) minimal = 0;
          }
        if (minimal && n_least < 64u) least[n_least++] = acts | place << 8;
      }
    for (i = 0u; i < n_least; i++) {
      unsigned acts0 = least[i] & 63u, p0 = least[i] >> 8;
      if (n < 256u) cand[n++] = (of_test_t){acts0, p0};
      for (place = 0u; place < 4u; place++)
        for (j = 0u; j < 64u; j++) {
          of_test_t t;
          unsigned near;
          if (j == 0u) continue;
          near = popcount(j ^ acts0);
          t.acts = j;
          t.place = place;
          if (on_fires(only, &t)) {
            /* the least it fires for, and one act more: does adding
               something spoil it? */
            if (place == p0 && near == 1u && (j & acts0) == acts0 && n < 256u) cand[n++] = t;
            continue;
          }
          if ((place == p0 && (near == 1u || (near == 2u && popcount(j) == popcount(acts0)))) ||
              (place != p0 && j == acts0))
            if (n < 256u) cand[n++] = t;
        }
    }
    if (n == 0u || (M->tests % 4u) == 0u) {
      for (ties = 0u; ties < 400u; ties++) {
        of_test_t t = {1u + rnd(M) % 63u, rnd(M) % 4u};
        if (on_fires(only, &t)) return t;
      }
      return n ? cand[0] : best;
    }
    start = rnd(M) % n;
    for (i = 0u; i < n; i++)
      if (!tried(M, &cand[(start + i) % n])) return cand[(start + i) % n];
    return cand[start];
  }
  /* several: the test that splits them most evenly */
  for (place = 0u; place < 4u; place++)
    for (acts = 1u; acts < 64u; acts++) {
      of_test_t t = {acts, place};
      unsigned yes = 0u, split;
      for (g = 0u; g < M->n_groups; g++)
        if (M->holds[g] && M->lit[g] >= LIT && on_fires(&M->idea[g], &t)) yes++;
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

/* a test, through its body: what it actually did (its efference copy, which
   may be less than it meant), where, and what it heard */
static int try_it(on_mind_t *M, ob_world_t *W, const of_test_t *meant, of_test_t *done) {
  look_to(M, W, meant->place);
  done->place = quarter_of(&W->crib);
  ob_world_moment(W, M->O, 1u, through(M, meant->acts));
  done->acts = did(M->O);
  return ob_world_moment(W, M->O, 1u, 0u) > 0.5f;
}

/* ----- the meeting ----- */

static void begin(on_mind_t *M, ob_one_t *O, unsigned off, unsigned seed) {
  /* how readily a group fires, by the layer its neurons sit in (deepest
     first: 6, 5, 4, 3, 2). A modelling choice: groups must differ, and here
     the layer says how */
  static const float THETA_OF_LAYER[OB_LAYERS] = {1.0f, 2.0f, 1.0f, 2.0f, 3.0f};
  const ob_brain_t *b = &O->brain;
  unsigned i, a, layer, n[ON_FEAT];
  memset(M, 0, sizeof(*M));
  M->O = O;
  M->off = off;
  M->rng = seed * 2654435761u + 12345u;
  if (!M->rng) M->rng = 1u;
  for (a = 0u; a < ON_SOURCES; a++) M->pathway[a] = 0.5f;
  /* the DLPFC's groups, six neurons of one layer each */
  for (layer = 0u; layer < OB_LAYERS; layer++) {
    unsigned in_group = 0u;
    for (i = 0u; i < b->n_neurons && M->n_groups < ON_HELD; i++) {
      const ob_neuron_t *nn = &b->neuron[i];
      if (nn->inhibitory || on_area(nn) != ON_DLPFC || nn->layer != layer) continue;
      M->neuron[M->n_groups][in_group] = i;
      O->external[i] = 1u;
      if (++in_group == 6u) {
        in_group = 0u;
        M->theta[M->n_groups] = THETA_OF_LAYER[layer];
        M->n_groups++;
      }
    }
  }
  /* the populations: each act's zone of the brainstem, each quarter of the
     tectum's map */
  memset(n, 0, sizeof n);
  for (i = 0u; i < b->n_neurons; i++) {
    const ob_neuron_t *nn = &b->neuron[i];
    unsigned f = ON_FEAT;
    if (nn->region == OB_BRAINSTEM && ob_one_zone(nn) < OL_MOTORS) f = ob_one_zone(nn);
    if (nn->region == OB_TECTUM) f = OL_MOTORS + (nn->sx >= 0.5f) + 2u * (nn->sy >= 0.5f);
    if (f < ON_FEAT && n[f] < ON_K) M->feat[f][n[f]++] = i;
  }
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
  int failed = -1;
  memset(push, 0, sizeof push);
  for (g = 0u; g < M->n_groups; g++) {
    if (!M->holds[g] || M->lit[g] < LIT) continue;
    if (on_fires(&M->idea[g], t)) expected_chime++;
    if (on_fires(&M->idea[g], t) == heard) {
      if (heard) {
        foretold++;
        push[g] = -1.0f;               /* dopamine steadies the group that foretold it */
        M->pathway[M->idea[g].source] += 0.05f * (1.0f - M->pathway[M->idea[g].source]);
        if (M->n_good < 32u) M->good[M->n_good++] = M->idea[g];
        else M->good[rnd(M) % 32u] = M->idea[g];
      }
    } else {
      wrong++;
      if (!heard) failed = (int)g;
      if (!(M->off & ON_OFF_ACC)) push[g] = STRIKE;
    }
  }
  /* cortisol: it expected the chime and it did not come */
  if (!heard && expected_chime) M->O->failure_now = 1.0f;
  remember(M, t, heard);
  settle(M, M->lit, push, -1);
  for (g = 0u; g < M->n_groups; g++)
    if (M->holds[g] && push[g] >= STRIKE && M->lit[g] < LIT) M->struck++;
  /* the MFG binds the failed idea again, as the ACC's darkening lets its
     group go (its synapses still in the binding window) */
  if (failed >= 0) {
    on_idea_t was = M->idea[failed];
    forget_dark(M);
    mfg(M, t, 0, &was);
  } else {
    forget_dark(M);
  }
  show(M);
  /* the unexpected (the VAN): a chime nothing foretold */
  if (heard && !foretold) mfg(M, t, 1, 0);
  (void)wrong;
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
      if (rule(&t) != on_fires(idea, &t)) return 0;
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
      /* it uses what it knows: the least it fires for */
      of_test_t t = {0u, 0u}, done;
      unsigned a, p;
      for (p = 0u; p < 4u && !t.acts; p++)
        for (a = 1u; a < 64u && !t.acts; a++) {
          of_test_t c = {a, p};
          if (on_fires(&out->known, &c)) t = c;
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
        mfg(&M, &t, heard, 0);
      }
    }
    /* between tests, idle a moment: the default mode network */
    dmn(&M);
    out->cortisol += O->cortisol;
    out->dopamine += O->dopamine;
    out->oxytocin += O->oxytocin;
    n_chem++;
  }
  out->claimed = known;
  out->found = known && rule_equals(rule, &out->known);
  if (!known) out->tries = 0u;
  memcpy(out->arisen, M.arisen, sizeof out->arisen);
  out->rejected = M.rejected;
  out->struck = M.struck;
  out->inhibitions = M.inhibitions;
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
