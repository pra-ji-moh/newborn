/* Hartley Non-Abelian Model: the smallest version that could fail.
 *
 * The world: five slots, each holding one of three colours. The being has K
 * acts. Each act secretly moves the slots by a fixed permutation of five
 * (an element of S5). The acts are drawn so that together they generate all
 * of S5, so order matters: doing A then B is not doing B then A.
 *
 * What the model sees: episodes. A start, a sequence of acts, and the end.
 * Never the steps in between.
 *
 * What the model holds (Hartley, 1928): for each act, the set of
 * permutations it could still be. All 120 to begin with. Nothing is weighed;
 * every survivor counts the same. What it still doesn't know is log2 of
 * what is left. Each episode it remembers rules out every combination that
 * would not carry its start to its end, composed in the order the acts were
 * done. Remembered episodes are re-checked until nothing more falls.
 *
 * The test: held-out sequences of 10, 100 and 1000 acts from fresh starts.
 * It claims an end state only when every surviving possibility agrees.
 * Otherwise it says it doesn't know.
 *
 * The control: the same model with order thrown away, so an episode is a
 * set of acts (composed in a fixed order). That is the limit the grown brain
 * hit (its ideas are sets).
 *
 * No gradients, no training, no frameworks. Plain C.
 *
 * Build: cc -std=c99 -O2 hnam.c -o hnam -lm
 * Run:   ./hnam [worlds] [mode]   mode 0 = short and long episodes,
 *                                 mode 1 = no single steps (lengths 3 to 5)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define N 5          /* slots */
#define NP 120       /* permutations of five */
#define C 3          /* colours */
#define NS 243       /* 3^5 states */
#define K 3          /* acts */
#define MAXEP 400
#define MAXL 8

static int perm[NP][N];       /* perm[p][i] = where slot i's colour goes */
static int comp[NP][NP];      /* comp[a][b] = do a, then b */
static int act_on[NP][NS];    /* state after applying p */

static unsigned long long rng = 88172645463325252ULL;
static unsigned rnd(void) {
    rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
    return (unsigned)(rng >> 11);
}

static void setup(void) {
    int n = 0, a[N];
    for (a[0] = 0; a[0] < N; a[0]++) for (a[1] = 0; a[1] < N; a[1]++)
    for (a[2] = 0; a[2] < N; a[2]++) for (a[3] = 0; a[3] < N; a[3]++)
    for (a[4] = 0; a[4] < N; a[4]++) {
        int used = 0, ok = 1;
        for (int i = 0; i < N; i++) { if (used >> a[i] & 1) ok = 0; used |= 1 << a[i]; }
        if (ok) { memcpy(perm[n], a, sizeof a); n++; }
    }
    for (int p = 0; p < NP; p++) for (int s = 0; s < NS; s++) {
        int c[N], d[N], t = s, r = 0;
        for (int i = 0; i < N; i++) { c[i] = t % C; t /= C; }
        for (int i = 0; i < N; i++) d[perm[p][i]] = c[i];
        for (int i = N - 1; i >= 0; i--) r = r * C + d[i];
        act_on[p][s] = r;
    }
    for (int a1 = 0; a1 < NP; a1++) for (int b = 0; b < NP; b++) {
        int q[N];
        for (int i = 0; i < N; i++) q[i] = perm[b][perm[a1][i]];
        for (int p = 0; p < NP; p++) if (!memcmp(q, perm[p], sizeof q)) { comp[a1][b] = p; break; }
    }
}

static int group_size(const int *g, int k) {
    char in[NP] = {0}; int q[NP], h = 0, t = 0, id = 0;
    for (int p = 0; p < NP; p++) { int ok = 1; for (int i = 0; i < N; i++) if (perm[p][i] != i) ok = 0; if (ok) id = p; }
    in[id] = 1; q[t++] = id;
    while (h < t) { int x = q[h++]; for (int j = 0; j < k; j++) { int y = comp[x][g[j]]; if (!in[y]) { in[y] = 1; q[t++] = y; } } }
    return t;
}

typedef struct { int start, end, len, act[MAXL]; } Ep;

