/* Long text through relations: divide, Hartley cuts, non-abelian relations
 * valued by Shannon, chains followed in order. No gradients.
 *
 * Reading a document (a stream of sentences, cut at "."):
 *   divide       each sentence is a piece.
 *   Hartley cut  keep only the surprising words of a piece (Shannon's
 *                surprise, -log2 P(w), at least theta), and the words that
 *                turned out to be answers in a first pass with a lower cut
 *                (8.5 bits) - answers to question-like templates (ones
 *                answered at least half the time), at least half the times
 *                they follow one. Common but important words, such as the
 *                names of the four continents, stay; common filler goes.
 *   relations    every ordered pair of kept words x ... y in a piece is a
 *                relation x -> y, named by the common words between them
 *                ("lives in", "is in", "likes"). Order kept: x -> y is not
 *                y -> x (non-abelian).
 *   value        Shannon: each relation's value is its evidence (times seen)
 *                times how useful its kind of relation has been for
 *                answering, learned by counting on the training documents. A
 *                kind never seen in training, between two answer words, gets
 *                what relations between answers have had in general.
 *   priority     memory holds at most CAP relations; when full, Hartley
 *                rules out the lowest-valued one. Cost per word does not
 *                grow with the length of the text.
 * Asking: the common words of the sentence so far, with the kept words as
 * slots, form a template ("_ city ?", "who likes _ ?"). From the training
 * documents the model counts which chain of relations, from the slot's word,
 * reaches the word that comes next: one step or two, forward or backward
 * ("lives in" then "is in" for a country; "likes" backward for "who likes").
 * Following a chain is composing relations in order. At test the chain's
 * answers, weighted by Shannon (how often that chain answered the template
 * times how often it was right when present, and the evidence along it), are
 * mixed with a trigram. A relation in wording never seen in training counts
 * as "some relation, this way round" (direction kept, wording ignored); how much to trust
 * them after each word is set by counting (EM) on the validation documents.
 *
 * Build: cc -std=c99 -O2 longtext.c -o longtext -lm
 * Run:   ./longtext DATA_DIR [control]   (files from gen_long.py)
 *        control: trigram (no relations), abelian (direction ignored),
 *                 oldest (forget the oldest relation, not the least valued)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef unsigned long long u64;
static int V, DOT, QMARK;
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
static u64 K(int kind, u64 a, u64 b) {      /* the kind in the top 6 bits, a hash of (a, b) below */
    u64 h = a * 0x100000001B3ULL ^ (b + 0x9E3779B97F4A7C15ULL) * 0xC2B2AE3D27D4EB4FULL; h ^= h >> 31;
    return ((u64)kind << 58) | (h & ((1ULL << 58) - 1)) | 1; }
static double *uni; static long total;

/* trigram over the stream, restarting at each "." */
static void count_trigram(const Docs *D) {
    uni = calloc(V, sizeof(double)); total = 0;
    for (int i = 0; i < D->n; i++) { int a = -1, b = -1;
        for (int t = 0; t < D->len[i]; t++) { int w = D->w[i][t]; Ent *x;
            x = get(K(3, (u64)(a + 2) << 20 | (u64)(b + 2), w), 1); if (!x->c++) get(K(5, (u64)(a + 2) << 20 | (u64)(b + 2), 0), 1)->types++;
            get(K(5, (u64)(a + 2) << 20 | (u64)(b + 2), 0), 1)->c++;
            x = get(K(2, b + 2, w), 1); if (!x->c++) get(K(4, b + 2, 0), 1)->types++;
            get(K(4, b + 2, 0), 1)->c++;
            uni[w]++; total++;
            if (w == DOT) a = b = -1; else { a = b; b = w; } } }
    for (int w = 0; w < V; w++) uni[w] = (uni[w] + 1) / (total + V);
}
static double trigram(int a, int b, int w) {
    double p = uni[w]; Ent *c, *x;
    if ((c = get(K(4, b + 2, 0), 0)) && c->c) { x = get(K(2, b + 2, w), 0); double l = c->c / (c->c + c->types); p = l * (x ? x->c : 0) / c->c + (1 - l) * p; }
    if ((c = get(K(5, (u64)(a + 2) << 20 | (u64)(b + 2), 0), 0)) && c->c) { x = get(K(3, (u64)(a + 2) << 20 | (u64)(b + 2), w), 0); double l = c->c / (c->c + c->types); p = l * (x ? x->c : 0) / c->c + (1 - l) * p; }
    return p;
}

/* ---------- relations in memory ---------- */

