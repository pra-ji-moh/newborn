/* Grouping histories by their future, on real language (Penn Treebank).
 *
 * The same model as lang.c's grouped_shannon_hartley, built for a
 * 10,000-word vocabulary: edges are stored sparsely in one hash table.
 * Histories (sentence beginnings) are grouped when they have the same
 * future: Shannon's test (a Hoeffding bound at level alpha, as in ALERGIA)
 * says when the rates of a next word differ beyond chance; Hartley's veto
 * says a word seen at least 3 times after one cannot be absent after the
 * other where at least 5 were due. Words absent from the new history are
 * checked among the group's 200 most frequent words, refreshed as it grows
 * (rarer words cannot exceed the bound at these counts).
 *
 * Prediction runs the groups; a word never seen from a group spreads the
 * belief to wherever that word has led. Each group's next-word rates are
 * smoothed toward a backup:
 *   grouped          - backup is how often each word occurs at all;
 *   grouped+trigram  - backup is a trigram model (Shannon weighs the group's
 *                      own evidence against the last two words).
 * Also scored: the trigram model alone (Witten-Bell).
 *
 * Build: cc -std=c99 -O2 real.c -o real -lm
 * Run:   ./real DATA_DIR TRAIN_FILE      (files from real_prep.py)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TOPK 200
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

/* ---------- edges: (node, word) -> count, target; kept in one hash ---------- */

typedef struct { int node, w, to, next; long c; } Edge;
static Edge *E; static int ne, cap_e;
static int *head; static long *nout; static char *red; static int nn, cap_n;
static int *htab; static unsigned hmask;

static unsigned hslot(int node, int w) { u64 k = (u64)node * 1000003ULL + (u64)w; k ^= k >> 31; k *= 0x9E3779B97F4A7C15ULL; return (unsigned)(k >> 32) & hmask; }
static int find(int node, int w) {
    for (unsigned h = hslot(node, w);; h = (h + 1) & hmask) { int i = htab[h]; if (i < 0) return -1; if (E[i].node == node && E[i].w == w) return i; }
}
static void grow_hash(void);
static int add_edge(int node, int w, int to, long c) {
    if (ne == cap_e) { cap_e = cap_e ? cap_e * 2 : 1 << 20; E = realloc(E, cap_e * sizeof(Edge)); }
    if ((unsigned)ne * 2 >= hmask) grow_hash();
    E[ne] = (Edge){node, w, to, head[node], c}; head[node] = ne;
    unsigned h = hslot(node, w); while (htab[h] >= 0) h = (h + 1) & hmask; htab[h] = ne;
    return ne++;
}
static void grow_hash(void) {
    unsigned size = hmask ? (hmask + 1) * 2 : 1 << 22; free(htab); htab = malloc(size * sizeof(int)); hmask = size - 1;
    for (unsigned i = 0; i < size; i++) htab[i] = -1;
    for (int i = 0; i < ne; i++) { unsigned h = hslot(E[i].node, E[i].w); while (htab[h] >= 0) h = (h + 1) & hmask; htab[h] = i; }
}
static int new_node(void) {
    if (nn == cap_n) { cap_n = cap_n ? cap_n * 2 : 1 << 20; head = realloc(head, cap_n * sizeof(int)); nout = realloc(nout, cap_n * sizeof(long)); red = realloc(red, cap_n); }
    head[nn] = -1; nout[nn] = 0; red[nn] = 0; return nn++;
}
static void build_tree(const Set *S) {
    ne = 0; nn = 0; if (!hmask) grow_hash(); else for (unsigned i = 0; i <= hmask; i++) htab[i] = -1;
    new_node();
    for (int i = 0; i < S->n; i++) { int v = 0;
        for (int t = 0; t < S->len[i]; t++) { int w = S->w[i][t]; int e = find(v, w);
            if (e < 0) { int to = w == END ? -1 : new_node(); e = add_edge(v, w, to, 0); }
            E[e].c++; nout[v]++; v = E[e].to; if (v < 0) break; } }
}

/* ---------- grouping ---------- */

static double alpha; static int use_veto;
static int *grp, ngrp; static int (*top)[TOPK]; static int *ntop; static long *top_at;