typedef struct {
    char cand[K][NP];
    int ordered;          /* 1 = order kept, 0 = order thrown away */
    int contradiction;
} Model;

static void order_view(const Ep *e, int *seq, int ordered) {
    memcpy(seq, e->act, e->len * sizeof(int));
    if (!ordered) /* a set of acts: fixed order, so order carries nothing */
        for (int i = 1; i < e->len; i++) for (int j = i; j > 0 && seq[j] < seq[j-1]; j--) { int t = seq[j]; seq[j] = seq[j-1]; seq[j-1] = t; }
}

/* Rule out every possibility no surviving combination supports. Returns
 * 1 if something fell, -1 if the episode was too wide to check yet. */
static int eliminate(Model *m, const Ep *e) {
    int seq[MAXL], ids[K], nd = 0, slot[K];
    order_view(e, seq, m->ordered);
    for (int k = 0; k < K; k++) slot[k] = -1;
    for (int i = 0; i < e->len; i++) if (slot[seq[i]] < 0) { slot[seq[i]] = nd; ids[nd++] = seq[i]; }
    int list[K][NP], cnt[K]; double width = 1;
    for (int d = 0; d < nd; d++) {
        cnt[d] = 0;
        for (int p = 0; p < NP; p++) if (m->cand[ids[d]][p]) list[d][cnt[d]++] = p;
        width *= cnt[d];
    }
    if (width > 2e6) return -1;
    char seen[K][NP]; memset(seen, 0, sizeof seen);
    int ix[K] = {0}, any = 0;
    if (width == 0) { m->contradiction = 1; return 0; }
    for (;;) {
        int s = e->start;
        for (int i = 0; i < e->len; i++) s = act_on[list[slot[seq[i]]][ix[slot[seq[i]]]]][s];
        if (s == e->end) { any = 1; for (int d = 0; d < nd; d++) seen[d][list[d][ix[d]]] = 1; }
        int d = 0;
        while (d < nd && ++ix[d] == cnt[d]) { ix[d] = 0; d++; }
        if (d == nd) break;
    }
    if (!any) { m->contradiction = 1; return 0; }
    int fell = 0;
    for (int d = 0; d < nd; d++) for (int p = 0; p < NP; p++)
        if (m->cand[ids[d]][p] && !seen[d][p]) { m->cand[ids[d]][p] = 0; fell = 1; }
    return fell;
}

static void settle(Model *m, Ep *eps, int n) {
    int changed = 1;
    while (changed && !m->contradiction) {
        changed = 0;
        for (int i = 0; i < n && !m->contradiction; i++) if (eliminate(m, &eps[i]) == 1) changed = 1;
    }
}

static double bits(const Model *m) {
    double b = 0;
    for (int k = 0; k < K; k++) { int c = 0; for (int p = 0; p < NP; p++) c += m->cand[k][p]; b += c ? log2(c) : 0; }
    return b;
}

/* The set of ends every surviving possibility allows. Claim only if one. */
static int predict(const Model *m, int start, const int *seq, int len) {
    if (m->contradiction) return -1;
    static char cur[NS], nxt[NS];
    int s2[1000];
    memcpy(s2, seq, len * sizeof(int));
    if (!m->ordered) { Ep e; (void)e; /* sort, as the order-blind model must */
        for (int i = 1; i < len; i++) for (int j = i; j > 0 && s2[j] < s2[j-1]; j--) { int t = s2[j]; s2[j] = s2[j-1]; s2[j-1] = t; } }
    memset(cur, 0, NS); cur[start] = 1;
    for (int i = 0; i < len; i++) {
        memset(nxt, 0, NS);
        for (int s = 0; s < NS; s++) if (cur[s]) for (int p = 0; p < NP; p++) if (m->cand[s2[i]][p]) nxt[act_on[p][s]] = 1;
        memcpy(cur, nxt, NS);
    }
    int found = -1, n = 0;
    for (int s = 0; s < NS; s++) if (cur[s]) { found = s; n++; }
    return n == 1 ? found : -1;
}

