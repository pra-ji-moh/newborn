# Order router (prototype)

Sends an AI query to the big model only when its work does not commute: it asks
for something new, or its steps must happen in order (non-abelian). Lookups of
existing material (abelian) go to the small model. Topic is never used to decide.

`index.html` is the demo page; the router runs in the browser.

## Results (held-out half, 1,500 real queries)

| Strategy | Cost per 1,000 | Saved | Missed | Wasted | Routed right |
|---|---|---|---|---|---|
| Everything to the big model | $2.07 | 0% | 0% | 100% | 40.0% |
| Topic router (science goes big) | $0.61 | 71% | 45% | 19% | 70.5% |
| **Order router** | **$1.05** | **49%** | **22%** | **1%** | **90.3%** |

Anomalies (need the big model, not about science): 150 of 272 caught; a topic
router catches 0. Prices: Claude Opus 5.5 vs Claude Haiku 5.5 list prices.

**Not measured yet:** answer quality (no model was called), a real customer's
query mix, and commercial routers.

## Data

Real queries from SciQ, WebQuestions and Dolly-15k open_qa (lookups), GSM8K and
Dolly-15k creative_writing (need the big model). The label comes from the
dataset, not from us. The data is not committed; to rebuild:

```
RAW=some/dir; mkdir -p $RAW && cd $RAW
curl -LO https://raw.githubusercontent.com/openai/grade-school-math/master/grade_school_math/data/test.jsonl && mv test.jsonl gsm8k_test.jsonl
curl -L -o sciq.zip https://ai2-public-datasets.s3.amazonaws.com/sciq/SciQ.zip && mkdir sciq && (cd sciq && unzip ../sciq.zip)
curl -L -o webq.json https://raw.githubusercontent.com/brmson/dataset-factoid-webquestions/master/main/trainmodel.json
curl -L -o dolly.jsonl https://raw.githubusercontent.com/databrickslabs/dolly/d000e3030970379aabbf6d291f50ffdd3b715b64/data/databricks-dolly-15k.jsonl
cd -; python3 build_eval.py $RAW
node eval.js dev; node eval.js test; node build_page.js
```

Rules were tuned on the dev half only; the test half was scored once they were fixed.

## Files

- `router.js`: the router (browser and Node)
- `build_eval.py`: builds `data/eval.jsonl` from the public datasets
- `eval.js`: scores the strategies, writes `results/<split>.json`
- `page.html` + `build_page.js`: build `index.html`