static void refresh_top(int gi) {
    int s = grp[gi], n = 0; int *t = top[gi];
    for (int e = head[s]; e >= 0; e = E[e].next) {
        if (n < TOPK) { t[n++] = e; }
        else { int m = 0; for (int j = 1; j < TOPK; j++) if (E[t[j]].c < E[t[m]].c) m = j; if (E[e].c > E[t[m]].c) t[m] = e; }
    }
    ntop[gi] = n; top_at[gi] = nout[s];
}
static int fits(long f1, long n1, long f2, long n2, double bound) {
    if (fabs((double)f1 / n1 - (double)f2 / n2) > bound) return 0;
    if (use_veto && ((f1 >= 3 && f2 == 0 && n2 * (double)f1 / n1 >= 5) || (f2 >= 3 && f1 == 0 && n1 * (double)f2 / n2 >= 5))) return 0;
    return 1;
}
static int compatible(int r, int b, int gi, int depth) {
    long n1 = nout[r], n2 = nout[b];
    if (depth > 30 || !n1 || !n2) return 1;
    double bound = sqrt(0.5 * log(2 / alpha)) * (1 / sqrt((double)n1) + 1 / sqrt((double)n2));
    if (bound >= 1 && !use_veto) return 1;
    for (int e = head[b]; e >= 0; e = E[e].next) { int x = find(r, E[e].w); if (!fits(x < 0 ? 0 : E[x].c, n1, E[e].c, n2, bound)) return 0; }
    if (gi >= 0) { for (int j = 0; j < ntop[gi]; j++) { int x = top[gi][j]; if (find(b, E[x].w) < 0 && !fits(E[x].c, n1, 0, n2, bound)) return 0; } }
    else for (int x = head[r]; x >= 0; x = E[x].next) if (find(b, E[x].w) < 0 && !fits(E[x].c, n1, 0, n2, bound)) return 0;
    for (int e = head[b]; e >= 0; e = E[e].next) { if (E[e].to < 0) continue; int x = find(r, E[e].w);
        if (x >= 0 && E[x].to >= 0 && !compatible(E[x].to, E[e].to, -1, depth + 1)) return 0; }
    return 1;
}
static void push(int e);
/* Fold b's branch into r. A group that gains a branch to an unplaced
 * history queues it, so every edge from a group ends in a group. */
static void fold(int r, int b) {
    nout[r] += nout[b];
    for (int e = head[b]; e >= 0; e = E[e].next) {
        int x = find(r, E[e].w);
        if (x < 0) { x = add_edge(r, E[e].w, E[e].to, E[e].c); if (red[r] && E[x].to >= 0 && !red[E[x].to]) push(x); }
        else { E[x].c += E[e].c; if (E[x].to >= 0 && E[e].to >= 0 && E[x].to != E[e].to) fold(E[x].to, E[e].to); }
    }
}

/* candidates: edges from a group to a history not yet placed, most seen first */
typedef struct { long c; int e; } Cand;
static Cand *heap; static int nh, cap_h;
static void push(int e) {
    if (nh == cap_h) { cap_h = cap_h ? cap_h * 2 : 1 << 16; heap = realloc(heap, cap_h * sizeof(Cand)); }
    int i = nh++; heap[i] = (Cand){nout[E[e].to], e};
    while (i && heap[(i - 1) / 2].c < heap[i].c) { Cand t = heap[i]; heap[i] = heap[(i - 1) / 2]; heap[(i - 1) / 2] = t; i = (i - 1) / 2; }
}
static Cand pop(void) {
    Cand top_ = heap[0]; heap[0] = heap[--nh]; int i = 0;
    for (;;) { int l = 2 * i + 1, r = l + 1, m = i;
        if (l < nh && heap[l].c > heap[m].c) m = l; if (r < nh && heap[r].c > heap[m].c) m = r;
        if (m == i) break; Cand t = heap[i]; heap[i] = heap[m]; heap[m] = t; i = m; }
    return top_;
}
static void push_edges(int s) { for (int e = head[s]; e >= 0; e = E[e].next) if (E[e].to >= 0 && !red[E[e].to]) push(e); }

static void group_histories(const Set *S) {
    build_tree(S);
    grp = realloc(grp, sizeof(int) * nn); top = realloc(top, sizeof *top * 1); ngrp = 0; nh = 0;
    int capg = 1; ntop = realloc(ntop, sizeof(int)); top_at = realloc(top_at, sizeof(long));
    red[0] = 1; grp[ngrp] = 0; refresh_top(ngrp); ngrp++; push_edges(0);
    while (nh) {
        Cand cd = pop(); int e = cd.e, b = E[e].to;
        if (b < 0 || red[b]) continue;
        int placed = 0;
        for (int i = 0; i < ngrp && !placed; i++) {
            if (nout[grp[i]] > 2 * top_at[i]) refresh_top(i);
            if (compatible(grp[i], b, i, 0)) { E[e].to = grp[i]; fold(grp[i], b); placed = 1; }
        }
        if (!placed) {
            red[b] = 1;
            if (ngrp == capg) { capg *= 2; top = realloc(top, sizeof *top * capg); ntop = realloc(ntop, sizeof(int) * capg); top_at = realloc(top_at, sizeof(long) * capg); }
            grp[ngrp] = b; refresh_top(ngrp); ngrp++; push_edges(b);
        }
    }
}