#define CAP 64
static double theta;                       /* Hartley cut: surprise in bits */
static int order_blind, oldest_out;       /* controls: direction ignored; forget the oldest, not the least valued */
typedef struct { int x, y; u64 rel; double count, value; long seen; } Rel;
typedef struct { Rel r[CAP + 1]; int n; long clock; } Store;
static char *useful;                       /* words that have turned out to be answers */
static int kept(int w) { return w != DOT && (-log2(uni[w]) >= theta || (useful && useful[w])); }
/* How useful a kind of relation has been for answering. A kind never seen
 * in training, between two words that are answers, gets what relations
 * between answers have had in general (K 17 useful, K 18 stored). */
static double usefulness(u64 rel, int x, int y) {
    Ent *s = get(K(10, rel, 0), 0), *u = get(K(9, rel, 0), 0);
    if (!s && useful && useful[x] && useful[y]) { Ent *a = get(K(17, 0, 0), 0), *b = get(K(18, 0, 0), 0);
        return ((a ? a->c : 0) + 0.01) / ((b ? b->c : 0) + 1); }
    return ((u ? u->c : 0) + 0.01) / ((s ? s->c : 0) + 1); }
static void store_rel(Store *S, int x, int y, u64 rel) {
    S->clock++;
    for (int i = 0; i < S->n; i++) if (S->r[i].x == x && S->r[i].y == y && S->r[i].rel == rel) {
        S->r[i].count++; S->r[i].seen = S->clock; S->r[i].value = S->r[i].count * usefulness(rel, x, y); return; }
    S->r[S->n++] = (Rel){x, y, rel, 1, usefulness(rel, x, y), S->clock};
    if (S->n > CAP) {               /* Hartley: rule out the least valuable, oldest first among equals */
        int worst = 0;
        for (int i = 1; i < S->n; i++)
            if (oldest_out ? S->r[i].seen < S->r[worst].seen
                           : (S->r[i].value < S->r[worst].value || (S->r[i].value == S->r[worst].value && S->r[i].seen < S->r[worst].seen))) worst = i;
        S->r[worst] = S->r[--S->n]; }
}
/* A finished piece: relations between its kept words, named by the common
 * words between them (at most 5). */
static void absorb(Store *S, const int *s, int n, int learning) {
    int kp[64], nk = 0;
    for (int t = 0; t < n && nk < 64; t++) if (kept(s[t])) kp[nk++] = t;
    for (int i = 0; i < nk; i++) for (int j = i + 1; j < nk; j++) {
        int a = kp[i], b = kp[j]; if (b - a - 1 > 5) continue;
        u64 rel = 7; for (int t = a + 1; t < b; t++) rel = rel * 1000003ULL + (u64)s[t] + 1;
        if (learning) { get(K(10, rel, 0), 1)->c++; if (useful && useful[s[a]] && useful[s[b]]) get(K(18, 0, 0), 1)->c++; }
        store_rel(S, s[a], s[b], rel);
    }
}

/* ---------- chains: one or two relations, forward or backward ---------- */

typedef struct { u64 rel; int dir, idx; } Step;   /* idx: which stored relation (not part of the key) */
#define MAXSTEP 3
typedef struct { Step s[MAXSTEP]; int n; } Chain;
static u64 chain_key_m(const Chain *c, int wild) {   /* wild: steps whose wording is ignored, direction kept */
    u64 k = c->n + 0x51ULL * (u64)(wild != 0);
    for (int i = 0; i < c->n; i++) k = k * 0x9E3779B97F4A7C15ULL + (wild >> i & 1 ? 0 : c->s[i].rel) * 2 + (u64)c->s[i].dir;
    return k; }
/* every chain from x (one to three relations, each forward or backward),
 * with where it ends and its weight */
typedef struct { Chain c; int end; double w; } Reach;
static int reach_n, reach_max; static Reach *reach_out;
static void extend(const Store *S, int at, int origin, Chain *c, double w, char *used) {
    for (int i = 0; i < S->n && reach_n < reach_max; i++) for (int d = 0; d < 2 && reach_n < reach_max; d++) {
        const Rel *r = &S->r[i]; int from = d ? r->y : r->x, to = d ? r->x : r->y;
        if (used[i] || from != at || to == origin) continue;
        c->s[c->n] = (Step){r->rel, order_blind ? 0 : d, i}; c->n++;
        reach_out[reach_n++] = (Reach){*c, to, w * r->count};
        if (c->n < MAXSTEP) { used[i] = 1; extend(S, to, origin, c, w * r->count, used); used[i] = 0; }
        c->n--;
    }
}
static int reach_all(const Store *S, int x, Reach *out, int max) {
    char used[CAP + 1] = {0}; Chain c = {.n = 0};
    reach_n = 0; reach_max = max; reach_out = out;
    extend(S, x, x, &c, 1, used);
    return reach_n;
}

