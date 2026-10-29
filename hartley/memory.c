/* Memory for the four-domain model: Shannon decides what to store, Hartley
 * boils it down, recall is a dense associative (Hopfield) memory whose
 * sharpness is the Hartley-Shannon dial. No gradients anywhere.
 *
 * Learned by counting, from the training text:
 *   lift[k][w]  how much more likely w is to come later in the sentence
 *               when k came earlier: log2 of P(w later | k) / P(w later).
 *   info[k]     how much k tells about what comes later (Shannon: the
 *               average lift of what actually follows k).
 * Reading a sentence:
 *   store       Shannon decides: a word is stored if info[k] >= tau.
 *   chain       a stored word whose strongest later word is very likely
 *               (P >= 0.5) also stores that word, imagined (one step), so
 *               "Ann" brings "Paris", and "Paris" can bring "France".
 *   boil down   Hartley rules out: an item whose strongest later word has
 *               just appeared has done its job and is crossed off; when the
 *               memory is over capacity, the item that tells least goes.
 *   recall      each item m suggests P_m(w) proportional to
 *               P(w) * 2^(beta * lift[m][w]); items are weighted by
 *               recency (gamma per word of age). High beta: the strongest
 *               suggestion wins (Hartley); low beta: blended (Shannon).
 *   mix         p(w) = (1 - lam[c]) trigram(w) + lam[c] memory(w), with
 *               lam[c] for each previous word c set by counting (EM on the
 *               validation text) - Shannon decides how much to trust memory.
 * Settings (tau, beta, gamma) are chosen on validation.
 *
 * Prior art: trigger pairs and cache models for language (Rosenfeld 1996;
 * Kuhn and De Mori 1990); dense associative memory (Krotov and Hopfield
 * 2016; Demircigil et al. 2017; Ramsauer et al. 2020).
 *
 * Build: cc -std=c99 -O2 memory.c -o memory -lm
 * Run:   ./memory DATA_DIR TRAIN_FILE [task]   (files from real_prep.py or
 *        gen_struct.py; task also scores the class words and the long test)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef unsigned long long u64;
static int V, END;
typedef struct { int n, *len, **w; long tokens; } Set;

static void load(Set *S, const char *path) {
    FILE *f = fopen(path, "r"); if (!f) { perror(path); exit(1); }
    int cap = 1024; S->n = 0; S->len = malloc(cap * sizeof(int)); S->w = malloc(cap * sizeof(int *)); S->tokens = 0;
    char line[1 << 16];
    while (fgets(line, sizeof line, f)) {
        int buf[4096], n = 0; char *p = line, *e;
        for (;;) { long x = strtol(p, &e, 10); if (e == p) break; buf[n++] = (int)x; p = e; }
        buf[n++] = END;
        if (S->n == cap) { cap *= 2; S->len = realloc(S->len, cap * sizeof(int)); S->w = realloc(S->w, cap * sizeof(int *)); }
        S->w[S->n] = malloc(n * sizeof(int)); memcpy(S->w[S->n], buf, n * sizeof(int)); S->len[S->n++] = n; S->tokens += n;
    }
    fclose(f);
}

/* ---------- counts in one hash: n-grams for the trigram, pairs for lift ---------- */

typedef struct { u64 key; double c; int types; } Ent;
static Ent *H; static unsigned hmask;
static Ent *get(u64 key, int create) {
    for (unsigned h = (unsigned)((key * 0x9E3779B97F4A7C15ULL) >> 38) & hmask;; h = (h + 1) & hmask) {
        if (H[h].key == key) return &H[h];
        if (!H[h].key) { if (!create) return NULL; H[h].key = key; return &H[h]; } }
}
static u64 K(int kind, int a, int b, int c) { return ((u64)kind << 60) | ((u64)(a + 2) << 40) | ((u64)(b + 2) << 20) | (u64)(c + 2); }
static double *uni, *later_p; static long total;

