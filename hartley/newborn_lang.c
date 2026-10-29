/* The relational model, learning the newborn's way: from nothing, one
 * document at a time, with ideas nobody writes for words it has never met.
 *
 * From 0 (serve and return). Nothing is known at the start: no counts, no
 * vocabulary, no word kept as important. Documents arrive one at a time, in
 * a single pass. Every question in a document is answered before its answer
 * is seen; then the answer comes, and the counts take it in (a chain that
 * reached it gains trust, one that was present and missed loses it). The
 * first-try answers are scored as the stream goes, and at checkpoints the
 * model, frozen, is tested on held-out documents.
 *
 * What it reads with is longtext.c's: divide into sentences; Hartley cuts
 * each to its surprising words (a word never met is surprising; Shannon's
 * surprise from the counts so far) and the words questions ask for (the
 * answer after a question at least once in twenty appearances); every ordered pair of kept words is a relation named by the words
 * between; Shannon values relations and drops the least valued when the
 * memory (64) is full; questions are answered by chains of relations
 * followed in order, trusted by how often they were right.
 *
 * Questions are marked, as a caregiver's are: a sentence holding a "?" is a
 * question, and the word that follows is its answer (serve and return).
 * Kinds of words (learned): a word's kind is which questions it has been
 * the answer to ("_ city ?", "who likes _ ?", ...), counted as the stream
 * goes. Each kind of relation remembers the kinds of words seen at its two
 * ends.
 *
 * New words: ideas nobody writes (ob_novel.c's loop). A relation in wording
 * seen fewer than 3 times, between two words that questions ask for:
 *   DMN / MFG    propose: "it is like <a relation already useful>, this way
 *                round or reversed", for each of the most useful ones.
 *   ACC          tests each idea against what is known: the kinds of words at
 *                its ends must fit the kinds that relation joins. An idea
 *                whose ends don't fit (a city where a person belongs) is
 *                ruled out (Hartley).
 *   DLPFC        holds the survivors, weighted by Shannon, and the chains
 *                use them in place of the unknown wording.
 * Passives (Wernicke): which way round a wording runs is learned from the
 * words it is made of. For the questions that relations answer, each
 * wording's direction is compared with the main one's ("is liked by" runs
 * against "likes"); the count is kept by the wording's last word. "... by"
 * turns out to reverse. A new wording ending in "by" is then read reversed,
 * whatever its verb; one ending in a word never met takes the usual way.
 * When the kinds of words at the ends already settle the direction (a city
 * and a person), they decide; the frame word decides when they cannot (two
 * people).
 *
 * Build: cc -std=c99 -O2 newborn_lang.c -o newborn_lang -lm
 * Run:   ./newborn_lang DATA_DIR [no_ideas|no_wernicke]   (gen_long3.py's folder)
 *        controls: no ideas for new wordings; ideas without the frame word
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef unsigned long long u64;
static int V, DOT;
typedef struct { int n, *len, **w, **mask; long tokens; } Docs;

static void load(Docs *D, const char *path, const char *mpath) {
    FILE *f = fopen(path, "r"), *g = mpath ? fopen(mpath, "r") : NULL; if (!f) { perror(path); exit(1); }
    int cap = 256; D->n = 0; D->len = malloc(cap * sizeof(int)); D->w = malloc(cap * sizeof(int *)); D->mask = malloc(cap * sizeof(int *)); D->tokens = 0;
    size_t bufsz = 1 << 24; char *line = malloc(bufsz); int *buf = malloc(sizeof(int) * (1 << 20));
    while (fgets(line, (int)bufsz, f)) {
        int n = 0; char *p = line, *e;
        for (;;) { long x = strtol(p, &e, 10); if (e == p) break; buf[n++] = (int)x; p = e; }
        if (D->n == cap) { cap *= 2; D->len = realloc(D->len, cap * sizeof(int)); D->w = realloc(D->w, cap * sizeof(int *)); D->mask = realloc(D->mask, cap * sizeof(int *)); }
        D->w[D->n] = malloc(n * sizeof(int)); memcpy(D->w[D->n], buf, n * sizeof(int)); D->mask[D->n] = calloc(n, sizeof(int));
        if (g && fgets(line, (int)bufsz, g)) { p = line; for (int t = 0; t < n; t++) { long x = strtol(p, &e, 10); if (e == p) break; D->mask[D->n][t] = (int)x; p = e; } }
        D->len[D->n++] = n; D->tokens += n;
    }
    fclose(f); if (g) fclose(g); free(line); free(buf);
}

/* ---------- one hash for every count ---------- */