/* the template of the sentence so far: common words, kept words as a slot */
static u64 templ(const int *s, int n, int *slot) {
    u64 k = 99; *slot = -1;
    for (int t = (n > 6 ? n - 6 : 0); t < n; t++) {
        if (kept(s[t])) { k = k * 1000003ULL + 1; *slot = s[t]; } else k = k * 1000003ULL + (u64)s[t] + 2; }
    return k;
}

/* Learning chains: for every kept word that comes next, count which chains
 * from the slot word reach it; and how often each template is seen. */
#define MAXR 32768
static Reach reach_buf[MAXR];
static int count_useful;     /* second pass: count which words question-like templates answer */
typedef struct { u64 key; int hit, idx; } KeyHit;
static KeyHit keybuf[MAXR * 8];
static int by_key(const void *a, const void *b) { u64 x = ((const KeyHit *)a)->key, y = ((const KeyHit *)b)->key; return (x > y) - (x < y); }
static void learn(const Docs *D) {
    for (int i = 0; i < D->n; i++) {
        Store S = {.n = 0, .clock = 0}; int start = 0;
        for (int t = 0; t < D->len[i]; t++) {
            int w = D->w[i][t];
            if (t > start) { int slot; u64 T = templ(D->w[i] + start, t - start, &slot);
                if (slot >= 0) {
                    get(K(6, T, 0), 1)->c++;
                    if (kept(w)) { int n = reach_all(&S, slot, reach_buf, MAXR), hit = 0;
                        for (int j = 0; j < n && !hit; j++) hit = reach_buf[j].end == w;
                        if (hit) get(K(15, T, 0), 1)->c++;            /* this template got answered */
                        if (count_useful) { Ent *a = get(K(15, T, 0), 0), *o = get(K(6, T, 0), 0);
                            if (a && o && o->c >= 50 && a->c >= 0.5 * o->c) {   /* a question-like template */
                                get(K(14, w, 0), 1)->c++; if (hit) get(K(13, w, 0), 1)->c++; } }
                        /* each chain present once: was it present (K 16), did it hit (K 7) */
                        /* every chain, and every version of it with some wordings ignored */
                        int nk = 0;
                        for (int j = 0; j < n; j++) for (int wm = 0; wm < 1 << reach_buf[j].c.n; wm++)
                            keybuf[nk++] = (KeyHit){chain_key_m(&reach_buf[j].c, wm), reach_buf[j].end == w, j};
                        qsort(keybuf, nk, sizeof(KeyHit), by_key);
                        n = nk;
                        for (int j = 0; j < n; j++) {
                            if (j && keybuf[j].key == keybuf[j - 1].key) continue;
                            int any = 0, at = j; for (int q = j; q < n && keybuf[q].key == keybuf[j].key; q++) if (keybuf[q].hit) { any = 1; at = keybuf[q].idx; }
                            get(K(16, T, keybuf[j].key), 1)->c++;
                            if (any) { get(K(7, T, keybuf[j].key), 1)->c++;
                                for (int q = 0; q < reach_buf[at].c.n; q++) { get(K(9, reach_buf[at].c.s[q].rel, 0), 1)->c++;
                                    const Rel *r = &S.r[reach_buf[at].c.s[q].idx]; if (useful && useful[r->x] && useful[r->y]) get(K(17, 0, 0), 1)->c++; } } } } } }
            if (w == DOT) { absorb(&S, D->w[i] + start, t - start, 1); start = t + 1; }
        }
    }
}

/* What memory says the next word is, as a distribution over a few words. */
/* Answers reached by the chains that answered this template before,
 * weighted by Shannon: how often each chain was right when it was present
 * (its precision), times the evidence along the chain. Updates ("P moved to C") need no rule of their own: the
 * counts learn that a "moved to" chain is the stronger evidence. */
