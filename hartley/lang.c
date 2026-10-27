/* The four domains on a language: can it learn to predict the next word,
 * the task ChatGPT and Claude are trained on, with no gradients?
 *
 * The author's idea: Hartley (possible or not) and Shannon (weighted), each
 * abelian (order ignored) or non-abelian (order kept), with Shannon as the
 * pivot between them.
 *
 * The languages are made here from hidden generators, so the best anyone
 * could ever do is known exactly (the floor, in bits per word):
 *   L1 grammar  - a random sparse grammar: 10 hidden states, 12 words.
 *   L2 tracking - three acts move four items around (S4, order matters);
 *                 after "?" the next word must say where item 0 is.
 *   L3 counters - the same, but the acts commute (Z6): order carries nothing.
 * Noise: each word is swapped for a random word with probability p.
 *
 * The models (no gradients, nothing trained by backprop):
 *   SHANNON non-abelian: words are matrices, a sentence is their product in
 *     order; the matrices come from counts by one factorisation (spectral
 *     learning of weighted automata: Hsu, Kakade and Zhang 2009; Balle et al.
 *     2014). The number of bands (rank) is chosen on validation data.
 *   HARTLEY non-abelian: the same, on possible-or-not; it predicts every
 *     surviving word equally, nothing weighed.
 *   PIVOT: Shannon measures the noise (rare continuations among
 *     well-seen prefixes). Clean: Hartley rules words out and Shannon weighs
 *     the survivors. Noisy: Shannon alone. It also measures whether order
 *     carries information (the abelian / non-abelian pivot).
 *   TRIGRAM: classic counting with the last two words (a control).
 * The gradient opponents (a transformer and an LSTM) are in lang_nets.py and
 * read the same files this program writes.
 *
 * Build: cc -std=c99 -O2 lang.c -o lang -lm
 * Run:   ./lang LANG NOISE_PERCENT TRAIN_SENTENCES OUTDIR
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAXQ 64
#define MAXV 12          /* words; END is symbol V */
#define MAXSYM (MAXV + 1)
#define MAXLEN 400
#define LONGLEN 120
#define NPRE 400
#define NSUF 400
#define MAXK 64

static unsigned long long rng;
static unsigned rnd(void) { rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17; return (unsigned)(rng >> 11); }
static double unif(void) { return (rnd() + 0.5) / 4294967296.0; }

/* ---------- the hidden languages ---------- */

static int answer_marker = -1;   /* the word before an answer ("?"), or -1 */

typedef struct {
    int Q, V;
    double emit[MAXQ][MAXSYM];
    int next[MAXQ][MAXV];
    double noise;
} Lang;

static int perm4[24][4], comp4[24][24];
static void setup_s4(void) {
    int n = 0;
    for (int a = 0; a < 4; a++) for (int b = 0; b < 4; b++) for (int c = 0; c < 4; c++) for (int d = 0; d < 4; d++)
        if (a != b && a != c && a != d && b != c && b != d && c != d) { perm4[n][0] = a; perm4[n][1] = b; perm4[n][2] = c; perm4[n][3] = d; n++; }
    for (int x = 0; x < 24; x++) for (int y = 0; y < 24; y++) {     /* x then y */
        int q[4]; for (int i = 0; i < 4; i++) q[i] = perm4[y][perm4[x][i]];
        for (int z = 0; z < 24; z++) if (!memcmp(q, perm4[z], sizeof q)) comp4[x][y] = z;
    }
}

static void make_grammar(Lang *L) {
    L->Q = 10; L->V = 12;
    for (;;) {
        memset(L->emit, 0, sizeof L->emit);
        for (int q = 0; q < L->Q; q++) {
            int k = 2 + rnd() % 3; double tot = 0;
            for (int j = 0; j < k;) { int w = rnd() % L->V; if (L->emit[q][w] == 0) { L->emit[q][w] = 0.2 + unif(); tot += L->emit[q][w]; j++; } }
            if (q % 3 == 1) { L->emit[q][L->V] = 0.15 + 0.15 * unif(); }
            double e = L->emit[q][L->V];
            for (int w = 0; w < L->V; w++) L->emit[q][w] *= (1 - e) / tot;
            for (int w = 0; w < L->V; w++) L->next[q][w] = rnd() % L->Q;
        }
        int seen[MAXQ] = {0}, stack[MAXQ], sp = 0; seen[0] = 1; stack[sp++] = 0;
        while (sp) { int q = stack[--sp]; for (int w = 0; w < L->V; w++) if (L->emit[q][w] > 0 && !seen[L->next[q][w]]) { seen[L->next[q][w]] = 1; stack[sp++] = L->next[q][w]; } }
        int all = 1; for (int q = 0; q < L->Q; q++) all &= seen[q];
        if (all) return;
    }
}

