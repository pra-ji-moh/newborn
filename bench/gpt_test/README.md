# Long-text test for a chat model (ChatGPT, Claude)

15 documents from the harder long-text test (`hartley/gen_long2.py`), 3 per
set: questions 50, 500, 2,000 and 8,000 words after the facts, and facts in
wordings never seen in training (`new_2000`). 16 questions each, 240 in all.
Made by `hartley/gpt_export.py`.

## How to run it

1. Open a **new chat** for each prompt (so documents don't mix), with no
   memory or custom instructions on.
2. Paste the whole of `prompt_<set>_<n>.txt`, exactly as it is.
3. Save the reply, as written, to `reply_<set>_<n>.txt` in this folder.
4. Score: `python3 hartley/score_gpt.py bench/gpt_test`

Don't paste `key.txt`: it holds the answers. Note which model and settings
you used (for example "GPT-5.4, default settings, date").

## The relational model on the same 15 documents

From `relational_model_scores.txt` (no gradients, seconds on a CPU):

| Set | Right |
|---|---|
| 50 words | 95.8% |
| 500 words | 97.9% |
| 2,000 words | 95.8% |
| 8,000 words | 95.8% |
| Never-seen wordings, 2,000 words | 70.8% |

Fair-comparison notes: the chat model reads natural questions ("Which city
does person110 live in now?"); the relational model reads the same facts
with questions in the test's own form ("person110 city ?"). The chat model
was not trained on this kind of document; the relational model learned from
1,500 training documents of it. Say both when you report.

## Runs so far

- GPT-6 in ChatGPT, Oct 28, 2026 (run by the author): prompts 50_1, 50_2,
  50_3; replies in `reply_*.txt`.