static int mem_answer(const Store *S, const int *s, int n, int *words, double *p) {
    int slot; u64 T = templ(s, n, &slot); if (slot < 0) return 0;
    Ent *tt = get(K(6, T, 0), 0); if (!tt) return 0;
    int nr = reach_all(S, slot, reach_buf, MAXR), m = 0; double tot = 0;
    for (int j = 0; j < nr; j++) {
        /* a relation in wording never seen in training counts as "some
         * relation, this way round": its chain is judged by the version
         * with that wording ignored */
        int wild = 0; for (int q = 0; q < reach_buf[j].c.n; q++) if (!get(K(10, reach_buf[j].c.s[q].rel, 0), 0)) wild |= 1 << q;
        u64 ck = chain_key_m(&reach_buf[j].c, wild);
        Ent *c = get(K(7, T, ck), 0); if (!c) continue;
        Ent *av = get(K(16, T, ck), 0);
        double w = c->c * c->c / ((av ? av->c : 0) + 1) / tt->c * reach_buf[j].w; int e = reach_buf[j].end, k;   /* times right, times precision */
        for (k = 0; k < m; k++) if (words[k] == e) break;
        if (k == m) { if (m == 64) continue; words[m] = e; p[m] = 0; m++; }
        p[k] += w; tot += w; }
    if (tot <= 0) return 0;
    for (int k = 0; k < m; k++) p[k] /= tot;
    return m;
}

/* ---------- reading and scoring ---------- */

#define MAXCLS 4
static int cls_n[MAXCLS + 1], cls_w[MAXCLS + 1][1024];
static double *lam;                 /* trust in memory after each word */
typedef struct { double loc, mem; int prev; } Obs;

static double prob(double loc, double mem, int prev) { return mem < 0 ? loc : (1 - lam[prev + 1]) * loc + lam[prev + 1] * mem; }

/* Walk a document set. obs (if given) collects loc/mem pairs for EM;
 * probes score masked answers within their class. */
