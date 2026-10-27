/* The Hartley transform on S5: the second Hartley.
 *
 * Hartley's 1942 transform is the Fourier transform written with real numbers
 * only (cas = cos + sin). On a group where order matters, the Fourier
 * transform turns each function into one matrix per irreducible
 * representation. For the permutations of five things (S5), every one of
 * those representations can be written with real numbers (Young's orthogonal
 * form), so the transform is real and its inverse is a transpose: a
 * non-abelian Hartley transform.
 *
 * What this program checks, by brute force:
 *   1. The seven representations of S5 (sizes 1,4,5,6,5,4,1) are real,
 *      orthogonal, and respect composition. 1+16+25+36+25+16+1 = 120.
 *   2. The transform is exactly invertible.
 *   3. "A then B" becomes a matrix product, and the product is in the
 *      reverse order: order lives in the transform itself. For sets of
 *      possibilities A and B, the possibilities for "A then B" are read off
 *      a product of matrices, never by listing pairs.
 *   4. How much of a possibility set the low bands hold. The first band
 *      (sizes 1 and 4, together "which slot goes where") is what band.c
 *      eliminates on at scale.
 *
 * Prior art: Fourier transforms on permutations for tracking beliefs are
 * Kondor, Howard and Jebara (2007) and Huang, Guestrin and Guibas (JMLR
 * 2009), with probabilities. Here the functions are Hartley's: possible or
 * not, nothing weighed.
 *
 * Build: cc -std=c99 -O2 transform.c -o transform -lm
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define N 5
#define NP 120
#define NIR 7
#define MAXD 6

static int perm[NP][N];       /* perm[p][i] = where slot i's colour goes */
static int comp[NP][NP];      /* comp[a][b] = do a, then b */

/* The partitions of five, largest first. */
static const int shape[NIR][N] = {
    {5}, {4,1}, {3,2}, {3,1,1}, {2,2,1}, {2,1,1,1}, {1,1,1,1,1}
};
static int dim[NIR];
static int tab[NIR][MAXD][N];          /* tab[r][t][v] = row*8+col of value v */
static double rho[NIR][NP][MAXD][MAXD];

static unsigned long long rng = 88172645463325252ULL;
static unsigned rnd(void) {
    rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
    return (unsigned)(rng >> 11);
}

static void setup_perms(void) {
    int n = 0, a[N];
    for (a[0] = 0; a[0] < N; a[0]++) for (a[1] = 0; a[1] < N; a[1]++)
    for (a[2] = 0; a[2] < N; a[2]++) for (a[3] = 0; a[3] < N; a[3]++)
    for (a[4] = 0; a[4] < N; a[4]++) {
        int used = 0, ok = 1;
        for (int i = 0; i < N; i++) { if (used >> a[i] & 1) ok = 0; used |= 1 << a[i]; }
        if (ok) { memcpy(perm[n], a, sizeof a); n++; }
    }
    for (int a1 = 0; a1 < NP; a1++) for (int b = 0; b < NP; b++) {
        int q[N];
        for (int i = 0; i < N; i++) q[i] = perm[b][perm[a1][i]];
        for (int p = 0; p < NP; p++) if (!memcmp(q, perm[p], sizeof q)) { comp[a1][b] = p; break; }
    }
}

/* Standard tableaux: place 0..4 in turn, each at the end of a row whose
 * row above is already longer. */
static void tableaux(int r, int v, int *len, int *cur) {
    if (v == N) { memcpy(tab[r][dim[r]++], cur, sizeof(int) * N); return; }
    for (int row = 0; row < N && shape[r][row]; row++)
        if (len[row] < shape[r][row] && (row == 0 || len[row - 1] > len[row])) {
            cur[v] = row * 8 + len[row]; len[row]++;
            tableaux(r, v + 1, len, cur);
            len[row]--;
        }
}

/* Young's orthogonal form for the swap of values k and k+1. */
static void adjacent(int r, int k, double m[MAXD][MAXD]) {
    memset(m, 0, sizeof(double) * MAXD * MAXD);
    for (int t = 0; t < dim[r]; t++) {
        int a = tab[r][t][k], b = tab[r][t][k + 1];
        int d = ((b & 7) - (b >> 3)) - ((a & 7) - (a >> 3));   /* content(k+1) - content(k) */
        m[t][t] = 1.0 / d;
        if (d == 1 || d == -1) continue;
        int sw[N]; memcpy(sw, tab[r][t], sizeof sw); sw[k] = b; sw[k + 1] = a;
        for (int u = 0; u < dim[r]; u++) if (!memcmp(sw, tab[r][u], sizeof sw))
            m[u][t] = sqrt(1.0 - 1.0 / ((double)d * d));
    }
}