/* Words: 0-2 acts, 3 "?", then the answers. State = (arrangement, phase). */
static void make_tracking(Lang *L, int abelian) {
    int ng = abelian ? 6 : 24, nans = abelian ? 6 : 4;
    L->V = 4 + nans; L->Q = 2 * ng;
    memset(L->emit, 0, sizeof L->emit);
    for (int q = 0; q < MAXQ; q++) for (int w = 0; w < MAXV; w++) L->next[q][w] = q;   /* never said: stays */
    int g[3];
    if (abelian) { g[0] = 3; g[1] = 2; g[2] = 0; }           /* Z6: add 3, add 2, do nothing */
    else for (;;) {                                          /* three acts that generate S4 */
        for (int k = 0; k < 3; k++) g[k] = rnd() % 24;
        char in[24] = {0}; int qu[24], h = 0, t = 0; in[0] = 1; qu[t++] = 0;   /* perm4[0] is the identity */
        while (h < t) { int x = qu[h++]; for (int k = 0; k < 3; k++) { int y = comp4[x][g[k]]; if (!in[y]) { in[y] = 1; qu[t++] = y; } } }
        if (t == 24 && comp4[g[0]][g[1]] != comp4[g[1]][g[0]]) break;
    }
    for (int s = 0; s < ng; s++) {
        int q0 = 2 * s, q1 = 2 * s + 1;
        for (int k = 0; k < 3; k++) { L->emit[q0][k] = 0.29; L->next[q0][k] = 2 * (abelian ? (s + g[k]) % 6 : comp4[s][g[k]]); }
        L->emit[q0][3] = 0.08; L->next[q0][3] = q1;
        L->emit[q0][L->V] = 0.05;
        int ans = abelian ? s : perm4[s][0];                 /* where item 0 is */
        L->emit[q1][4 + ans] = 1; L->next[q1][4 + ans] = q0;
    }
}

static int sample_word(const Lang *L, int q, int no_end) {
    double tot = 0; for (int w = 0; w < L->V + !no_end; w++) tot += L->emit[q][w];
    double r = unif() * tot;
    for (int w = 0; w < L->V + !no_end; w++) { r -= L->emit[q][w]; if (r <= 0 && L->emit[q][w] > 0) return w; }
    for (int w = L->V - 1 + !no_end; w >= 0; w--) if (L->emit[q][w] > 0) return w;
    return 0;
}

/* One sentence; len_fixed > 0 means no END, exactly that many words. */
static int sentence(const Lang *L, int *out, int len_fixed) {
    for (int tries = 0;; tries++) {
        int q = 0, n = 0;
        for (;;) {
            if (len_fixed && n == len_fixed) return n;
            int w = sample_word(L, q, len_fixed > 0);
            if (w == L->V) { out[n++] = w; return n; }
            int o = unif() < L->noise ? (int)(rnd() % L->V) : w;
            out[n++] = o; q = L->next[q][w];
            if (n >= 60 && !len_fixed) break;
        }
    }
}

/* The floor: bits the true generator itself needs, noise included. */
static double floor_abits;
static double floor_bits(const Lang *L, const int *s, int n, int no_end) {
    double b[MAXQ] = {0}, nb[MAXQ]; b[0] = 1; double bits = 0;
    for (int t = 0; t < n; t++) {
        int o = s[t]; double po = 0; memset(nb, 0, sizeof nb);
        for (int q = 0; q < L->Q; q++) if (b[q] > 0) {
            double tot = 0; for (int w = 0; w < L->V + !no_end; w++) tot += L->emit[q][w];
            for (int w = 0; w < L->V + !no_end; w++) if (L->emit[q][w] > 0) {
                double e = L->emit[q][w] / tot, ch;
                if (w == L->V) ch = (o == L->V); else ch = o == L->V ? 0 : (1 - L->noise) * (o == w) + L->noise / L->V;
                double m = b[q] * e * ch; if (m <= 0) continue;
                po += m; if (w < L->V) nb[L->next[q][w]] += m;
            }
        }
        bits -= log2(po);
        if (t && s[t - 1] == answer_marker) floor_abits -= log2(po);
        for (int q = 0; q < L->Q; q++) b[q] = nb[q] / po;
    }
    return bits;
}

/* ---------- data ---------- */

typedef struct { int n, *len, **w; long tokens; } Set;
static void make_set(const Lang *L, Set *S, int n, int len_fixed) {
    S->n = n; S->len = malloc(n * sizeof(int)); S->w = malloc(n * sizeof(int *)); S->tokens = 0;
    int buf[MAXLEN];
    for (int i = 0; i < n; i++) { S->len[i] = sentence(L, buf, len_fixed); S->w[i] = malloc(S->len[i] * sizeof(int)); memcpy(S->w[i], buf, S->len[i] * sizeof(int)); S->tokens += S->len[i]; }
}
static void write_set(const Set *S, const char *dir, const char *name) {
    char path[512]; snprintf(path, sizeof path, "%s/%s.txt", dir, name);
    FILE *f = fopen(path, "w");
    for (int i = 0; i < S->n; i++) { for (int t = 0; t < S->len[i]; t++) fprintf(f, t ? " %d" : "%d", S->w[i][t]); fprintf(f, "\n"); }
    fclose(f);
}