static void count_all(const Set *S) {
    hmask = (1u << 26) - 1; H = calloc(hmask + 1, sizeof(Ent)); total = 0;
    uni = calloc(V + 1, sizeof(double)); later_p = calloc(V + 1, sizeof(double));
    char *seen = calloc(V + 1, 1); int *ws = malloc(sizeof(int) * 8192);
    double later_tot = 0;
    for (int i = 0; i < S->n; i++) {
        int a = -1, b = -1, L = S->len[i];
        for (int t = 0; t < L; t++) { int w = S->w[i][t]; Ent *x;
            x = get(K(3, a, b, w), 1); if (!x->c++) get(K(5, a, b, 0), 1)->types++;
            get(K(5, a, b, 0), 1)->c++;
            x = get(K(2, 0, b, w), 1); if (!x->c++) get(K(4, 0, b, 0), 1)->types++;
            get(K(4, 0, b, 0), 1)->c++;
            uni[w]++; total++; a = b; b = w; }
        /* pairs: k earlier, w later (each distinct pair once per sentence) */
        for (int t = 0; t + 1 < L; t++) {
            int k = S->w[i][t], n = 0;
            for (int u = t + 1; u < L; u++) { int w = S->w[i][u]; if (!seen[w]) { seen[w] = 1; ws[n++] = w; } }
            for (int j = 0; j < n; j++) { get(K(6, 0, k, ws[j]), 1)->c++; get(K(7, 0, k, 0), 1)->c++; later_p[ws[j]]++; later_tot++; seen[ws[j]] = 0; }
        }
    }
    for (int w = 0; w <= V; w++) { uni[w] = (uni[w] + 1) / (total + V + 1); later_p[w] = (later_p[w] + 1) / (later_tot + V + 1); }
    free(seen); free(ws);
}
static double trigram(int a, int b, int w) {
    double p = uni[w]; Ent *c, *x;
    if ((c = get(K(4, 0, b, 0), 0)) && c->c) { x = get(K(2, 0, b, w), 0); double l = c->c / (c->c + c->types); p = l * (x ? x->c : 0) / c->c + (1 - l) * p; }
    if ((c = get(K(5, a, b, 0), 0)) && c->c) { x = get(K(3, a, b, w), 0); double l = c->c / (c->c + c->types); p = l * (x ? x->c : 0) / c->c + (1 - l) * p; }
    return p;
}

/* ---------- lift rows: for each k, the later words it raises ---------- */

typedef struct { int w; float lift; } Lift;
static Lift **row; static int *nrow; static double *info; static int *best_w; static double *best_p;
static void build_lift(void) {
    row = calloc(V + 1, sizeof(Lift *)); nrow = calloc(V + 1, sizeof(int)); int *cap = calloc(V + 1, sizeof(int));
    info = calloc(V + 1, sizeof(double)); best_w = malloc(sizeof(int) * (V + 1)); best_p = calloc(V + 1, sizeof(double));
    /* best_w: the later word k points to most sharply, among those that
     * follow k at least half the time; otherwise the one that follows most */
    int *sharp = malloc(sizeof(int) * (V + 1)); double *sharp_l = calloc(V + 1, sizeof(double)), *top_p = calloc(V + 1, sizeof(double));
    int *top = malloc(sizeof(int) * (V + 1));
    for (int k = 0; k <= V; k++) best_w[k] = sharp[k] = top[k] = -1;
    for (unsigned h = 0; h <= hmask; h++) {
        if (!H[h].key || (int)(H[h].key >> 60) != 6 || H[h].c < 2) continue;
        int k = (int)((H[h].key >> 20) & 0xFFFFF) - 2, w = (int)(H[h].key & 0xFFFFF) - 2;
        double ck = get(K(7, 0, k, 0), 0)->c, p = H[h].c / ck, lf = log2(p / later_p[w]);
        if (lf <= 0) continue;
        if (nrow[k] == cap[k]) { cap[k] = cap[k] ? cap[k] * 2 : 8; row[k] = realloc(row[k], cap[k] * sizeof(Lift)); }
        row[k][nrow[k]++] = (Lift){w, (float)lf};
        info[k] += p * lf;
        if (p >= 0.5 && lf > sharp_l[k]) { sharp_l[k] = lf; sharp[k] = w; }
        if (p > top_p[k]) { top_p[k] = p; top[k] = w; }
    }
    for (int k = 0; k <= V; k++) {
        if (sharp[k] >= 0) { best_w[k] = sharp[k]; best_p[k] = 0.5; }
        else { best_w[k] = top[k]; best_p[k] = top_p[k]; }
    }
    free(cap); free(sharp); free(sharp_l); free(top_p); free(top);
}
static double lift_of(int k, int w) {      /* the same value as row[k], straight from the counts */
    Ent *x = get(K(6, 0, k, w), 0); if (!x || x->c < 2) return 0;
    double lf = log2(x->c / get(K(7, 0, k, 0), 0)->c / later_p[w]);
    return lf > 0 ? lf : 0;
}