typedef struct { u64 key; double c; int types; } Ent;
static Ent *H; static unsigned hmask;
static Ent *get(u64 key, int create) {
    for (unsigned h = (unsigned)((key * 0x9E3779B97F4A7C15ULL) >> 38) & hmask;; h = (h + 1) & hmask) {
        if (H[h].key == key) return &H[h];
        if (!H[h].key) { if (!create) return NULL; H[h].key = key; return &H[h]; } }
}
static u64 K(int kind, u64 a, u64 b) {
    u64 h = a * 0x100000001B3ULL ^ (b + 0x9E3779B97F4A7C15ULL) * 0xC2B2AE3D27D4EB4FULL; h ^= h >> 31;
    return ((u64)kind << 58) | (h & ((1ULL << 58) - 1)) | 1; }
static double cnt(u64 key) { Ent *e = get(key, 0); return e ? e->c : 0; }

/* counts so far: words, and the trigram (restarting at each ".") */
static double *uni; static double total;
static void see_word(int a, int b, int w) {
    Ent *x;
    x = get(K(3, (u64)(a + 2) << 20 | (u64)(b + 2), w), 1); if (!x->c++) get(K(5, (u64)(a + 2) << 20 | (u64)(b + 2), 0), 1)->types++;
    get(K(5, (u64)(a + 2) << 20 | (u64)(b + 2), 0), 1)->c++;
    x = get(K(2, b + 2, w), 1); if (!x->c++) get(K(4, b + 2, 0), 1)->types++;
    get(K(4, b + 2, 0), 1)->c++;
    uni[w]++; total++;
}
static double p_uni(int w) { return (uni[w] + 0.5) / (total + 20000); }   /* a word never met: about 1 in 20,000 */
static double trigram(int a, int b, int w) {
    double p = p_uni(w); Ent *c, *x;
    if ((c = get(K(4, b + 2, 0), 0)) && c->c) { x = get(K(2, b + 2, w), 0); double l = c->c / (c->c + c->types); p = l * (x ? x->c : 0) / c->c + (1 - l) * p; }
    if ((c = get(K(5, (u64)(a + 2) << 20 | (u64)(b + 2), 0), 0)) && c->c) { x = get(K(3, (u64)(a + 2) << 20 | (u64)(b + 2), w), 0); double l = c->c / (c->c + c->types); p = l * (x ? x->c : 0) / c->c + (1 - l) * p; }
    return p;
}

/* ---------- questions, and the kinds of words ---------- */

#define NQ 32
static u64 qt[NQ]; static int nqt;          /* question-like templates, in the order met */
static int q_index(u64 T, int add) {      /* add: the template asks (it holds a "?") */
    for (int i = 0; i < nqt; i++) if (qt[i] == T) return i;
    if (!add || nqt == NQ) return -1;
    qt[nqt] = T; return nqt++;
}
/* a word's kind: which questions it has answered, as shares */
static void kind_of(int w, double *v) {
    double s = 0;
    for (int i = 0; i < NQ; i++) { v[i] = i < nqt ? cnt(K(20, w, i)) : 0; s += v[i]; }
    if (s > 0) for (int i = 0; i < NQ; i++) v[i] /= s;
}
/* worth keeping because questions ask for it: the times it came right after
 * a question, against the times it appears at all */
static int useful(int w) { double o = cnt(K(14, w, 0)); return o >= 5 && o >= 0.05 * uni[w]; }