/* ---------- counts: a tree of every sentence beginning ---------- */

static int (*child)[MAXSYM]; static long *cnt; static int nnodes, cap_nodes;
static int new_node(void) {
    if (nnodes == cap_nodes) { cap_nodes = cap_nodes ? cap_nodes * 2 : 1 << 16; child = realloc(child, cap_nodes * sizeof *child); cnt = realloc(cnt, cap_nodes * sizeof *cnt); }
    memset(child[nnodes], -1, sizeof child[0]); cnt[nnodes] = 0; return nnodes++;
}
static void build_tree(const Set *S) {
    nnodes = 0; new_node();
    for (int i = 0; i < S->n; i++) { int v = 0; cnt[0]++;
        for (int t = 0; t < S->len[i]; t++) { int w = S->w[i][t]; if (child[v][w] < 0) { int c = new_node(); child[v][w] = c; } v = child[v][w]; cnt[v]++; } }
}
static long count_of(int node, const int *s, int n) {
    for (int i = 0; i < n && node >= 0; i++) node = child[node][s[i]];
    return node < 0 ? 0 : cnt[node];
}

/* ---------- the basis: prefixes and suffixes ---------- */

typedef struct { int len, s[8]; long c; int node; } Str;
static Str pre[NPRE * 8], suf[NSUF * 8]; static int npre, nsuf;
static int by_count(const void *a, const void *b) { long x = ((const Str *)a)->c, y = ((const Str *)b)->c; return (y > x) - (y < x); }

static int Lp = 4, Ls = 4;
static void collect_prefixes(int node, int depth, int *path, int nsym) {
    if (npre < NPRE * 8) { Str *p = &pre[npre++]; p->len = depth; memcpy(p->s, path, depth * sizeof(int)); p->c = cnt[node]; p->node = node; }
    if (depth == Lp) return;
    for (int w = 0; w < nsym; w++) if (child[node][w] >= 0 && cnt[child[node][w]] >= 2) { path[depth] = w; collect_prefixes(child[node][w], depth + 1, path, nsym); }
}
/* Suffixes: continuations seen after the basis prefixes, most frequent first. */
#define HS (1 << 18)
static unsigned long long hkey[HS]; static long hval[HS]; static Str hstr[HS]; static int hused[HS], nh;
static void add_suffix(const int *s, int len, long c) {
    unsigned long long k = len; for (int i = 0; i < len; i++) k = k * 17 + s[i] + 1;
    unsigned h = (unsigned)(k * 0x9E3779B97F4A7C15ULL >> 46) & (HS - 1);
    while (hused[h] && hkey[h] != k) h = (h + 1) & (HS - 1);
    if (!hused[h]) { if (nh >= HS / 2) return; hused[h] = 1; hkey[h] = k; hval[h] = 0; hstr[h].len = len; memcpy(hstr[h].s, s, len * sizeof(int)); nh++; }
    hval[h] += c;
}
static void walk_suffixes(int node, int depth, int *path, int nsym) {
    if (depth) add_suffix(path, depth, cnt[node]);
    if (depth == Ls) return;
    for (int w = 0; w < nsym; w++) if (child[node][w] >= 0) { path[depth] = w; walk_suffixes(child[node][w], depth + 1, path, nsym); }
}
static void choose_basis(int nsym) {
    int path[16]; npre = 0; collect_prefixes(0, 0, path, nsym);
    qsort(pre, npre, sizeof(Str), by_count); if (npre > NPRE) npre = NPRE;
    memset(hused, 0, sizeof hused); nh = 0;
    for (int i = 0; i < npre; i++) walk_suffixes(pre[i].node, 0, path, nsym);
    nsuf = 0; suf[nsuf].len = 0; suf[nsuf].c = 1L << 60; nsuf++;     /* the empty suffix */
    for (int h = 0; h < HS; h++) if (hused[h] && nsuf < NSUF * 8) { suf[nsuf] = hstr[h]; suf[nsuf].c = hval[h]; nsuf++; }
    qsort(suf, nsuf, sizeof(Str), by_count); if (nsuf > NSUF) nsuf = NSUF;
}

/* ---------- linear algebra, from scratch ---------- */

