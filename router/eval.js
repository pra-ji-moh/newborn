// Score the routers on the public test set. Usage: node eval.js [dev|test]
// Writes results/<split>.json and prints a table.
const fs = require("fs");
const R = require("./router.js");
const split = process.argv[2] || "dev";
const rows = fs.readFileSync(__dirname + "/data/eval.jsonl", "utf8").trim().split("\n").map(JSON.parse)
  .filter((r) => r.split === split);

const strategies = {
  "Everything to the big model": () => "big",
  "Everything to the small model": () => "small",
  "Topic router (science goes big)": (q) => R.routeByTopic(q).model,
  "Order router (ours)": (q) => R.route(q).model,
};

const bigCost = rows.reduce((s, r) => s + R.cost("big", r.in_tok, r.out_tok), 0);
const out = { split, n: rows.length, bigCostPer1000: (bigCost / rows.length) * 1000, strategies: {}, bySource: {}, misses: [] };

for (const [name, f] of Object.entries(strategies)) {
  let c = 0, missed = 0, wasted = 0, needBig = 0, lookups = 0;
  for (const r of rows) {
    const m = f(r.q);
    c += R.cost(m, r.in_tok, r.out_tok);
    if (r.needs_big) { needBig++; if (m === "small") missed++; } else { lookups++; if (m === "big") wasted++; }
  }
  out.strategies[name] = {
    costPer1000: (c / rows.length) * 1000,
    saved: 1 - c / bigCost,
    missedPct: missed / needBig,     // needed the big model, sent small: quality risk
    wastedPct: wasted / lookups,     // a lookup sent big: money wasted
    accuracy: 1 - (missed + wasted) / rows.length,
  };
}

// Per source, and the anomaly: needs the big model but is not about science.
for (const r of rows) {
  const s = (out.bySource[r.source] ??= { n: 0, needsBig: r.needs_big, oursBig: 0, topicBig: 0 });
  s.n++;
  if (R.route(r.q).model === "big") s.oursBig++;
  if (R.routeByTopic(r.q).model === "big") s.topicBig++;
}
const anomalies = rows.filter((r) => r.needs_big && R.routeByTopic(r.q).topic !== "science");
out.anomaly = {
  n: anomalies.length,
  caughtByOurs: anomalies.filter((r) => R.route(r.q).model === "big").length,
  caughtByTopic: 0,
};
out.misses = rows.filter((r) => (R.route(r.q).model === "big") !== r.needs_big).slice(0, 40)
  .map((r) => ({ source: r.source, needs_big: r.needs_big, q: r.q.slice(0, 160) }));

fs.mkdirSync(__dirname + "/results", { recursive: true });
fs.writeFileSync(`${__dirname}/results/${split}.json`, JSON.stringify(out, null, 1));

const pct = (x) => (100 * x).toFixed(1) + "%";
console.log(`${split}: ${rows.length} real queries; all-big costs $${out.bigCostPer1000.toFixed(2)} per 1,000\n`);
console.log("strategy".padEnd(34), "cost/1k".padStart(8), "saved".padStart(7), "missed".padStart(7), "wasted".padStart(7), "right".padStart(7));
for (const [k, v] of Object.entries(out.strategies))
  console.log(k.padEnd(34), ("$" + v.costPer1000.toFixed(2)).padStart(8), pct(v.saved).padStart(7), pct(v.missedPct).padStart(7), pct(v.wastedPct).padStart(7), pct(v.accuracy).padStart(7));
console.log("\nsent to the big model, by source (ours / topic router):");
for (const [k, v] of Object.entries(out.bySource))
  console.log("  " + k.padEnd(10), (v.needsBig ? "needs big " : "lookup    "), `${v.oursBig}/${v.n}`.padStart(8), `${v.topicBig}/${v.n}`.padStart(8));
console.log(`\nanomalies (need the big model, not about science): ${out.anomaly.n}; ours caught ${out.anomaly.caughtByOurs}, topic router caught 0`);