#define THETA 9.5
static int kept(int w) { return w != DOT && (-log2(p_uni(w)) >= THETA || useful(w)); }

/* each kind of relation: the kinds of words at its two ends, and its last word */
#define NSLOT 65536
static float (*ends)[2][NQ]; static int nslot;
static int slot_of(u64 rel, int add) {
    Ent *e = get(K(22, rel, 0), 0);
    if (e) return (int)e->c - 1;
    if (!add || nslot == NSLOT) return -1;
    get(K(22, rel, 0), 1)->c = ++nslot; return nslot - 1;
}

/* ---------- relations in memory ---------- */

#define CAP 64
#define NHYP 3
typedef struct { u64 rel; int flip; double w; } Hyp;
typedef struct { int x, y; u64 rel; double count, value; long seen; Hyp h[NHYP]; int nh; } Rel;
typedef struct { Rel r[CAP + 1]; int n; long clock; } Store;

static double usefulness(u64 rel) { return (cnt(K(9, rel, 0)) + 0.01) / (cnt(K(10, rel, 0)) + 1); }

/* ---------- Wernicke: which way a wording runs, from its last word ---------- */

/* For each question, the one-step relations that answered it, with their
 * direction: the most used is the main one; the others are compared with it.
 * K 23 (last word, same way) and K 24 (last word, reversed). Rebuilt now and
 * then from the counts (K 25: one-step chain hits by template). */
#define NTOP 16
typedef struct { u64 rel; int dir; double c; } Top;
static Top top[NQ][NTOP]; static int ntop[NQ];
static int lastword_of(u64 rel) { Ent *e = get(K(21, rel, 0), 0); return e ? (int)e->c - 1 : -1; }
static void note_onestep(int qi, u64 rel, int dir) {
    for (int j = 0; j < ntop[qi]; j++) if (top[qi][j].rel == rel && top[qi][j].dir == dir) { top[qi][j].c++; return; }
    if (ntop[qi] < NTOP) top[qi][ntop[qi]++] = (Top){rel, dir, 1};
    else { int m = 0; for (int j = 1; j < NTOP; j++) if (top[qi][j].c < top[qi][m].c) m = j; if (top[qi][m].c <= 1) top[qi][m] = (Top){rel, dir, 1}; }
}
static double fr_same[1 << 16], fr_rev[1 << 16], fr_all[2];
static int orient(u64 rel) {          /* a wording's way relative to the main one of a question it answers */
    for (int q = 0; q < nqt; q++) { int m = 0; if (!ntop[q]) continue;
        for (int j = 1; j < ntop[q]; j++) if (top[q][j].c > top[q][m].c) m = j;
        for (int j = 0; j < ntop[q]; j++) if (top[q][j].rel == rel && top[q][j].c >= 5) return top[q][j].dir != top[q][m].dir; }
    return 0;
}
static void rebuild_frames(void) {
    memset(fr_same, 0, sizeof fr_same); memset(fr_rev, 0, sizeof fr_rev); fr_all[0] = fr_all[1] = 0;
    for (int q = 0; q < nqt; q++) { if (!ntop[q]) continue; int m = 0;
        for (int j = 1; j < ntop[q]; j++) if (top[q][j].c > top[q][m].c) m = j;
        for (int j = 0; j < ntop[q]; j++) { if (top[q][j].c < 5) continue;
            int lw = lastword_of(top[q][j].rel), rev = top[q][j].dir != top[q][m].dir;
            if (lw >= 0 && lw < (1 << 16)) { if (rev) fr_rev[lw]++; else fr_same[lw]++; }
            fr_all[rev]++; } }
}
static int no_wernicke;              /* control: the frame word has no say */
static double p_reversed(int lw) {    /* Shannon: how likely a wording ending in this word runs reversed */
    if (no_wernicke) return 0.5;
    double base = (fr_all[1] + 1) / (fr_all[0] + fr_all[1] + 2);
    if (lw < 0 || lw >= (1 << 16)) return base;
    return (fr_rev[lw] + 2 * base) / (fr_same[lw] + fr_rev[lw] + 2);
}