static void matmul(int d, double a[MAXD][MAXD], double b[MAXD][MAXD], double out[MAXD][MAXD]) {
    double t[MAXD][MAXD] = {{0}};
    for (int i = 0; i < d; i++) for (int j = 0; j < d; j++)
        for (int k = 0; k < d; k++) t[i][j] += a[i][k] * b[k][j];
    memcpy(out, t, sizeof t);
}

/* rho(p): sort perm[p] to the identity by swapping neighbouring positions;
 * the swaps, read backwards, spell p as a product of adjacent swaps. */
static void build_reps(void) {
    for (int r = 0; r < NIR; r++) {
        int len[N] = {0}, cur[N];
        tableaux(r, 0, len, cur);
        double s[N - 1][MAXD][MAXD];
        for (int k = 0; k < N - 1; k++) adjacent(r, k, s[k]);
        for (int p = 0; p < NP; p++) {
            double m[MAXD][MAXD] = {{0}};
            for (int i = 0; i < dim[r]; i++) m[i][i] = 1;
            int a[N]; memcpy(a, perm[p], sizeof a);
            for (int pass = 0; pass < N; pass++) for (int k = 0; k < N - 1; k++)
                if (a[k] > a[k + 1]) { int t = a[k]; a[k] = a[k + 1]; a[k + 1] = t; matmul(dim[r], s[k], m, m); }
            memcpy(rho[r][p], m, sizeof m);
        }
    }
}

typedef double Hat[NIR][MAXD][MAXD];

static void forward(const double *f, Hat out) {
    memset(out, 0, sizeof(Hat));
    for (int r = 0; r < NIR; r++) for (int p = 0; p < NP; p++) if (f[p] != 0)
        for (int i = 0; i < dim[r]; i++) for (int j = 0; j < dim[r]; j++) out[r][i][j] += f[p] * rho[r][p][i][j];
}

/* f(p) = (1/120) sum over bands of dim * trace(rho(p)^T fhat), keeping only
 * the bands in mask. */
static void inverse(Hat h, int mask, double *f) {
    for (int p = 0; p < NP; p++) {
        double s = 0;
        for (int r = 0; r < NIR; r++) if (mask >> r & 1)
            for (int i = 0; i < dim[r]; i++) for (int j = 0; j < dim[r]; j++) s += dim[r] * rho[r][p][i][j] * h[r][i][j];
        f[p] = s / NP;
    }
}

static void random_set(double *f, int size) {
    memset(f, 0, sizeof(double) * NP);
    for (int n = 0; n < size;) { int p = rnd() % NP; if (!f[p]) { f[p] = 1; n++; } }
}

