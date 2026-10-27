/* Both Hartleys at once: 1928 elimination done on the first band of the 1942
 * transform, so the model scales where listing possibilities can't.
 *
 * hnam.c lists every permutation an act could be: 120 for five slots. For
 * twelve slots that is 479,001,600 per act, and the combinations an episode
 * must check multiply. transform.c shows that the set one observation leaves
 * is held exactly by the first band of the Hartley transform: a slot matrix,
 * "slot i's contents can go to slot j", yes or no. This model keeps only
 * that matrix for each act and eliminates on it:
 *
 *   - An episode's colours flow through the acts in the order they were
 *     done. A move i -> j for an act falls if, at any step the act was done,
 *     no colour that can reach slot i could still end where it was seen.
 *   - Every slot goes somewhere and no two go to the same place, so a move
 *     falls if it is in no complete assignment of the matrix (Regin's rule:
 *     a perfect matching, then alternating cycles).
 *   - Remembered episodes are re-checked until nothing more falls.
 *
 * Nothing is weighed. Elimination is sound: the truth never falls, so every
 * claim it makes is right. It is not complete: the band can keep moves that
 * the full set of possibilities would rule out, so it may need more episodes
 * than hnam.c. That cost is measured, not assumed.
 *
 * Build: cc -std=c99 -O2 band.c -o band -lm
 * Run:   ./band slots colours worlds mode suppose   (slots <= 64, colours <= 16)
 *        mode 0 = short then long episodes, mode 1 = lengths 3 to 5 only
 *        suppose 1 = also test single moves by supposing them
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAXN 64
#define K 3
#define MAXEP 1280
#define MAXL 8
#define TESTLEN 1000

typedef unsigned long long u64;

static int n, ncol;
static unsigned long long rng;
static unsigned rnd(void) {
    rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
    return (unsigned)(rng >> 11);
}

typedef struct { unsigned char start[MAXN], end[MAXN]; int len, act[MAXL]; } Ep;

typedef struct {
    u64 move[K][MAXN];      /* move[k][i]: the slots act k may send slot i to */
    int contradiction;
} Model;

static int popc(u64 x) { return __builtin_popcountll(x); }

/* Colours flow forward and back through the episode; a move survives a step
 * only if a colour can reach its source and still end correctly from its
 * target. */
static int flow(Model *m, const Ep *e) {
    unsigned short fwd[MAXL + 1][MAXN], bwd[MAXL + 1][MAXN];
    for (int i = 0; i < n; i++) { fwd[0][i] = 1u << e->start[i]; bwd[e->len][i] = 1u << e->end[i]; }
    for (int t = 1; t <= e->len; t++) {
        for (int j = 0; j < n; j++) fwd[t][j] = 0;
        for (int i = 0; i < n; i++) { u64 r = m->move[e->act[t - 1]][i];
            while (r) { int j = __builtin_ctzll(r); r &= r - 1; fwd[t][j] |= fwd[t - 1][i]; } }
    }
    for (int t = e->len; t >= 1; t--)
        for (int i = 0; i < n; i++) { u64 r = m->move[e->act[t - 1]][i]; unsigned short b = 0;
            while (r) { int j = __builtin_ctzll(r); r &= r - 1; b |= bwd[t][j]; }
            bwd[t - 1][i] = b; }
    int fell = 0;
    for (int t = 1; t <= e->len; t++) {
        int k = e->act[t - 1];
        for (int i = 0; i < n; i++) { u64 r = m->move[k][i], keep = r;
            while (r) { int j = __builtin_ctzll(r); r &= r - 1; if (!(fwd[t - 1][i] & bwd[t][j])) keep &= ~(1ULL << j); }
            if (keep != m->move[k][i]) { m->move[k][i] = keep; fell = 1; if (!keep) m->contradiction = 1; } }
    }
    return fell;
}

static int match_col[MAXN], match_row[MAXN], seen_col[MAXN], stamp;
static int augment(const u64 *mv, int i) {
    u64 r = mv[i];
    while (r) { int j = __builtin_ctzll(r); r &= r - 1;
        if (seen_col[j] == stamp) continue;
        seen_col[j] = stamp;
        if (match_col[j] < 0 || augment(mv, match_col[j])) { match_col[j] = i; match_row[i] = j; return 1; } }
    return 0;
}