/* ---------- ideas nobody writes, for wordings never met ---------- */

#define NCAND 24
static u64 cand[NCAND]; static int ncand;
/* The most useful relations so far (with ends known) are the ideas' material,
 * kept as relations become useful (K 9 grows): */
static void consider(u64 rel) {
    double u = cnt(K(9, rel, 0));
    if (u < 10 || slot_of(rel, 0) < 0) return;
    for (int i = 0; i < ncand; i++) if (cand[i] == rel) return;
    if (ncand < NCAND) { cand[ncand++] = rel; return; }
    int m = 0; for (int i = 1; i < ncand; i++) if (cnt(K(9, cand[i], 0)) < cnt(K(9, cand[m], 0))) m = i;
    if (cnt(K(9, cand[m], 0)) < u) cand[m] = rel;
}
static double cosv(const double *a, const float *b) {
    double ab = 0, aa = 0, bb = 0;
    for (int i = 0; i < NQ; i++) { ab += a[i] * b[i]; aa += a[i] * a[i]; bb += (double)b[i] * b[i]; }
    return aa > 0 && bb > 0 ? ab / sqrt(aa * bb) : 0;
}
static int no_ideas;                 /* control: wordings never met get no ideas */
static int ideas(Rel *r, int lastword) {
    r->nh = 0; if (no_ideas) return 0;
    if (!useful(r->x) || !useful(r->y)) return 0;      /* only relations between words that questions ask for */
    double kx[NQ], ky[NQ]; kind_of(r->x, kx); kind_of(r->y, ky);
    double pr = p_reversed(lastword), best = 0;
    Hyp all[NCAND * 2]; int na = 0;
    for (int i = 0; i < ncand; i++) { int s = slot_of(cand[i], 0); if (s < 0) continue;
        int o = orient(cand[i]);
        for (int f = 0; f < 2; f++) {
            /* ACC: do the kinds at the ends fit? (f: read reversed) */
            double fit = f ? cosv(ky, ends[s][0]) * cosv(kx, ends[s][1]) : cosv(kx, ends[s][0]) * cosv(ky, ends[s][1]);
            if (fit < 0.25) continue;                                   /* Hartley: ruled out */
            /* Wernicke: the frame word's say on the way round, relative to this relation's own way */
            double way = (f ^ o) ? pr : 1 - pr;
            double w = fit * (0.25 + way) * cnt(K(9, cand[i], 0));
            all[na++] = (Hyp){cand[i], f, w}; if (w > best) best = w; } }
    for (int i = 0; i < na && r->nh < NHYP; i++) {        /* DLPFC: hold the strongest */
        int m = i; for (int j = i + 1; j < na; j++) if (all[j].w > all[m].w) m = j;
        Hyp t = all[i]; all[i] = all[m]; all[m] = t;
        if (all[i].w < 0.2 * best) break;
        r->h[r->nh++] = all[i]; }
    double s = 0; for (int i = 0; i < r->nh; i++) s += r->h[i].w;
    for (int i = 0; i < r->nh; i++) r->h[i].w /= s;
    return r->nh;
}