int main(void) {
    setup_perms();
    build_reps();

    /* 1. Real, orthogonal, and respecting composition. */
    int sq = 0; double worst_orth = 0, worst_hom = 0;
    for (int r = 0; r < NIR; r++) {
        sq += dim[r] * dim[r];
        for (int p = 0; p < NP; p++) {
            double t[MAXD][MAXD];
            for (int i = 0; i < dim[r]; i++) for (int j = 0; j < dim[r]; j++) {
                double s = 0; for (int k = 0; k < dim[r]; k++) s += rho[r][p][k][i] * rho[r][p][k][j];
                double e = fabs(s - (i == j)); if (e > worst_orth) worst_orth = e;
            }
            for (int q = 0; q < NP; q++) {
                matmul(dim[r], rho[r][q], rho[r][p], t);   /* rho(q) rho(p) */
                for (int i = 0; i < dim[r]; i++) for (int j = 0; j < dim[r]; j++) {
                    double e = fabs(t[i][j] - rho[r][comp[p][q]][i][j]);
                    if (e > worst_hom) worst_hom = e;
                }
            }
        }
    }
    printf("1. Seven bands of sizes");
    for (int r = 0; r < NIR; r++) printf(" %d", dim[r]);
    printf("; squares sum to %d (of %d).\n", sq, NP);
    printf("   All real. Largest error: orthogonality %.1e, composition %.1e.\n", worst_orth, worst_hom);
    printf("   'p then q' is rho(q) rho(p): the later act multiplies on the left.\n");

    /* 2. Exact inverse. */
    double f[NP], g[NP], back[NP]; Hat fh, gh;
    double worst_inv = 0;
    for (int trial = 0; trial < 100; trial++) {
        for (int p = 0; p < NP; p++) f[p] = (double)(rnd() % 1000) / 999.0 - 0.5;
        forward(f, fh); inverse(fh, 127, back);
        for (int p = 0; p < NP; p++) if (fabs(back[p] - f[p]) > worst_inv) worst_inv = fabs(back[p] - f[p]);
    }
    printf("2. Inverse of the transform, 100 random functions: largest error %.1e.\n", worst_inv);

    /* 3. "A then B" as a matrix product, and order. */
    int agree = 0, differ = 0, trials = 1000; double worst_conv = 0;
    for (int trial = 0; trial < trials; trial++) {
        random_set(f, 1 + rnd() % 20); random_set(g, 1 + rnd() % 20);
        double ab[NP] = {0}, ba[NP] = {0};      /* counts, listed the slow way */
        for (int a = 0; a < NP; a++) if (f[a]) for (int b = 0; b < NP; b++) if (g[b]) { ab[comp[a][b]]++; ba[comp[b][a]]++; }
        forward(f, fh); forward(g, gh);
        Hat ph;                                  /* A then B: gh times fh */
        for (int r = 0; r < NIR; r++) matmul(dim[r], gh[r], fh[r], ph[r]);
        inverse(ph, 127, back);
        int same_support = 1, ab_ba = 0;
        for (int p = 0; p < NP; p++) {
            if (fabs(back[p] - ab[p]) > worst_conv) worst_conv = fabs(back[p] - ab[p]);
            if ((back[p] > 0.5) != (ab[p] > 0)) same_support = 0;
            if ((ab[p] > 0) != (ba[p] > 0)) ab_ba = 1;
        }
        agree += same_support; differ += ab_ba;
    }
    printf("3. %d random pairs of possibility sets (1 to 20 each):\n", trials);
    printf("   'A then B' read off one matrix product per band: exact in %d/%d (largest error %.1e).\n",
           agree, trials, worst_conv);
    printf("   'A then B' and 'B then A' allow different outcomes in %d/%d pairs.\n", differ, trials);

    /* 4. How much of a possibility set the low bands hold. */
    const int sizes[] = {1, 4, 12, 30, 60, 120};
    const int masks[] = {1, 1 | 2, 1 | 2 | 4 | 8, 127};
    const char *mname[] = {"band 0", "bands 0-1", "bands 0-3", "all"};
    printf("4. Share of each possibility set's energy, by bands kept (200 random sets per size):\n");
    printf("   size |");
    for (int m = 0; m < 4; m++) printf(" %9s |", mname[m]);
    printf(" set recovered from bands 0-1\n");
    for (int si = 0; si < 6; si++) {
        double share[4] = {0}; int rec = 0;
        for (int trial = 0; trial < 200; trial++) {
            random_set(f, sizes[si]); forward(f, fh);
            double total = 0, part[NIR] = {0};
            for (int r = 0; r < NIR; r++) {
                for (int i = 0; i < dim[r]; i++) for (int j = 0; j < dim[r]; j++) part[r] += dim[r] * fh[r][i][j] * fh[r][i][j];
                total += part[r];
            }
            for (int m = 0; m < 4; m++) { double s = 0; for (int r = 0; r < NIR; r++) if (masks[m] >> r & 1) s += part[r]; share[m] += s / total; }
            /* Read back the support from the first band alone: the slot
             * matrix says which slot can go where; keep what it allows. */
            double slot[N][N] = {{0}};
            for (int p = 0; p < NP; p++) if (f[p]) for (int i = 0; i < N; i++) slot[i][perm[p][i]] = 1;
            int n = 0, ok = 1;
            for (int p = 0; p < NP; p++) {
                int allowed = 1; for (int i = 0; i < N; i++) if (!slot[i][perm[p][i]]) allowed = 0;
                n += allowed; if (allowed && !f[p]) ok = 0;
            }
            rec += ok;
        }
        printf("   %4d |", sizes[si]);
        for (int m = 0; m < 4; m++) printf("   %5.1f%%  |", 100 * share[m] / 200);
        printf(" %d/200\n", rec);
    }

    /* The sets one observation leaves: every act that carries a seen start
     * to a seen end. These are what elimination works with. */
    int rec1 = 0, rec_and = 0, tot1 = 0;
    for (int trial = 0; trial < 1000; trial++) {
        int st[N], en[N], truth = rnd() % NP, st2[N], en2[N];
        for (int i = 0; i < N; i++) { st[i] = rnd() % 3; st2[i] = rnd() % 3; }
        for (int i = 0; i < N; i++) { en[perm[truth][i]] = st[i]; en2[perm[truth][i]] = st2[i]; }
        for (int both = 0; both < 2; both++) {
            double slot[N][N] = {{0}};
            for (int p = 0; p < NP; p++) {
                int ok = 1; for (int i = 0; i < N; i++) if (en[perm[p][i]] != st[i] || (both && en2[perm[p][i]] != st2[i])) ok = 0;
                f[p] = ok; if (ok) for (int i = 0; i < N; i++) slot[i][perm[p][i]] = 1;
            }
            int ok = 1;
            for (int p = 0; p < NP; p++) { int al = 1; for (int i = 0; i < N; i++) if (!slot[i][perm[p][i]]) al = 0; if (al != (f[p] != 0)) ok = 0; }
            if (both) rec_and += ok; else rec1 += ok;
        }
        tot1++;
    }
    printf("   Sets left by one observation, recovered exactly from bands 0-1: %d/%d.\n", rec1, tot1);
    printf("   Sets left by two observations of the same act: %d/%d.\n", rec_and, tot1);
    return 0;
}