/* ---------- the trigram backup (Witten-Bell), sparse ---------- */

typedef struct { u64 key; long c; int types; } Ng;
static Ng *ng; static unsigned ngmask;
static Ng *ng_get(u64 key, int create) {
    for (unsigned h = (unsigned)((key * 0x9E3779B97F4A7C15ULL) >> 40) & ngmask;; h = (h + 1) & ngmask) {
        if (ng[h].key == key) return &ng[h];
        if (ng[h].key == 0) { if (!create) return NULL; ng[h].key = key; return &ng[h]; } }
}
/* keys: kind (1 uni, 2 bi, 3 tri, 4 context bi, 5 context tri), words + 2 */
static u64 K(int kind, int a, int b, int c) { return ((u64)kind << 60) | ((u64)(a + 2) << 40) | ((u64)(b + 2) << 20) | (u64)(c + 2); }
static long total_words; static double *uni;
static void build_ngrams(const Set *S) {
    ngmask = (1u << 24) - 1; free(ng); ng = calloc(ngmask + 1, sizeof(Ng)); total_words = 0;
    for (int i = 0; i < S->n; i++) { int a = -1, b = -1;
        for (int t = 0; t < S->len[i]; t++) { int w = S->w[i][t];
            Ng *x;
            x = ng_get(K(3, a, b, w), 1); if (!x->c++) ng_get(K(5, a, b, 0), 1)->types++;
            ng_get(K(5, a, b, 0), 1)->c++;
            x = ng_get(K(2, 0, b, w), 1); if (!x->c++) ng_get(K(4, 0, b, 0), 1)->types++;
            ng_get(K(4, 0, b, 0), 1)->c++;
            ng_get(K(1, 0, 0, w), 1)->c++; total_words++;
            a = b; b = w; } }
    free(uni); uni = malloc(sizeof(double) * (V + 1));
    for (int w = 0; w <= V; w++) { Ng *x = ng_get(K(1, 0, 0, w), 0); uni[w] = ((x ? x->c : 0) + 1.0) / (total_words + V + 1); }
}
static double trigram(int a, int b, int w) {
    double p = uni[w]; Ng *c, *x;
    if ((c = ng_get(K(4, 0, b, 0), 0)) && c->c) { x = ng_get(K(2, 0, b, w), 0); double l = (double)c->c / (c->c + c->types); p = l * (x ? x->c : 0) / c->c + (1 - l) * p; }
    if ((c = ng_get(K(5, a, b, 0), 0)) && c->c) { x = ng_get(K(3, a, b, w), 0); double l = (double)c->c / (c->c + c->types); p = l * (x ? x->c : 0) / c->c + (1 - l) * p; }
    return p;
}

/* ---------- scoring: bits per word ---------- */

static double score_trigram(const Set *T) {
    double bits = 0; long n = 0;
    for (int i = 0; i < T->n; i++) { int a = -1, b = -1;
        for (int t = 0; t < T->len[i]; t++) { int w = T->w[i][t]; bits -= log2(trigram(a, b, w)); n++; a = b; b = w; } }
    return bits / n;
}