static void store_rel(Store *S, int x, int y, u64 rel, int lastword) {
    S->clock++;
    for (int i = 0; i < S->n; i++) if (S->r[i].x == x && S->r[i].y == y && S->r[i].rel == rel) {
        S->r[i].count++; S->r[i].seen = S->clock; S->r[i].value = S->r[i].count * (S->r[i].nh ? 1 : usefulness(rel)); return; }
    Rel *r = &S->r[S->n++];
    *r = (Rel){x, y, rel, 1, usefulness(rel), S->clock, {{0}}, 0};
    if (cnt(K(10, rel, 0)) < 3 && ideas(r, lastword)) {          /* a new wording: valued like the relation it is taken for */
        double v = 0; for (int i = 0; i < r->nh; i++) v += r->h[i].w * usefulness(r->h[i].rel); r->value = v; }
    if (S->n > CAP) { int worst = 0;
        for (int i = 1; i < S->n; i++) if (S->r[i].value < S->r[worst].value || (S->r[i].value == S->r[worst].value && S->r[i].seen < S->r[worst].seen)) worst = i;
        S->r[worst] = S->r[--S->n]; }
}
static void absorb(Store *S, const int *s, int n, int learning) {
    int kp[64], nk = 0;
    for (int t = 0; t < n && nk < 64; t++) if (kept(s[t])) kp[nk++] = t;
    for (int i = 0; i < nk; i++) for (int j = i + 1; j < nk; j++) {
        int a = kp[i], b = kp[j]; if (b - a - 1 > 5) continue;
        u64 rel = 7; for (int t = a + 1; t < b; t++) rel = rel * 1000003ULL + (u64)s[t] + 1;
        int lw = b - 1 > a ? s[b - 1] : -1;
        if (learning) {
            get(K(10, rel, 0), 1)->c++;
            if (lw >= 0) get(K(21, rel, 0), 1)->c = lw + 1;
            int sl = slot_of(rel, 1);
            if (sl >= 0) { double kx[NQ], ky[NQ]; kind_of(s[a], kx); kind_of(s[b], ky);
                for (int q = 0; q < NQ; q++) { ends[sl][0][q] += (float)kx[q]; ends[sl][1][q] += (float)ky[q]; } } }
        store_rel(S, s[a], s[b], rel, lw);
    }
}

/* ---------- chains (as longtext.c), with held ideas standing in ---------- */

typedef struct { u64 rel; int dir, idx; } Step;
#define MAXSTEP 3
typedef struct { Step s[MAXSTEP]; int n; } Chain;
static u64 chain_key_m(const Chain *c, int wild) {
    u64 k = c->n + 0x51ULL * (u64)(wild != 0);
    for (int i = 0; i < c->n; i++) k = k * 0x9E3779B97F4A7C15ULL + (wild >> i & 1 ? 0 : c->s[i].rel) * 2 + (u64)c->s[i].dir;
    return k; }
typedef struct { Chain c; int end; double w; } Reach;
static int reach_n, reach_max; static Reach *reach_out;
static void extend(const Store *S, int at, int origin, Chain *c, double w, char *used) {
    for (int i = 0; i < S->n && reach_n < reach_max; i++) for (int d = 0; d < 2 && reach_n < reach_max; d++) {
        const Rel *r = &S->r[i]; int from = d ? r->y : r->x, to = d ? r->x : r->y;
        if (used[i] || from != at || to == origin) continue;
        int nv = r->nh ? r->nh : 1;
        for (int v = 0; v < nv && reach_n < reach_max; v++) {
            u64 rel = r->nh ? r->h[v].rel : r->rel; int dir = r->nh ? d ^ r->h[v].flip : d;
            double hw = r->nh ? r->h[v].w : 1;
            c->s[c->n] = (Step){rel, dir, i}; c->n++;
            reach_out[reach_n++] = (Reach){*c, to, w * r->count * hw};
            if (c->n < MAXSTEP) { used[i] = 1; extend(S, to, origin, c, w * r->count * hw, used); used[i] = 0; }
            c->n--; }
    }
}
#define MAXR 32768
static Reach reach_buf[MAXR];
static int reach_all(const Store *S, int x) {
    char used[CAP + 1] = {0}; Chain c = {.n = 0};
    reach_n = 0; reach_max = MAXR; reach_out = reach_buf;
    extend(S, x, x, &c, 1, used);
    return reach_n;
}
static int QMARK;
static int asking;                   /* set by templ: the sentence so far is a question (it holds a "?") */
static u64 templ(const int *s, int n, int *slot) {
    u64 k = 99; *slot = -1; asking = 0;
    for (int t = (n > 6 ? n - 6 : 0); t < n; t++) {
        if (s[t] == QMARK) asking = 1;
        if (kept(s[t])) { k = k * 1000003ULL + 1; *slot = s[t]; } else k = k * 1000003ULL + (u64)s[t] + 2; }
    return k;
}