/* ---------- reading with memory ---------- */

#define CAP 8
static double tau, beta, gamma_;
typedef struct { int k[CAP * 2]; int age[CAP * 2]; int n; int st[64]; int ns; } Mem;

/* ---------- nesting: a stack of open pairs ----------
 * Shannon proposes pairs: an opener is a word whose sharpest later word c
 * follows it nearly always. Hartley confirms or rules out: the pair must
 * nest in the training text - no c without an opener still open before it,
 * every opener closed by the end - in all but 1% of the sentences that use
 * it (the 1% lets a little noise through). The end of a sentence is not a
 * closer. Confirmed openers go on a stack;
 * the top says, with certainty, which closer must come next. */
static int *closer_of; static char *is_closer; static int npairs;
static void find_pairs(const Set *S) {
    closer_of = malloc(sizeof(int) * (V + 1)); is_closer = calloc(V + 1, 1); npairs = 0;
    for (int k = 0; k <= V; k++) closer_of[k] = -1;
    int *depth = calloc(V + 1, sizeof(int)), *bad = calloc(V + 1, sizeof(int)), *used = calloc(V + 1, sizeof(int)), *occ = calloc(V + 1, sizeof(int)), *follow = calloc(V + 1, sizeof(int));
    char *in = calloc(V + 1, 1);
    for (int i = 0; i < S->n; i++) {
        for (int k = 0; k <= V; k++) depth[k] = 0, in[k] = 0;
        for (int t = 0; t < S->len[i]; t++) { int w = S->w[i][t]; occ[w]++;
            if (best_w[w] >= 0 && best_w[w] != w) { depth[w]++; in[w] = 1; }
            for (int k = 0; k <= V; k++) if (best_w[k] == w && k != w && in[k]) { if (--depth[k] < 0) { bad[k]++; depth[k] = 0; } else follow[k]++; } }
        for (int k = 0; k <= V; k++) if (in[k]) { used[k]++; if (depth[k] != 0) bad[k]++; }
    }
    /* candidates, most used first */
    int *cand = malloc(sizeof(int) * (V + 1)), nc = 0;
    for (int k = 0; k <= V; k++) {
        int c = best_w[k];
        if (c < 0 || c == k || c == END || occ[k] < 20 || follow[k] < 0.9 * occ[k] || bad[k] > 0.01 * used[k]) continue;
        cand[nc++] = k;
    }
    for (int i = 1; i < nc; i++) for (int j = i; j > 0 && occ[cand[j]] > occ[cand[j - 1]]; j--) { int t = cand[j]; cand[j] = cand[j - 1]; cand[j - 1] = t; }
    /* Hartley: real pairs never cross ("( [ ) ]"). A candidate is ruled out
     * if, with the pairs already confirmed, a closer arrives while another
     * pair's opener is still on top, in more than 1% of its sentences. */
    for (int ci = 0; ci < nc; ci++) {
        int k = cand[ci], c = best_w[k], cross = 0, uses = 0;
        if (closer_of[c] >= 0 || is_closer[k] || is_closer[c] || closer_of[k] >= 0) continue;
        closer_of[k] = c; is_closer[c] = 1;
        for (int i = 0; i < S->n; i++) { int st[256], ns = 0, has = 0, crossed = 0;
            for (int t = 0; t < S->len[i]; t++) { int w = S->w[i][t];
                if (w == k) has = 1;
                if (closer_of[w] >= 0) { if (ns < 256) st[ns++] = w; }
                else if (is_closer[w]) { int j = ns - 1; while (j >= 0 && closer_of[st[j]] != w) j--;
                    if (j >= 0) { if (j != ns - 1 && (closer_of[st[ns - 1]] == c || w == c)) crossed = 1; memmove(st + j, st + j + 1, sizeof(int) * (ns - 1 - j)); ns--; } } }
            uses += has; cross += crossed; }
        if (cross > 0.01 * uses) { closer_of[k] = -1; is_closer[c] = 0; } else npairs++;
    }
    free(cand); free(depth); free(bad); free(used); free(occ); free(follow); free(in);
}
static double *zrow;         /* per item: normaliser of P(w) 2^(beta lift) */
static void prepare_z(void) {
    free(zrow); zrow = malloc(sizeof(double) * (V + 1));
    for (int k = 0; k <= V; k++) { double z = 1; for (int j = 0; j < nrow[k]; j++) z += later_p[row[k][j].w] * (pow(2, beta * row[k][j].lift) - 1); zrow[k] = z; }
}
static double mem_prob(const Mem *m, int w) {
    if (!m->n && !m->ns) return -1;
    double items = 0;
    if (m->n) { double num = 0, den = 0;
        for (int i = 0; i < m->n; i++) { double r = pow(gamma_, m->age[i]); int k = m->k[i];
            num += r * later_p[w] * pow(2, beta * lift_of(k, w)) / zrow[k]; den += r; }
        items = num / den; }
    if (!m->ns) return items;
    double top = w == closer_of[m->st[m->ns - 1]];     /* Hartley: this closer, no other */
    return m->n ? 0.5 * items + 0.5 * top : top;
}
static void mem_store(Mem *m, int k, int depth) {
    if (info[k] < tau) return;
    for (int i = 0; i < m->n; i++) if (m->k[i] == k) { m->age[i] = 0; return; }
    m->k[m->n] = k; m->age[m->n] = 0; m->n++;
    if (depth < 1 && best_w[k] >= 0 && best_p[k] >= 0.5) mem_store(m, best_w[k], depth + 1);
    while (m->n > CAP) {      /* Hartley: rule out the item that tells least */
        int worst = 0; for (int i = 1; i < m->n; i++) if (info[m->k[i]] < info[m->k[worst]]) worst = i;
        m->k[worst] = m->k[m->n - 1]; m->age[worst] = m->age[m->n - 1]; m->n--; }
}
static void mem_see(Mem *m, int w) {
    for (int i = 0; i < m->n; i++) m->age[i]++;
    if (closer_of[w] >= 0) { if (m->ns < 64) m->st[m->ns++] = w; return; }
    if (is_closer[w]) {       /* take off the nearest opener this closes */
        for (int j = m->ns - 1; j >= 0; j--) if (closer_of[m->st[j]] == w) { memmove(m->st + j, m->st + j + 1, sizeof(int) * (m->ns - 1 - j)); m->ns--; break; }
        return; }
    for (int i = 0; i < m->n;) {  /* Hartley: an item whose job is done is crossed off */
        if (best_w[m->k[i]] == w) { m->k[i] = m->k[m->n - 1]; m->age[i] = m->age[m->n - 1]; m->n--; } else i++; }
    mem_store(m, w, 0);
}