static int rand_start(void) {
    for (;;) { int s = rnd() % NS, c[C] = {0}, t = s; for (int i = 0; i < N; i++) { c[t % C]++; t /= C; }
               if (c[0] < N && c[1] < N && c[2] < N) return s; }
}

int main(int argc, char **argv) {
    int worlds = argc > 1 ? atoi(argv[1]) : 30, mode = argc > 2 ? atoi(argv[2]) : 0;
    const int checks[] = {5, 10, 20, 40, 80, 160};
    const int nchk = 6, lens[] = {10, 100, 1000}, nlen = 3, tests = 100;
    setup();
    long claim[2][6][3] = {{{0}}}, right[2][6][3] = {{{0}}}, wrong[2][6][3] = {{{0}}};
    double bsum[2][6] = {{0}}; int contra[2][6] = {{0}}, exact_at[2] = {0}; long exact_sum[2] = {0};
    for (int w = 0; w < worlds; w++) {
        rng = 0x9E3779B97F4A7C15ULL * (w + 1) + (mode + 1) * 7919;
        int g[K];
        do { for (int k = 0; k < K; k++) g[k] = rnd() % NP; } while (group_size(g, K) != NP);
        Ep eps[MAXEP]; int ne = 0;
        Model m[2];
        for (int o = 0; o < 2; o++) { memset(&m[o], 0, sizeof m[o]); memset(m[o].cand, 1, sizeof m[o].cand); m[o].ordered = (o == 0); }
        int found_exact[2] = {-1, -1};
        for (int c = 0; c < nchk; c++) {
            while (ne < checks[c]) {
                Ep *e = &eps[ne];
                if (mode == 0) e->len = ne < 10 ? 1 + rnd() % 2 : 1 + rnd() % 5;
                else e->len = 3 + rnd() % 3;
                e->start = rand_start();
                int s = e->start;
                for (int i = 0; i < e->len; i++) { e->act[i] = rnd() % K; s = act_on[g[e->act[i]]][s]; }
                e->end = s; ne++;
                for (int o = 0; o < 2; o++) {
                    settle(&m[o], eps, ne);
                    if (found_exact[o] < 0 && !m[o].contradiction && bits(&m[o]) == 0) found_exact[o] = ne;
                }
            }
            for (int o = 0; o < 2; o++) {
                bsum[o][c] += bits(&m[o]); contra[o][c] += m[o].contradiction;
                for (int l = 0; l < nlen; l++) for (int t = 0; t < tests; t++) {
                    int seq[1000], st = rand_start(), s = st;
                    for (int i = 0; i < lens[l]; i++) { seq[i] = rnd() % K; s = act_on[g[seq[i]]][s]; }
                    int p = predict(&m[o], st, seq, lens[l]);
                    if (p >= 0) { claim[o][c][l]++; if (p == s) right[o][c][l]++; else wrong[o][c][l]++; }
                }
            }
        }
        for (int o = 0; o < 2; o++) if (found_exact[o] > 0) { exact_at[o]++; exact_sum[o] += found_exact[o]; }
    }
    const char *name[2] = {"ORDERED (Hartley non-abelian)", "ORDER-BLIND (acts as a set)"};
    printf("worlds %d, mode %s, %d held-out sequences per length per checkpoint\n",
           worlds, mode ? "no single steps (lengths 3-5)" : "short then long episodes", tests);
    for (int o = 0; o < 2; o++) {
        printf("\n%s\n  episodes | bits left | broken | len10 right/wrong/claimed | len100 | len1000\n", name[o]);
        for (int c = 0; c < nchk; c++) {
            printf("  %8d | %9.2f | %6d |", checks[c], bsum[o][c] / worlds, contra[o][c]);
            for (int l = 0; l < nlen; l++) printf("  %5ld/%ld/%ld", right[o][c][l], wrong[o][c][l], claim[o][c][l]);
            printf("   (of %d)\n", worlds * tests);
        }
        printf("  every act pinned exactly in %d/%d worlds", exact_at[o], worlds);
        if (exact_at[o]) printf(", after %.1f episodes on average", (double)exact_sum[o] / exact_at[o]);
        printf("\n");
    }
    return 0;
}