/* what memory says comes next */
static int mem_answer(const Store *S, const int *s, int n, int *words, double *p) {
    int slot; u64 T = templ(s, n, &slot); if (slot < 0) return 0;
    double tt = cnt(K(6, T, 0)); if (!tt) return 0;
    int nr = reach_all(S, slot), m = 0; double tot = 0;
    for (int j = 0; j < nr; j++) {
        int wild = 0; for (int q = 0; q < reach_buf[j].c.n; q++) if (!get(K(10, reach_buf[j].c.s[q].rel, 0), 0)) wild |= 1 << q;
        u64 ck = chain_key_m(&reach_buf[j].c, wild);
        double c = cnt(K(7, T, ck)); if (!c) continue;
        double w = c * c / (cnt(K(16, T, ck)) + 1) / tt * reach_buf[j].w; int e = reach_buf[j].end, k;
        for (k = 0; k < m; k++) if (words[k] == e) break;
        if (k == m) { if (m == 64) continue; words[m] = e; p[m] = 0; m++; }
        p[k] += w; tot += w; }
    if (tot <= 0) return 0;
    for (int k = 0; k < m; k++) p[k] /= tot;
    return m;
}

/* learning from one answer that came: which chains reached it */
typedef struct { u64 key; int hit, idx; } KeyHit;
static KeyHit keybuf[MAXR * 8];
static int by_key(const void *a, const void *b) { u64 x = ((const KeyHit *)a)->key, y = ((const KeyHit *)b)->key; return (x > y) - (x < y); }
static void take_in(Store *S, u64 T, int slot, int w) {
    (void)slot;
    get(K(6, T, 0), 1)->c++;
    if (!kept(w)) return;
    int n = reach_all(S, slot), hit = 0;
    for (int j = 0; j < n && !hit; j++) hit = reach_buf[j].end == w;
    if (hit) get(K(15, T, 0), 1)->c++;
    int qi = q_index(T, asking);
    if (qi >= 0) { get(K(14, w, 0), 1)->c++; if (hit) get(K(13, w, 0), 1)->c++; get(K(20, w, qi), 1)->c++; }
    int nk = 0;
    for (int j = 0; j < n; j++) for (int wm = 0; wm < 1 << reach_buf[j].c.n; wm++)
        keybuf[nk++] = (KeyHit){chain_key_m(&reach_buf[j].c, wm), reach_buf[j].end == w, j};
    qsort(keybuf, nk, sizeof(KeyHit), by_key);
    for (int j = 0; j < nk; j++) {
        if (j && keybuf[j].key == keybuf[j - 1].key) continue;
        int any = 0, at = j; for (int q = j; q < nk && keybuf[q].key == keybuf[j].key; q++) if (keybuf[q].hit) { any = 1; at = keybuf[q].idx; }
        get(K(16, T, keybuf[j].key), 1)->c++;
        if (any) { get(K(7, T, keybuf[j].key), 1)->c++;
            const Chain *c = &reach_buf[at].c;
            for (int q = 0; q < c->n; q++) { get(K(9, c->s[q].rel, 0), 1)->c++; consider(c->s[q].rel); }
            if (qi >= 0 && c->n == 1 && keybuf[j].key == chain_key_m(c, 0)) note_onestep(qi, c->s[0].rel, c->s[0].dir); } }
}

/* ---------- reading: learning from the stream, or frozen for a test ---------- */

#define MAXCLS 4
static int ncls, cls_n[MAXCLS + 1], cls_w[MAXCLS + 1][1024];
static double *lam;
static double prob(double loc, double mem, int prev) { return mem < 0 ? loc : (1 - lam[prev + 1]) * loc + lam[prev + 1] * mem; }
typedef struct { double loc, mem; int prev; } Obs;
typedef struct { long n[MAXCLS + 1], right[MAXCLS + 1]; Obs *obs; long nobs; } Tally;