static void jacobi_eigen(double *a, int n, double *vec, double *val) {     /* symmetric a, n x n */
    for (int i = 0; i < n; i++) for (int j = 0; j < n; j++) vec[i * n + j] = i == j;
    for (int sweep = 0; sweep < 30; sweep++) {
        double off = 0; for (int i = 0; i < n; i++) for (int j = i + 1; j < n; j++) off += a[i * n + j] * a[i * n + j];
        if (off < 1e-22) break;
        for (int p = 0; p < n; p++) for (int q = p + 1; q < n; q++) {
            double apq = a[p * n + q]; if (fabs(apq) < 1e-15) continue;
            double th = (a[q * n + q] - a[p * n + p]) / (2 * apq);
            double t = (th >= 0 ? 1 : -1) / (fabs(th) + sqrt(th * th + 1)), c = 1 / sqrt(t * t + 1), s = t * c;
            for (int k = 0; k < n; k++) { double x = a[k * n + p], y = a[k * n + q]; a[k * n + p] = c * x - s * y; a[k * n + q] = s * x + c * y; }
            for (int k = 0; k < n; k++) { double x = a[p * n + k], y = a[q * n + k]; a[p * n + k] = c * x - s * y; a[q * n + k] = s * x + c * y; }
            for (int k = 0; k < n; k++) { double x = vec[k * n + p], y = vec[k * n + q]; vec[k * n + p] = c * x - s * y; vec[k * n + q] = s * x + c * y; }
        }
    }
    for (int i = 0; i < n; i++) val[i] = a[i * n + i];
}
static int invert(double *m, int n) {                                      /* in place, Gauss-Jordan */
    double *a = malloc(sizeof(double) * n * 2 * n);
    for (int i = 0; i < n; i++) for (int j = 0; j < 2 * n; j++) a[i * 2 * n + j] = j < n ? m[i * n + j] : (j - n == i);
    for (int c = 0; c < n; c++) {
        int piv = c; for (int r = c + 1; r < n; r++) if (fabs(a[r * 2 * n + c]) > fabs(a[piv * 2 * n + c])) piv = r;
        if (fabs(a[piv * 2 * n + c]) < 1e-300) { free(a); return 0; }
        for (int j = 0; j < 2 * n; j++) { double t = a[c * 2 * n + j]; a[c * 2 * n + j] = a[piv * 2 * n + j]; a[piv * 2 * n + j] = t; }
        double d = a[c * 2 * n + c]; for (int j = 0; j < 2 * n; j++) a[c * 2 * n + j] /= d;
        for (int r = 0; r < n; r++) if (r != c) { double f = a[r * 2 * n + c]; if (f != 0) for (int j = 0; j < 2 * n; j++) a[r * 2 * n + j] -= f * a[c * 2 * n + j]; }
    }
    for (int i = 0; i < n; i++) for (int j = 0; j < n; j++) m[i * n + j] = a[i * 2 * n + n + j];
    free(a); return 1;
}

/* ---------- a weighted automaton: words as k x k matrices ---------- */

typedef struct { int k, nsym; double a0[MAXK], ainf[MAXK], A[MAXSYM][MAXK][MAXK]; } WA;

/* value(prefix, word, suffix) from counts: Shannon uses frequencies,
 * Hartley uses possible (1) or not (0). */
static double hval_of(const int *p, int pl, int w, const Str *s, int hartley, long N) {
    int buf[24], n = 0;
    for (int i = 0; i < pl; i++) buf[n++] = p[i];
    if (w >= 0) buf[n++] = w;
    for (int i = 0; i < s->len; i++) buf[n++] = s->s[i];
    long c = count_of(0, buf, n);
    return hartley ? (c > 0) : (double)c / N;
}

static double *Vtop; static double *eigval; static int vt_n;
static void factor(int hartley, long N) {
    int P = npre, S = nsuf;
    double *H = malloc(sizeof(double) * P * S), *C = malloc(sizeof(double) * S * S);
    for (int i = 0; i < P; i++) for (int j = 0; j < S; j++) H[i * S + j] = hval_of(pre[i].s, pre[i].len, -1, &suf[j], hartley, N);
    for (int a = 0; a < S; a++) for (int b = a; b < S; b++) { double t = 0; for (int i = 0; i < P; i++) t += H[i * S + a] * H[i * S + b]; C[a * S + b] = C[b * S + a] = t; }
    free(Vtop); free(eigval); Vtop = malloc(sizeof(double) * S * S); eigval = malloc(sizeof(double) * S); vt_n = S;
    jacobi_eigen(C, S, Vtop, eigval);
    /* sort columns by eigenvalue, largest first */
    for (int i = 0; i < S; i++) { int m = i; for (int j = i + 1; j < S; j++) if (eigval[j] > eigval[m]) m = j;
        if (m != i) { double t = eigval[i]; eigval[i] = eigval[m]; eigval[m] = t; for (int r = 0; r < S; r++) { t = Vtop[r * S + i]; Vtop[r * S + i] = Vtop[r * S + m]; Vtop[r * S + m] = t; } } }
    free(H); free(C);
}

