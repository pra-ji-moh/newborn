# Neurons as players (Nov 26, 2026)

The neuron layer in `local.py` learns with no backprop. On Nov 14 its broadcast
rule got about 40% of what backprop gives. Here each neuron is treated as a
player in a game, three ways, on the same test: Penn Treebank, 10,000 training
sentences, the grown counting model plus the neuron layer, 5 times through,
one run each. Perplexity on the held-out test text, lower is better.

| Neuron layer | Perplexity | Share of backprop's gain |
|---|---|---|
| None (counting only) | 207.2 | 0% |
| Broadcast error, Nov 14 (rerun) | 199.1 | 38% |
| **Team game**: every neuron paid the same, learns by trial | 207.2 | 0% (layer not kept) |
| **Players learn the game** (Kolen-Pollack) | **187.2** | **94%** |
| **Dominated players eliminated** (broadcast + Hartley) | 198.4 | 41% |
| Backprop (yardstick) | 185.9 | 100% |

The two reruns matched Nov 14 exactly (199.1 and 185.9).

- **Players who learn the game get 94% of backprop's gain with no backprop.**
  Each neuron starts with a wrong guess at how its firing affects the payoff
  and changes that guess by the same local step as its own synapses, forgetting
  a little each time. Alone, this layer did better on the latest tenth than
  backprop's (281 against 300): backprop overfits there by the fifth pass.
- **This rule is not new.** It is Kolen and Pollack (1994); Akrout et al. (2019)
  showed it close to backprop on ImageNet. So the "wall" in the Nov 14 summary
  was overstated: known backprop-free rules already come close.
- **Part of its edge may be the forgetting,** which acts like weight decay.
  Backprop here had none. Backprop with the same decay is the fair next control.
- **The team game fails.** One shared payoff for 256 neurons is too noisy; it
  made the guess worse, so the grower did not keep it.
- **Eliminating dominated players barely helps** (198.4 against 199.1, one run;
  likely within noise). 120 of 256 neurons were replaced.

Run: `python3 grow.py DATA_DIR train_clean_10000.txt neurons=5 game=N`
(1 team, 2 learn, 3 eliminate). Raw output: `game_results.txt`.