static void read_doc(const int *w0, const int *mask, int len, int learning, Tally *ta) {
    Store S = {.n = 0, .clock = 0}; int start = 0, a = -1, b = -1;
    for (int t = 0; t < len; t++) {
        int w = w0[t];
        int words[64]; double pm[64]; int m = 0; double mw = -1;
        int k = mask ? mask[t] : 0;
        if (t > start && (k || (ta && ta->obs))) {
            m = mem_answer(&S, w0 + start, t - start, words, pm);
            if (m) { mw = 0; for (int q = 0; q < m; q++) if (words[q] == w) mw = pm[q]; } }
        if (ta && ta->obs && m) ta->obs[ta->nobs++] = (Obs){trigram(a, b, w), mw, b};
        if (ta && k) {            /* the answer, tried before it comes */
            double best = -1; int arg = -1;
            for (int j = 0; j < cls_n[k]; j++) { int c = cls_w[k][j]; double mc = -1;
                if (m) { mc = 0; for (int q = 0; q < m; q++) if (words[q] == c) mc = pm[q]; }
                double pc = prob(trigram(a, b, c), mc, b); if (pc > best) { best = pc; arg = c; } }
            ta->n[k]++; ta->right[k] += arg == w; }
        if (learning) {           /* then it comes, and is taken in */
            if (t > start) { int slot; u64 T = templ(w0 + start, t - start, &slot); if (slot >= 0) take_in(&S, T, slot, w); }
            see_word(a, b, w); }
        if (w == DOT) { absorb(&S, w0 + start, t - start, learning); start = t + 1; a = b = -1; } else { a = b; b = w; }
    }
}
static void fit_lam(const Docs *va) {
    Tally ta = {{0}, {0}, malloc(sizeof(Obs) * (va->tokens + 1)), 0};
    for (int i = 0; i < V + 2; i++) lam[i] = 0.3;
    for (int i = 0; i < va->n; i++) read_doc(va->w[i], NULL, va->len[i], 0, &ta);
    double *num = calloc(V + 2, sizeof(double)), *den = calloc(V + 2, sizeof(double)), g = 0.3;
    for (int it = 0; it < 10; it++) {
        double gn = 0, gd = 0; memset(num, 0, sizeof(double) * (V + 2)); memset(den, 0, sizeof(double) * (V + 2));
        for (long j = 0; j < ta.nobs; j++) { int c = ta.obs[j].prev + 1; double l = lam[c], r = l * ta.obs[j].mem / (l * ta.obs[j].mem + (1 - l) * ta.obs[j].loc);
            num[c] += r; den[c] += 1; gn += r; gd += 1; }
        g = gd ? gn / gd : 0;
        for (int c = 0; c <= V + 1; c++) lam[c] = (num[c] + 3 * g) / (den[c] + 3);
    }
    free(ta.obs); free(num); free(den);
}