static void build_wa(WA *M, int k, int nsym, int hartley, long N) {
    int P = npre, S = nsuf;
    if (k > S) k = S; if (k > MAXK) k = MAXK;
    M->k = k; M->nsym = nsym;
    double *F = malloc(sizeof(double) * P * k), *Hs = malloc(sizeof(double) * P * S), *G = malloc(sizeof(double) * k * k), *Fp = malloc(sizeof(double) * k * P);
    for (int i = 0; i < P; i++) for (int j = 0; j < S; j++) Hs[i * S + j] = hval_of(pre[i].s, pre[i].len, -1, &suf[j], hartley, N);
    for (int i = 0; i < P; i++) for (int c = 0; c < k; c++) { double t = 0; for (int j = 0; j < S; j++) t += Hs[i * S + j] * Vtop[j * S + c]; F[i * k + c] = t; }
    for (int a = 0; a < k; a++) for (int b = 0; b < k; b++) { double t = 0; for (int i = 0; i < P; i++) t += F[i * k + a] * F[i * k + b]; G[a * k + b] = t + (a == b) * 1e-12; }
    invert(G, k);
    for (int a = 0; a < k; a++) for (int i = 0; i < P; i++) { double t = 0; for (int b = 0; b < k; b++) t += G[a * k + b] * F[i * k + b]; Fp[a * P + i] = t; }
    int e0 = -1, se = -1;
    for (int i = 0; i < P; i++) if (pre[i].len == 0) e0 = i;
    for (int j = 0; j < S; j++) if (suf[j].len == 0) se = j;
    for (int c = 0; c < k; c++) { double t = 0; for (int j = 0; j < S; j++) t += Hs[e0 * S + j] * Vtop[j * S + c]; M->a0[c] = t; }
    for (int a = 0; a < k; a++) { double t = 0; for (int i = 0; i < P; i++) t += Fp[a * P + i] * Hs[i * S + se]; M->ainf[a] = t; }
    for (int w = 0; w < nsym; w++) {
        for (int i = 0; i < P; i++) for (int j = 0; j < S; j++) Hs[i * S + j] = hval_of(pre[i].s, pre[i].len, w, &suf[j], hartley, N);
        double *HV = malloc(sizeof(double) * P * k);
        for (int i = 0; i < P; i++) for (int c = 0; c < k; c++) { double t = 0; for (int j = 0; j < S; j++) t += Hs[i * S + j] * Vtop[j * S + c]; HV[i * k + c] = t; }
        for (int a = 0; a < k; a++) for (int c = 0; c < k; c++) { double t = 0; for (int i = 0; i < P; i++) t += Fp[a * P + i] * HV[i * k + c]; M->A[w][a][c] = t; }
        free(HV);
    }
    free(F); free(Hs); free(G); free(Fp);
}

/* ---------- predicting the next word ---------- */

enum { SHANNON, HARTLEY, PIVOT };
typedef struct { double bits, abits; long words, awords, excluded_truth; } Score;

/* The word after the state: Shannon weighs, Hartley keeps survivors equal,
 * pivot lets Hartley rule out and Shannon weigh what is left. */
static void next_probs(const WA *S, const double *ss, const WA *H, const double *hs, int kind, int no_end, int V, double *p) {
    int nsym = V + 1; double g[MAXSYM], h[MAXSYM], tot = 0;
    for (int w = 0; w < nsym; w++) {
        if (kind != HARTLEY) { double t = 0; for (int a = 0; a < S->k; a++) { double r = 0; for (int b = 0; b < S->k; b++) r += S->A[w][a][b] * S->ainf[b]; t += ss[a] * r; } g[w] = t > 0 ? t : 0; }
        if (kind != SHANNON) { double t = 0; for (int a = 0; a < H->k; a++) { double r = 0; for (int b = 0; b < H->k; b++) r += H->A[w][a][b] * H->ainf[b]; t += hs[a] * r; } h[w] = t; }
    }
    if (no_end) { g[V] = 0; h[V] = 0; }
    int surv = 0; for (int w = 0; w < nsym; w++) if (kind != SHANNON && h[w] > 0.5) surv++;
    for (int w = 0; w < nsym; w++) {
        if (kind == SHANNON) p[w] = g[w];
        else if (kind == HARTLEY) p[w] = surv ? (h[w] > 0.5) : 1;
        else p[w] = surv ? g[w] * (h[w] > 0.5) : g[w];
        if (no_end && w == V) p[w] = 0;
        tot += p[w];
    }
    if (tot <= 0) { for (int w = 0; w < nsym; w++) p[w] = (no_end && w == V) ? 0 : 1; tot = nsym - no_end; }
    /* a little mass everywhere, so a ruled-out word that happens costs bits, not infinity */
    const double eps = 1e-3; double z = 0;
    for (int w = 0; w < nsym; w++) { p[w] = (no_end && w == V) ? 0 : (1 - eps) * p[w] / tot + eps / (nsym - no_end); z += p[w]; }
    for (int w = 0; w < nsym; w++) p[w] /= z;
}