/* Keep only moves that sit in some complete assignment. */
static int assign(Model *m, int k) {
    u64 *mv = m->move[k];
    for (int j = 0; j < n; j++) match_col[j] = -1;
    for (int i = 0; i < n; i++) { stamp++; if (!augment(mv, i)) { m->contradiction = 1; return 0; } }
    /* Nodes 0..n-1 rows, n..2n-1 columns. Row i -> its matched column;
     * column j -> row i for every other allowed move i -> j. */
    u64 reach[2 * MAXN][2];
    for (int v = 0; v < 2 * n; v++) reach[v][0] = reach[v][1] = 0;
    for (int i = 0; i < n; i++) {
        int j = match_row[i], c = n + j;
        reach[i][c >> 6] |= 1ULL << (c & 63);
        u64 r = mv[i] & ~(1ULL << j);
        while (r) { int jj = __builtin_ctzll(r); r &= r - 1; reach[n + jj][i >> 6] |= 1ULL << (i & 63); }
    }
    for (int w = 0; w < 2 * n; w++) for (int v = 0; v < 2 * n; v++)
        if (reach[v][w >> 6] >> (w & 63) & 1) { reach[v][0] |= reach[w][0]; reach[v][1] |= reach[w][1]; }
    int fell = 0;
    for (int i = 0; i < n; i++) {
        u64 r = mv[i], keep = r;
        while (r) { int j = __builtin_ctzll(r); r &= r - 1;
            if (j == match_row[i]) continue;
            int c = n + j;     /* on an alternating cycle: column j reaches row i and back */
            if (!((reach[c][i >> 6] >> (i & 63) & 1) && (reach[i][c >> 6] >> (c & 63) & 1))) keep &= ~(1ULL << j); }
        if (keep != mv[i]) { mv[i] = keep; fell = 1; }
    }
    return fell;
}

static void settle(Model *m, Ep *eps, int ne) {
    int changed = 1;
    while (changed && !m->contradiction) {
        changed = 0;
        for (int e = 0; e < ne && !m->contradiction; e++) changed |= flow(m, &eps[e]);
        for (int k = 0; k < K && !m->contradiction; k++) changed |= assign(m, k);
    }
}

/* Suppose: hold one move as if certain and settle. If the evidence then
 * contradicts itself, that move is ruled out. Still elimination, still
 * nothing weighed: a possibility falls only when it cannot be true. */
static int probe_depth;
static int suppose(Model *m, Ep *eps, int ne) {
    int fell = 0;
    for (int k = 0; k < K && !m->contradiction; k++)
        for (int i = 0; i < n && !m->contradiction; i++) {
            u64 r = m->move[k][i];
            if (popc(r) < 2) continue;
            while (r) { int j = __builtin_ctzll(r); r &= r - 1;
                static Model t; t = *m;
                t.move[k][i] = 1ULL << j;
                settle(&t, eps, ne);
                if (t.contradiction) { m->move[k][i] &= ~(1ULL << j); fell = 1; settle(m, eps, ne); } }
        }
    return fell;
}

static void think(Model *m, Ep *eps, int ne) {
    settle(m, eps, ne);
    if (probe_depth) while (!m->contradiction && suppose(m, eps, ne)) ;
}

static int open_moves(const Model *m) {
    int s = 0;
    for (int k = 0; k < K; k++) for (int i = 0; i < n; i++) s += popc(m->move[k][i]) - 1;
    return s;
}

/* Upper bound on Hartley's bits left: log2 of the moves each slot still has. */
static double bits_bound(const Model *m) {
    double b = 0;
    for (int k = 0; k < K; k++) for (int i = 0; i < n; i++) b += log2(popc(m->move[k][i]));
    return b;
}

/* Claim the end only if every slot's colour is certain. */
static int predict(const Model *m, const unsigned char *start, const int *seq, int len, unsigned char *out) {
    unsigned short cur[MAXN], nxt[MAXN];
    for (int i = 0; i < n; i++) cur[i] = 1u << start[i];
    for (int t = 0; t < len; t++) {
        for (int j = 0; j < n; j++) nxt[j] = 0;
        for (int i = 0; i < n; i++) { u64 r = m->move[seq[t]][i];
            while (r) { int j = __builtin_ctzll(r); r &= r - 1; nxt[j] |= cur[i]; } }
        memcpy(cur, nxt, sizeof(unsigned short) * n);
    }
    for (int i = 0; i < n; i++) { if (popc(cur[i]) != 1) return 0; out[i] = __builtin_ctz(cur[i]); }
    return 1;
}