static const char *cname[] = {"", "city", "country", "continent", "who-likes"};
static void report(const char *what, const Tally *t) {
    long an = 0, ar = 0; for (int k = 1; k <= ncls; k++) { an += t->n[k]; ar += t->right[k]; }
    printf("%s right %.3f (", what, an ? (double)ar / an : 0);
    for (int k = 1; k <= ncls; k++) printf("%s%s %.3f", k > 1 ? ", " : "", cname[k], t->n[k] ? (double)t->right[k] / t->n[k] : 0);
    printf(") of %ld\n", an); fflush(stdout);
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: newborn_lang DATA_DIR [no_ideas|no_wernicke]\n"); return 1; }
    no_ideas = argc > 2 && !strcmp(argv[2], "no_ideas"); no_wernicke = argc > 2 && !strcmp(argv[2], "no_wernicke");
    char p1[1024], p2[1024]; snprintf(p1, sizeof p1, "%s/meta.txt", argv[1]);
    FILE *mf = fopen(p1, "r"); if (!mf || fscanf(mf, "V %d", &V) != 1) { fprintf(stderr, "no meta\n"); return 1; } fclose(mf);
    snprintf(p1, sizeof p1, "%s/classes.txt", argv[1]); FILE *cf = fopen(p1, "r"); char line[1 << 14];
    while (cf && fgets(line, sizeof line, cf) && ncls < MAXCLS) { ncls++; char *p = line, *e; cls_n[ncls] = 0;
        for (;;) { long x = strtol(p, &e, 10); if (e == p) break; cls_w[ncls][cls_n[ncls]++] = (int)x; p = e; } }
    if (cf) fclose(cf);
    snprintf(p1, sizeof p1, "%s/dot_q.txt", argv[1]); FILE *dq = fopen(p1, "r");
    if (!dq || fscanf(dq, "%d %d", &DOT, &QMARK) != 2) { fprintf(stderr, "no dot_q.txt\n"); return 1; } fclose(dq);
    Docs tr, va;
    snprintf(p1, sizeof p1, "%s/train.txt", argv[1]); snprintf(p2, sizeof p2, "%s/train_mask.txt", argv[1]); load(&tr, p1, p2);
    snprintf(p1, sizeof p1, "%s/valid.txt", argv[1]); load(&va, p1, NULL);
    const char *sets[] = {"50", "2000", "8000", "new_2000", "pas_2000"};
    Docs te[5];
    for (int g = 0; g < 5; g++) { snprintf(p1, sizeof p1, "%s/test_%s.txt", argv[1], sets[g]); snprintf(p2, sizeof p2, "%s/test_%s_mask.txt", argv[1], sets[g]); load(&te[g], p1, p2); }
    hmask = (1u << 25) - 1; H = calloc(hmask + 1, sizeof(Ent));
    uni = calloc(V, sizeof(double)); lam = malloc(sizeof(double) * (V + 2)); for (int i = 0; i < V + 2; i++) lam[i] = 0.3;
    ends = calloc(NSLOT, sizeof *ends);
    if (no_wernicke) printf("control: ideas without Wernicke's frame word\n");
    printf("from 0%s: one pass over %d documents; every answer tried before it is seen\n", no_ideas ? ", no ideas for new words (control)" : "", tr.n);
    const int checks[] = {0, 1, 3, 10, 30, 100, 300, 1000, 1500};
    int ci = 0; Tally run = {{0}, {0}, NULL, 0};
    for (int i = 0; i <= tr.n; i++) {
        if (ci < 9 && i == checks[ci]) {
            rebuild_frames(); if (i) fit_lam(&va);
            printf("CHECK after %d documents\n", i);
            if (i) { char what[64]; snprintf(what, sizeof what, "  first tries, documents %d-%d:", i > 0 ? checks[ci - 1] + 1 : 0, i); report(what, &run); memset(&run, 0, sizeof run); }
            for (int g = 0; g < 5; g++) { Tally t = {{0}, {0}, NULL, 0};
                for (int d = 0; d < te[g].n; d++) read_doc(te[g].w[d], te[g].mask[d], te[g].len[d], 0, &t);
                char what[64]; snprintf(what, sizeof what, "  test %-9s", sets[g]); report(what, &t); }
            ci++; }
        if (i == tr.n) break;
        read_doc(tr.w[i], tr.mask[i], tr.len[i], 1, &run);
        if (i % 10 == 9) rebuild_frames();
    }
    /* what Wernicke learned: how often a wording ending in a word runs reversed */
    snprintf(p1, sizeof p1, "%s/vocab.txt", argv[1]); FILE *vf = fopen(p1, "r");
    if (vf) { char wd[256]; int id = 0;
        printf("Wernicke: chance a wording runs reversed, by its last word (overall %.2f):", p_reversed(-1));
        while (fgets(wd, sizeof wd, vf)) { wd[strcspn(wd, "\n")] = 0;
            if (id < (1 << 16) && fr_same[id] + fr_rev[id] > 0) printf(" %s %.2f", wd, p_reversed(id)); id++; }
        printf("\n"); fclose(vf); }
    return 0;
}