static void walk(const Docs *D, Obs *obs, long *nobs, double *bits, long *nb, double *pbits, long *pn, long *pright) {   /* pn, pright: per class */
    for (int i = 0; i < D->n; i++) {
        Store S = {.n = 0, .clock = 0}; int start = 0, a = -1, b = -1;
        for (int t = 0; t < D->len[i]; t++) {
            int w = D->w[i][t], words[64]; double pm[64]; int m = t > start ? mem_answer(&S, D->w[i] + start, t - start, words, pm) : 0;
            double mw = -1; if (m) { mw = 0; for (int k = 0; k < m; k++) if (words[k] == w) mw = pm[k]; }
            double loc = trigram(a, b, w);
            if (obs && m) { obs[*nobs] = (Obs){loc, mw, b}; (*nobs)++; }
            if (bits) { *bits -= log2(prob(loc, mw, b)); (*nb)++; }
            int k = D->mask[i][t];
            if (pbits && k) { double tot = 0, pw = 0, best = -1; int arg = -1;
                for (int j = 0; j < cls_n[k]; j++) { int c = cls_w[k][j]; double mc = -1;
                    if (m) { mc = 0; for (int q = 0; q < m; q++) if (words[q] == c) mc = pm[q]; }
                    double pc = prob(trigram(a, b, c), mc, b); tot += pc; if (c == w) pw = pc; if (pc > best) { best = pc; arg = c; } }
                *pbits -= log2(pw / tot); pn[k]++; pright[k] += arg == w; }
            if (w == DOT) { absorb(&S, D->w[i] + start, t - start, 0); start = t + 1; a = b = -1; } else { a = b; b = w; }
        }
    }
}
static void fit_lam(const Docs *va) {
    lam = malloc(sizeof(double) * (V + 2)); for (int c = 0; c <= V + 1; c++) lam[c] = 0.3;
    Obs *o = malloc(sizeof(Obs) * (va->tokens + 1)); long n = 0;
    walk(va, o, &n, NULL, NULL, NULL, NULL, NULL);
    double *num = calloc(V + 2, sizeof(double)), *den = calloc(V + 2, sizeof(double)), g = 0.3;
    for (int it = 0; it < 10; it++) {
        double gn = 0, gd = 0; memset(num, 0, sizeof(double) * (V + 2)); memset(den, 0, sizeof(double) * (V + 2));
        for (long j = 0; j < n; j++) { int c = o[j].prev + 1; double l = lam[c], r = l * o[j].mem / (l * o[j].mem + (1 - l) * o[j].loc);
            num[c] += r; den[c] += 1; gn += r; gd += 1; }
        g = gd ? gn / gd : 0;
        for (int c = 0; c <= V + 1; c++) lam[c] = (num[c] + 3 * g) / (den[c] + 3);
    }
    free(o); free(num); free(den);
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: longtext DATA_DIR [trigram|abelian|oldest]\n"); return 1; }
    const char *ctl = argc > 2 ? argv[2] : "";
    order_blind = !strcmp(ctl, "abelian"); oldest_out = !strcmp(ctl, "oldest");
    char p1[1024], p2[1024]; snprintf(p1, sizeof p1, "%s/meta.txt", argv[1]);
    FILE *mf = fopen(p1, "r"); if (!mf || fscanf(mf, "V %d", &V) != 1) { fprintf(stderr, "no meta\n"); return 1; } fclose(mf);
    snprintf(p1, sizeof p1, "%s/classes.txt", argv[1]); FILE *cf = fopen(p1, "r"); char line[1 << 14]; int nc = 0;
    while (cf && fgets(line, sizeof line, cf) && nc < MAXCLS) { nc++; char *p = line, *e; cls_n[nc] = 0;
        for (;;) { long x = strtol(p, &e, 10); if (e == p) break; cls_w[nc][cls_n[nc]++] = (int)x; p = e; } }
    if (cf) fclose(cf);
    Docs tr, va;
    snprintf(p1, sizeof p1, "%s/train.txt", argv[1]); load(&tr, p1, NULL);
    snprintf(p1, sizeof p1, "%s/valid.txt", argv[1]); load(&va, p1, NULL);
    /* the word numbers of "." and "?": the most frequent word ending a
     * sentence piece is "."; "?" is read from the vocabulary order below */
    snprintf(p1, sizeof p1, "%s/dot_q.txt", argv[1]); FILE *dq = fopen(p1, "r");
    if (!dq || fscanf(dq, "%d %d", &DOT, &QMARK) != 2) { fprintf(stderr, "no dot_q.txt\n"); return 1; } fclose(dq);
    hmask = (1u << 25) - 1; H = calloc(hmask + 1, sizeof(Ent));
    count_trigram(&tr);
    /* first pass, with a lower cut: see which words get answered */
    theta = 8.5; learn(&tr);                 /* which templates get answered */
    count_useful = 1; learn(&tr); count_useful = 0;   /* which words they answer */
    useful = calloc(V, 1); int nuseful = 0;
    for (int w = 0; w < V; w++) { Ent *h = get(K(13, w, 0), 0), *o = get(K(14, w, 0), 0);
        if (h && o && h->c >= 5 && h->c >= 0.5 * o->c) { useful[w] = 1; nuseful++; } }
    for (unsigned h = 0; h <= hmask; h++) { int kind = (int)(H[h].key >> 58); if (kind >= 6) H[h].c = 0; }
    const double thetas[] = {7, 8.5, 9.5, 11};
    double best = 1e300, bt = 8.5;
    for (int i = 0; i < 4; i++) {
        theta = thetas[i];
        /* fresh chain counts for this cut */
        for (unsigned h = 0; h <= hmask; h++) { int kind = (int)(H[h].key >> 58); if (kind >= 6) { H[h].c = 0; } }
        learn(&tr); fit_lam(&va);
        double bits = 0; long nb = 0; walk(&va, NULL, NULL, &bits, &nb, NULL, NULL, NULL);
        if (bits / nb < best) { best = bits / nb; bt = theta; }
    }
    theta = bt;
    for (unsigned h = 0; h <= hmask; h++) { int kind = (int)(H[h].key >> 58); if (kind >= 6) H[h].c = 0; }
    learn(&tr); fit_lam(&va);
    if (!strcmp(ctl, "trigram")) for (int c = 0; c <= V + 1; c++) lam[c] = 0;
    printf("control %s; theta %g (Hartley cut, bits of surprise); %d words kept as answers; memory %d relations\n", *ctl ? ctl : "none", theta, nuseful, CAP);
    const char *sets[] = {"50", "500", "2000", "8000", "new_50", "new_2000"};
    const char *cname[] = {"", "city", "country", "who-likes"};
    const char *cname4[] = {"", "city", "country", "continent", "who-likes"};
    for (int g = 0; g < 6; g++) {
        Docs te; snprintf(p1, sizeof p1, "%s/test_%s.txt", argv[1], sets[g]); snprintf(p2, sizeof p2, "%s/test_%s_mask.txt", argv[1], sets[g]);
        FILE *ex = fopen(p1, "r"); if (!ex) continue; fclose(ex);
        load(&te, p1, p2);
        double bits = 0, pb = 0; long nb = 0, pn[MAXCLS + 1] = {0}, pr[MAXCLS + 1] = {0};
        walk(&te, NULL, NULL, &bits, &nb, &pb, pn, pr);
        long an = 0, ar = 0; for (int k = 1; k <= nc; k++) { an += pn[k]; ar += pr[k]; }
        printf("RESULT relations set %s right %.3f (", sets[g], (double)ar / an);
        for (int k = 1; k <= nc; k++) printf("%s%s %.3f", k > 1 ? ", " : "", nc == 4 ? cname4[k] : cname[k], pn[k] ? (double)pr[k] / pn[k] : 0);
        printf(") answer_bits %.3f\n", pb / an);
        fflush(stdout);
    }
    return 0;
}