/* p_loc and p_mem for every word of a set (p_mem < 0: memory empty) */
typedef struct { double *loc, *mem; int *prev; long n; } Trace;
static void trace(const Set *T, Trace *tr) {
    tr->n = T->tokens; tr->loc = malloc(sizeof(double) * tr->n); tr->mem = malloc(sizeof(double) * tr->n); tr->prev = malloc(sizeof(int) * tr->n);
    long j = 0;
    for (int i = 0; i < T->n; i++) { Mem m = {.n = 0, .ns = 0}; int a = -1, b = -1;
        for (int t = 0; t < T->len[i]; t++, j++) { int w = T->w[i][t];
            tr->loc[j] = trigram(a, b, w); tr->mem[j] = mem_prob(&m, w); tr->prev[j] = b;
            mem_see(&m, w); a = b; b = w; } }
}

/* Shannon decides how much to trust memory after each word: EM on lam. */
static double *lam;
static double fit_lam(const Trace *tr) {
    free(lam); lam = malloc(sizeof(double) * (V + 2)); double g = 0.1;
    double *num = calloc(V + 2, sizeof(double)), *den = calloc(V + 2, sizeof(double));
    for (int c = 0; c <= V + 1; c++) lam[c] = g;
    for (int it = 0; it < 8; it++) {
        double gn = 0, gd = 0; memset(num, 0, sizeof(double) * (V + 2)); memset(den, 0, sizeof(double) * (V + 2));
        for (long j = 0; j < tr->n; j++) { if (tr->mem[j] < 0) continue; int c = tr->prev[j] + 1;
            double l = lam[c], r = l * tr->mem[j] / (l * tr->mem[j] + (1 - l) * tr->loc[j]);
            num[c] += r; den[c] += 1; gn += r; gd += 1; }
        g = gd ? gn / gd : 0;
        for (int c = 0; c <= V + 1; c++) lam[c] = (num[c] + 5 * g) / (den[c] + 5);
    }
    free(num); free(den);
    double bits = 0;
    for (long j = 0; j < tr->n; j++) { double p = tr->mem[j] < 0 ? tr->loc[j] : (1 - lam[tr->prev[j] + 1]) * tr->loc[j] + lam[tr->prev[j] + 1] * tr->mem[j]; bits -= log2(p); }
    return bits / tr->n;
}

