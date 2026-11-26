# Does the combination beat a fair opponent? (10k sentences, PTB, test perplexity, lower is better)

## Fair opponent
| Model | Test perplexity |
|---|---|
| Tuned LSTM (d=256, 1 layer, dropout 0.5, tied, Adam, early stop), 1st grid setting | **179.8** |
| Full combination (counting model + grown ways + kinds/far/recall + Kolen-Pollack neurons) | 187.2 |
| Full combination with backprop neurons | 185.9 |

The tuned LSTM beats the combination by 7.4 points. Its grid was still running when this was written
(see `combination_runs/lstm.txt`); the next setting was level on validation (198.2 vs 198.4).

## Ablation (game=2, one part removed at a time)
| Removed | Test perplexity | Change vs 187.2 |
|---|---|---|
| nothing | 187.2 | |
| word kinds (kinds=0) | 186.9 | -0.3 (better without it) |
| far-back words (far=0) | 187.7 | +0.5 |
| recall (recall=0) | 187.7 | +0.5 |
| grown ways (ways=1) | 198.0 | +10.8 |
| the counting model (neurons alone) | 273.0 | +85.8 |

## Verdict
- The combination does **not** beat a tuned LSTM at this size.
- Only two parts earn their place: the grown ways and the counting model itself. Kinds, far and recall add
  at most 0.5 each, which is noise-level; kinds actually hurts.
- The neurons and the counting model do help each other: 273 alone and 207 counting-only, but 187 together.
  A tuned LSTM gets 180 alone, however.