int main(int argc, char **argv) {
    n = argc > 1 ? atoi(argv[1]) : 5;
    ncol = argc > 2 ? atoi(argv[2]) : 3;
    int worlds = argc > 3 ? atoi(argv[3]) : 20, mode = argc > 4 ? atoi(argv[4]) : 1;
    probe_depth = argc > 5 ? atoi(argv[5]) : 0;
    if (n < 2 || n > MAXN || ncol < 2 || ncol > 16) { fprintf(stderr, "slots 2..64, colours 2..16\n"); return 1; }
    const int checks[] = {5, 10, 20, 40, 80, 160, 320, 640, 1280};
    const int nchk = 9, tests = 20;
    long right[9] = {0}, wrong[9] = {0}, claims[9] = {0};
    double bsum[9] = {0}; int pinned_at_chk[9] = {0}, broken = 0, pinned = 0; long pin_sum = 0;
    clock_t t0 = clock();
    static Ep eps[MAXEP];
    for (int w = 0; w < worlds; w++) {
        rng = 0x9E3779B97F4A7C15ULL * (w + 1) + (mode + 1) * 7919 + n * 104729ULL + ncol;
        int g[K][MAXN];
        for (;;) {      /* random acts; at least two must not commute */
            for (int k = 0; k < K; k++) { for (int i = 0; i < n; i++) g[k][i] = i;
                for (int i = n - 1; i > 0; i--) { int j = rnd() % (i + 1), t = g[k][i]; g[k][i] = g[k][j]; g[k][j] = t; } }
            int differ = 0;
            for (int i = 0; i < n; i++) if (g[1][g[0][i]] != g[0][g[1][i]]) differ = 1;
            if (differ) break;
        }
        Model m; memset(&m, 0, sizeof m);
        u64 all = n == 64 ? ~0ULL : (1ULL << n) - 1;
        for (int k = 0; k < K; k++) for (int i = 0; i < n; i++) m.move[k][i] = all;
        int ne = 0, pin = -1;
        for (int c = 0; c < nchk; c++) {
            while (ne < checks[c]) {
                Ep *e = &eps[ne];
                e->len = mode == 0 ? (ne < 10 ? 1 + rnd() % 2 : 1 + rnd() % 5) : 3 + rnd() % 3;
                unsigned char cur[MAXN], nxt[MAXN];
                for (int i = 0; i < n; i++) e->start[i] = cur[i] = rnd() % ncol;
                for (int t = 0; t < e->len; t++) { e->act[t] = rnd() % K;
                    for (int i = 0; i < n; i++) nxt[g[e->act[t]][i]] = cur[i];
                    memcpy(cur, nxt, n); }
                memcpy(e->end, cur, n); ne++;
                if (pin < 0 && !m.contradiction) {
                    think(&m, eps, ne);
                    if (!m.contradiction && open_moves(&m) == 0) pin = ne;
                }
            }
            bsum[c] += bits_bound(&m); pinned_at_chk[c] += pin > 0;
            for (int t = 0; t < tests && !m.contradiction; t++) {
                int seq[TESTLEN]; unsigned char st[MAXN], cur[MAXN], nxt[MAXN], guess[MAXN];
                for (int i = 0; i < n; i++) st[i] = cur[i] = rnd() % ncol;
                for (int s = 0; s < TESTLEN; s++) { seq[s] = rnd() % K;
                    for (int i = 0; i < n; i++) nxt[g[seq[s]][i]] = cur[i];
                    memcpy(cur, nxt, n); }
                if (predict(&m, st, seq, TESTLEN, guess)) { claims[c]++; if (!memcmp(guess, cur, n)) right[c]++; else wrong[c]++; }
            }
        }
        broken += m.contradiction;
        if (pin > 0) { pinned++; pin_sum += pin; }
    }
    double secs = (double)(clock() - t0) / CLOCKS_PER_SEC;
    double lg = 0; for (int i = 2; i <= n; i++) lg += log2(i);
    printf("slots %d, colours %d, worlds %d, mode %s, suppose %s\n", n, ncol, worlds,
           mode ? "lengths 3-5 only" : "short then long", probe_depth ? "on" : "off");
    printf("each act is one of %d! permutations: %.1f bits each, %.1f bits for all %d acts\n", n, lg, K * lg, K);
    printf("  episodes | bits left (at most) | worlds pinned | 1000-act tests right/wrong/claimed\n");
    for (int c = 0; c < nchk; c++)
        printf("  %8d | %19.1f | %8d/%-4d | %ld/%ld/%ld of %d\n", checks[c], bsum[c] / worlds,
               pinned_at_chk[c], worlds, right[c], wrong[c], claims[c], worlds * tests);
    printf("every act pinned in %d/%d worlds", pinned, worlds);
    if (pinned) printf(", after %.1f episodes on average", (double)pin_sum / pinned);
    printf("; contradictions %d (must be 0); %.1f s\n", broken, secs);
    return 0;
}