static void step(const WA *M, double *s, int w) {
    double t[MAXK], mass = 0, norm = 0;
    for (int b = 0; b < M->k; b++) { double x = 0; for (int a = 0; a < M->k; a++) x += s[a] * M->A[w][a][b]; t[b] = x; }
    /* keep the state the size of a probability: divide by its total mass */
    for (int b = 0; b < M->k; b++) { mass += t[b] * M->ainf[b]; norm += t[b] * t[b]; }
    double d = fabs(mass) > 1e-12 ? mass : sqrt(norm) + 1e-30;
    for (int b = 0; b < M->k; b++) s[b] = t[b] / d;
}

static Score score_wa(const WA *S, const WA *H, int kind, const Set *T, int no_end, int V) {
    Score r = {0, 0, 0, 0, 0}; double p[MAXSYM], ss[MAXK], hs[MAXK];
    for (int i = 0; i < T->n; i++) {
        if (kind != HARTLEY) memcpy(ss, S->a0, sizeof ss);
        if (kind != SHANNON) memcpy(hs, H->a0, sizeof hs);
        for (int t = 0; t < T->len[i]; t++) {
            int w = T->w[i][t];
            next_probs(S, ss, H, hs, kind, no_end, V, p);
            r.bits -= log2(p[w]); r.words++;
            if (t && T->w[i][t - 1] == answer_marker) { r.abits -= log2(p[w]); r.awords++; }
            if (kind != SHANNON && p[w] < 0.01) r.excluded_truth++;
            if (kind != HARTLEY) step(S, ss, w);
            if (kind != SHANNON) step(H, hs, w);
        }
    }
    return r;
}

/* ---------- the trigram control ---------- */

static Score score_trigram(const Set *Tr, const Set *T, int no_end, int V) {
    int ns = V + 2;               /* + a start symbol */
    double *c3 = calloc((size_t)ns * ns * ns, sizeof(double)), *c2 = calloc(ns * ns, sizeof(double)), *c1 = calloc(ns, sizeof(double));
    for (int i = 0; i < Tr->n; i++) { int a = V + 1, b = V + 1;
        for (int t = 0; t < Tr->len[i]; t++) { int w = Tr->w[i][t]; c3[(a * ns + b) * ns + w]++; c2[b * ns + w]++; c1[w]++; a = b; b = w; } }
    Score r = {0, 0, 0, 0, 0};
    for (int i = 0; i < T->n; i++) { int a = V + 1, b = V + 1;
        for (int t = 0; t < T->len[i]; t++) {
            int w = T->w[i][t]; double p[MAXSYM], tot1 = 0, tot2 = 0, tot3 = 0; int d2 = 0, d3 = 0;
            for (int x = 0; x <= V; x++) if (!(no_end && x == V)) { tot1 += c1[x]; tot2 += c2[b * ns + x]; tot3 += c3[(a * ns + b) * ns + x]; d2 += c2[b * ns + x] > 0; d3 += c3[(a * ns + b) * ns + x] > 0; }
            double z = 0;
            for (int x = 0; x <= V; x++) {    /* Witten-Bell interpolation */
                if (no_end && x == V) { p[x] = 0; continue; }
                double p1 = (c1[x] + 1) / (tot1 + V + 1 - no_end);
                double l2 = tot2 / (tot2 + d2 + 1e-9), p2 = tot2 > 0 ? l2 * c2[b * ns + x] / tot2 + (1 - l2) * p1 : p1;
                double l3 = tot3 / (tot3 + d3 + 1e-9), p3 = tot3 > 0 ? l3 * c3[(a * ns + b) * ns + x] / tot3 + (1 - l3) * p2 : p2;
                p[x] = p3; z += p3;
            }
            r.bits -= log2(p[w] / z); r.words++;
            if (t && T->w[i][t - 1] == answer_marker) { r.abits -= log2(p[w] / z); r.awords++; }
            a = b; b = w;
        }
    }
    free(c3); free(c2); free(c1);
    return r;
}

/* ---------- the pivots, measured by Shannon ---------- */

/* Noise: share of continuations, after prefixes seen at least 100 times,
 * that are rarer than 2%. A clean grammar has none; noise makes a tail.
 * Also the gap between Hartley's and Shannon's uncertainty there. */
static void noise_pivot(int nsym, double *tail, double *gap, int *judged) {
    double rare = 0, all = 0, g = 0; int n = 0;
    for (int i = 0; i < npre; i++) {
        long c = pre[i].c; if (c < 100) continue;
        int node = pre[i].node; double hs = 0; int support = 0; long tot = 0;
        for (int w = 0; w < nsym; w++) if (child[node][w] >= 0) tot += cnt[child[node][w]];
        if (!tot) continue;
        for (int w = 0; w < nsym; w++) if (child[node][w] >= 0) { double q = (double)cnt[child[node][w]] / tot;
            support++; hs -= q * log2(q); all += cnt[child[node][w]]; if (q < 0.02) rare += cnt[child[node][w]]; }
        g += log2(support) - hs; n++;
    }
    *judged = n; *tail = all > 0 ? rare / all : 0; *gap = n ? g / n : 0;
}