/* ---------- the class probes (as in real.c) ---------- */

#define MAXCLS 4
static int ncls, cls_n[MAXCLS + 1], cls_w[MAXCLS + 1][256];
static int **load_mask(const char *path, const Set *T) {
    FILE *f = fopen(path, "r"); if (!f) return NULL;
    int **m = malloc(sizeof(int *) * T->n); char line[1 << 16];
    for (int i = 0; i < T->n; i++) { m[i] = calloc(T->len[i], sizeof(int)); if (!fgets(line, sizeof line, f)) break;
        char *p = line, *e; for (int t = 0; t < T->len[i] - 1; t++) { long x = strtol(p, &e, 10); if (e == p) break; m[i][t] = (int)x; p = e; } }
    fclose(f); return m;
}
static void load_classes(const char *dir) {
    char path[1024]; snprintf(path, sizeof path, "%s/classes.txt", dir);
    FILE *f = fopen(path, "r"); ncls = 0; if (!f) return; char line[1 << 14];
    while (fgets(line, sizeof line, f) && ncls < MAXCLS) { ncls++; char *p = line, *e; cls_n[ncls] = 0;
        for (;;) { long x = strtol(p, &e, 10); if (e == p) break; cls_w[ncls][cls_n[ncls]++] = (int)x; p = e; } }
    fclose(f);
}
static double prob(int a, int b, const Mem *m, int w) {
    double loc = trigram(a, b, w), mp = mem_prob(m, w);
    return mp < 0 ? loc : (1 - lam[b + 1]) * loc + lam[b + 1] * mp;
}
static void score(const Set *T, int **mask, double *bits, double *pbits, double *pright) {
    double bt = 0, pb = 0; long n = 0, pn = 0, pr = 0;
    for (int i = 0; i < T->n; i++) { Mem m = {.n = 0, .ns = 0}; int a = -1, b = -1;
        for (int t = 0; t < T->len[i]; t++) { int w = T->w[i][t];
            bt -= log2(prob(a, b, &m, w)); n++;
            if (mask && mask[i][t]) { int k = mask[i][t]; double tot = 0, pw = 0, best = -1; int arg = -1;
                for (int j = 0; j < cls_n[k]; j++) { double p = prob(a, b, &m, cls_w[k][j]); tot += p; if (cls_w[k][j] == w) pw = p; if (p > best) { best = p; arg = cls_w[k][j]; } }
                pb -= log2(pw / tot); pn++; pr += arg == w; }
            mem_see(&m, w); a = b; b = w; } }
    *bits = bt / n; *pbits = pn ? pb / pn : 0; *pright = pn ? (double)pr / pn : 0;
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: memory DATA_DIR TRAIN_FILE [task]\n"); return 1; }
    char path[1024]; snprintf(path, sizeof path, "%s/meta.txt", argv[1]);
    FILE *mf = fopen(path, "r"); if (!mf || fscanf(mf, "V %d", &V) != 1) { fprintf(stderr, "no meta\n"); return 1; } fclose(mf);
    END = V;
    Set tr, va, te, tl; int have_long = 0, task = argc > 3 && !strcmp(argv[3], "task");
    snprintf(path, sizeof path, "%s/%s", argv[1], argv[2]); load(&tr, path);
    snprintf(path, sizeof path, "%s/valid.txt", argv[1]); load(&va, path);
    snprintf(path, sizeof path, "%s/test.txt", argv[1]); load(&te, path);
    int **mt = NULL, **ml = NULL;
    if (task) { load_classes(argv[1]);
        snprintf(path, sizeof path, "%s/test_mask.txt", argv[1]); mt = load_mask(path, &te);
        snprintf(path, sizeof path, "%s/test_long.txt", argv[1]); FILE *ft = fopen(path, "r");
        if (ft) { fclose(ft); load(&tl, path); have_long = 1; snprintf(path, sizeof path, "%s/test_long_mask.txt", argv[1]); ml = load_mask(path, &tl); } }
    count_all(&tr); build_lift(); find_pairs(&tr);
    printf("TRAIN %s: %d sentences, %ld words; nesting pairs confirmed: %d\n", argv[2], tr.n, tr.tokens, npairs);
    const double taus[] = {0.05, 0.3, 1, 3}, betas[] = {0.5, 1, 2, 4}, gammas[] = {1, 0.9};
    double best = 1e300, bt = 0, bb = 0, bg = 0;
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) for (int g = 0; g < 2; g++) {
        tau = taus[i]; beta = betas[j]; gamma_ = gammas[g]; prepare_z();
        Trace t; trace(&va, &t); double v = fit_lam(&t);
        free(t.loc); free(t.mem); free(t.prev);
        if (v < best) { best = v; bt = tau; bb = beta; bg = gamma_; }
    }
    tau = bt; beta = bb; gamma_ = bg; prepare_z();
    Trace t; trace(&va, &t); fit_lam(&t);
    double b0, p0, r0, b1 = 0, p1 = 0, r1 = 0, tb, tp, trr;
    score(&te, mt, &b0, &p0, &r0);
    if (have_long) score(&tl, ml, &b1, &p1, &r1);
    lam[0] = 0; for (int c = 0; c <= V + 1; c++) lam[c] = 0;   /* the trigram alone, for comparison */
    score(&te, mt, &tb, &tp, &trr);
    printf("RESULT trigram bits %.4f perplexity %.1f probe_bits %.4f probe_right %.4f\n", tb, pow(2, tb), tp, trr);
    printf("RESULT memory bits %.4f perplexity %.1f probe_bits %.4f probe_right %.4f long_probe_bits %.4f long_probe_right %.4f tau %g beta %g gamma %g\n",
           b0, pow(2, b0), p0, r0, p1, r1, bt, bb, bg);
    return 0;
}