/* lead[w]: where word w has led, over the groups, as a sparse list */
typedef struct { int g; double p; } Lead;
static Lead **lead; static int *nlead;
static int *gidx;
static void build_leads(void) {
    gidx = realloc(gidx, sizeof(int) * nn); for (int i = 0; i < ngrp; i++) gidx[grp[i]] = i;
    free(nlead); nlead = calloc(V + 1, sizeof(int));
    if (lead) for (int w = 0; w <= V; w++) free(lead[w]);
    free(lead); lead = calloc(V + 1, sizeof(Lead *));
    int *capl = calloc(V + 1, sizeof(int)); double *tot = calloc(V + 1, sizeof(double));
    for (int i = 0; i < ngrp; i++) for (int e = head[grp[i]]; e >= 0; e = E[e].next) {
        int w = E[e].w, to = E[e].to; if (to < 0) continue;
        int g = gidx[to];
        if (nlead[w] == capl[w]) { capl[w] = capl[w] ? capl[w] * 2 : 4; lead[w] = realloc(lead[w], capl[w] * sizeof(Lead)); }
        lead[w][nlead[w]++] = (Lead){g, (double)E[e].c}; tot[w] += E[e].c;
    }
    for (int w = 0; w <= V; w++) for (int j = 0; j < nlead[w]; j++) lead[w][j].p /= tot[w];
    free(capl); free(tot);
}
static double score_groups(const Set *T, double beta, int backup_trigram) {
    double *bel = calloc(ngrp, sizeof(double)), *nb = calloc(ngrp, sizeof(double)), bits = 0; long n = 0;
    int *act = malloc(sizeof(int) * ngrp), nact, *nxt = malloc(sizeof(int) * ngrp), nnx; char *on = calloc(ngrp, 1);
    for (int i = 0; i < T->n; i++) {
        nact = 1; act[0] = 0; bel[0] = 1; int a = -1, b = -1;
        for (int t = 0; t < T->len[i]; t++) {
            int w = T->w[i][t]; double q = backup_trigram ? trigram(a, b, w) : uni[w], pw = 0;
            nnx = 0;
            for (int k = 0; k < nact; k++) { int j = act[k], s = grp[j]; if (bel[j] <= 0) continue;
                int e = find(s, w); long c = e < 0 ? 0 : E[e].c;
                double pj = (c + beta * q) / (nout[s] + beta), m = bel[j] * pj; pw += m;
                if (w == END) continue;
                if (e >= 0 && E[e].to >= 0) { int g = gidx[E[e].to]; if (!on[g]) { on[g] = 1; nxt[nnx++] = g; } nb[g] += m; }
                else for (int l = 0; l < nlead[w]; l++) { int g = lead[w][l].g; if (!on[g]) { on[g] = 1; nxt[nnx++] = g; } nb[g] += m * lead[w][l].p; }
            }
            bits -= log2(pw); n++;
            for (int k = 0; k < nact; k++) bel[act[k]] = 0;
            double s = 0; for (int k = 0; k < nnx; k++) s += nb[nxt[k]];
            /* keep the belief small: drop groups under a millionth */
            nact = 0;
            for (int k = 0; k < nnx; k++) { int g = nxt[k]; on[g] = 0; double v = nb[g] / s; nb[g] = 0; if (v > 1e-6) { bel[g] = v; act[nact++] = g; } }
            if (!nact && w != END) { nact = 1; act[0] = 0; bel[0] = 1; }
            a = b; b = w;
        }
        for (int k = 0; k < nact; k++) bel[act[k]] = 0;
    }
    free(bel); free(nb); free(act); free(nxt); free(on);
    return bits / n;
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: real DATA_DIR TRAIN_FILE\n"); return 1; }
    char path[1024]; snprintf(path, sizeof path, "%s/meta.txt", argv[1]);
    FILE *mf = fopen(path, "r"); if (!mf || fscanf(mf, "V %d", &V) != 1) { fprintf(stderr, "no meta\n"); return 1; } fclose(mf);
    END = V;
    Set tr, va, te;
    snprintf(path, sizeof path, "%s/%s", argv[1], argv[2]); load(&tr, path);
    snprintf(path, sizeof path, "%s/valid.txt", argv[1]); load(&va, path);
    snprintf(path, sizeof path, "%s/test.txt", argv[1]); load(&te, path);
    build_ngrams(&tr);
    printf("TRAIN %s: %d sentences, %ld words\n", argv[2], tr.n, tr.tokens);
    double tri = score_trigram(&te);
    printf("RESULT trigram bits %.4f perplexity %.1f\n", tri, pow(2, tri));
    const double alphas[] = {0.001, 0.05, 0.5}, betas[] = {1, 10, 100, 1000};
    for (int backup = 0; backup < 2; backup++) for (int veto = 0; veto < 2; veto++) {
        use_veto = veto; double best = 1e300, ba = 0, bb = 0;
        for (int i = 0; i < 3; i++) { alpha = alphas[i]; group_histories(&tr); build_leads();
            for (int j = 0; j < 4; j++) { double v = score_groups(&va, betas[j], backup); if (v < best) { best = v; ba = alphas[i]; bb = betas[j]; } } }
        alpha = ba; group_histories(&tr); build_leads();
        double bits = score_groups(&te, bb, backup);
        printf("RESULT grouped%s%s bits %.4f perplexity %.1f groups %d alpha %g beta %g\n", veto ? "_hartley" : "", backup ? "+trigram" : "",
               bits, pow(2, bits), ngrp, ba, bb);
        fflush(stdout);
    }
    return 0;
}