/* Order: for prefixes ending "a b" and "b a" (same beginning), do the next
 * words differ more than chance? A G-test per pair; the share of pairs
 * significant at 1% (chance alone gives about 1%), and the information
 * order carries, in bits per word. */
static double chi2_sf(double x, int df) {               /* upper tail, by series */
    if (x <= 0) return 1; double a = df / 2.0, xx = x / 2, sum = 1, term = 1;
    for (int n = 1; n < 500; n++) { term *= xx / (a + n); sum += term; if (term < 1e-12 * sum) break; }
    double lp = -xx + a * log(xx) - lgamma(a + 1) + log(sum);
    return 1 - exp(lp);
}
static void order_pivot(int nsym, double *share_sig, double *bits, int *pairs) {
    int sig = 0, n = 0; double info = 0, wsum = 0, *c1 = malloc(sizeof(double) * MAXSYM * MAXSYM), *c2 = malloc(sizeof(double) * MAXSYM * MAXSYM);
    int nc = nsym * nsym;     /* continuations: the next two words (or the end) */
    for (int i = 0; i < npre; i++) {
        if (pre[i].len < 2) continue;
        const int *s = pre[i].s; int L = pre[i].len, a = s[L - 2], b = s[L - 1];
        if (a >= b) continue;
        int other[8]; memcpy(other, s, L * sizeof(int)); other[L - 2] = b; other[L - 1] = a;
        int n1 = pre[i].node, n2 = 0; for (int t = 0; t < L && n2 >= 0; t++) n2 = child[n2][other[t]];
        if (n2 < 0 || cnt[n1] < 30 || cnt[n2] < 30) continue;
        double t1 = 0, t2 = 0;
        for (int x = 0; x < nc; x++) c1[x] = c2[x] = 0;
        for (int nd = 0; nd < 2; nd++) { int node = nd ? n2 : n1; double *c = nd ? c2 : c1;
            for (int w = 0; w < nsym; w++) { int m = child[node][w]; if (m < 0) continue;
                long below = 0;
                for (int u = 0; u < nsym; u++) if (child[m][u] >= 0) { c[w * nsym + u] += cnt[child[m][u]]; below += cnt[child[m][u]]; }
                if (cnt[m] > below) c[w * nsym + (nsym - 1)] += cnt[m] - below; } }
        for (int x = 0; x < nc; x++) { t1 += c1[x]; t2 += c2[x]; }
        if (t1 < 30 || t2 < 30) continue;
        double G = 0, mi = 0; int df = -1;
        for (int x = 0; x < nc; x++) { double tw = c1[x] + c2[x]; if (tw == 0) continue; df++;
            double e1 = tw * t1 / (t1 + t2), e2 = tw * t2 / (t1 + t2);
            if (c1[x] > 0) { G += 2 * c1[x] * log(c1[x] / e1); mi += c1[x] * log2(c1[x] / e1); }
            if (c2[x] > 0) { G += 2 * c2[x] * log(c2[x] / e2); mi += c2[x] * log2(c2[x] / e2); } }
        if (df < 1) continue;
        n++; if (chi2_sf(G, df) < 0.01) sig++;
        info += mi; wsum += t1 + t2;
    }
    free(c1); free(c2);
    *pairs = n; *share_sig = n ? (double)sig / n : 0; *bits = wsum > 0 ? info / wsum : 0;
}

/* ---------- main ---------- */

int main(int argc, char **argv) {
    if (argc < 5) { fprintf(stderr, "usage: lang LANG(1-3) NOISE_PERCENT TRAIN_SENTENCES OUTDIR\n"); return 1; }
    int lang = atoi(argv[1]), ntrain = atoi(argv[3]); double noise = atof(argv[2]) / 100; const char *dir = argv[4];
    setup_s4();
    Lang L; memset(&L, 0, sizeof L);
    rng = 0x2545F4914F6CDD1DULL * lang;                 /* the language depends only on LANG */
    if (lang == 1) make_grammar(&L); else make_tracking(&L, lang == 3);
    L.noise = noise;
    if (lang >= 2) answer_marker = 3;
    rng = 0x9E3779B97F4A7C15ULL * (lang * 1000 + (int)(noise * 100) + 7) ^ (unsigned long long)ntrain * 2654435761ULL;
    int nval = ntrain / 5 < 50 ? 50 : ntrain / 5;
    Set tr, va, te, lo;
    make_set(&L, &tr, ntrain, 0); make_set(&L, &va, nval, 0); make_set(&L, &te, 2000, 0); make_set(&L, &lo, 200, LONGLEN);
    write_set(&tr, dir, "train"); write_set(&va, dir, "val"); write_set(&te, dir, "test"); write_set(&lo, dir, "long");
    int V = L.V, nsym = V + 1;
    double fte = 0, flo = 0, fate, falo; long ate = 0, alo = 0;
    floor_abits = 0; for (int i = 0; i < te.n; i++) { fte += floor_bits(&L, te.w[i], te.len[i], 0); for (int t = 1; t < te.len[i]; t++) ate += te.w[i][t - 1] == answer_marker; }
    fate = floor_abits;
    floor_abits = 0; for (int i = 0; i < lo.n; i++) { flo += floor_bits(&L, lo.w[i], lo.len[i], 1); for (int t = 1; t < lo.len[i]; t++) alo += lo.w[i][t - 1] == answer_marker; }
    falo = floor_abits;
    double fat = ate ? fate / ate : 0, fal = alo ? falo / alo : 0;
    char path[512]; snprintf(path, sizeof path, "%s/meta.txt", dir);
    FILE *mf = fopen(path, "w"); fprintf(mf, "V %d\nfloor_test %.6f\nfloor_long %.6f\nfloor_ans_test %.6f\nfloor_ans_long %.6f\nanswer_marker %d\n", V, fte / te.tokens, flo / lo.tokens, fat, fal, answer_marker); fclose(mf);

    build_tree(&tr);
    choose_basis(nsym);
    double tail, gap, sig, obits; int judged, pairs;
    noise_pivot(nsym, &tail, &gap, &judged);
    order_pivot(nsym, &sig, &obits, &pairs);
    int noisy = judged == 0 || tail > 0.005;      /* can't judge: assume noise, stay Shannon */
    int ordered = pairs == 0 || sig > 0.05;

    /* Shannon and Hartley machines; bands chosen on the validation set. */
    static WA S, H, bestS, bestH; const int ks[] = {2, 4, 6, 8, 12, 16, 24, 32, 40, 48, 64};
    double bestv = 1e300, besthv = 1e300;
    factor(0, tr.n);
    for (int i = 0; i < 11; i++) { build_wa(&S, ks[i], nsym, 0, tr.n); Score v = score_wa(&S, &S, SHANNON, &va, 0, V);
        if (v.bits < bestv) { bestv = v.bits; bestS = S; } }
    factor(1, tr.n);
    for (int i = 0; i < 11; i++) { build_wa(&H, ks[i], nsym, 1, tr.n); Score v = score_wa(&H, &H, HARTLEY, &va, 0, V);
        if (v.bits < besthv) { besthv = v.bits; bestH = H; } }

    Score r[4][2];
    for (int lg = 0; lg < 2; lg++) {
        const Set *T = lg ? &lo : &te;
        r[0][lg] = score_wa(&bestS, &bestH, SHANNON, T, lg, V);
        r[1][lg] = score_wa(&bestS, &bestH, HARTLEY, T, lg, V);
        r[2][lg] = score_wa(&bestS, &bestH, noisy ? SHANNON : PIVOT, T, lg, V);
        r[3][lg] = score_trigram(&tr, T, lg, V);
    }
    const char *lname[] = {"", "grammar", "tracking (S4)", "counters (Z6, abelian)"};
    printf("LANG %s, noise %.0f%%, %d training sentences (%ld words)\n", lname[lang], noise * 100, ntrain, tr.tokens);
    printf("pivot: rare-continuation share %.4f over %d prefixes, Hartley-Shannon gap %.3f bits -> %s\n", tail, judged, gap, noisy ? "SHANNON" : "HARTLEY rules out, Shannon weighs");
    printf("pivot: order changes the next word in %.1f%% of %d pairs, %.4f bits/word -> %s\n", 100 * sig, pairs, obits, ordered ? "NON-ABELIAN" : "ABELIAN");
    printf("bands: Shannon %d, Hartley %d\n", bestS.k, bestH.k);
    printf("RESULT model excess_test excess_long answer_excess_test answer_excess_long words_ruled_out_wrongly\n");
    const char *mn[] = {"shannon_nonabelian", "hartley_nonabelian", "four_domain_pivot", "trigram"};
    double ft = fte / te.tokens, fl = flo / lo.tokens;
    for (int m = 0; m < 4; m++)
        printf("RESULT %s %.4f %.4f %.4f %.4f %ld\n", mn[m], r[m][0].bits / r[m][0].words - ft, r[m][1].bits / r[m][1].words - fl,
               r[m][0].awords ? r[m][0].abits / r[m][0].awords - fat : 0, r[m][1].awords ? r[m][1].abits / r[m][1].awords - fal : 0, r[m][0].excluded_truth + r[m][1].excluded_truth);
    printf("floor (bits/word): test %.4f, long %.4f; answers: test %.4f, long %.4f\n", ft, fl, fat, fal);
    return 0;
}
