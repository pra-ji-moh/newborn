// Build index.html: page.html with the router code and the held-out results put in.
// Run after eval.js test. Usage: node build_page.js
const fs = require("fs");
const R = require("./router.js");
const res = JSON.parse(fs.readFileSync(__dirname + "/results/test.json", "utf8"));
const rows = fs.readFileSync(__dirname + "/data/eval.jsonl", "utf8").trim().split("\n").map(JSON.parse).filter((r) => r.split === "test");
const cells = {};
for (const r of rows) { const x = R.route(r.q); const k = x.topic + "|" + x.kind; cells[k] = (cells[k] || 0) + 1; }
const results = { strategies: res.strategies, anomaly: res.anomaly, cells, n: res.n };
const page = fs.readFileSync(__dirname + "/page.html", "utf8")
  .replace("/*ROUTER*/", () => fs.readFileSync(__dirname + "/router.js", "utf8"))
  .replace("/*RESULTS*/", () => JSON.stringify(results));
fs.writeFileSync(__dirname + "/index.html", page);
console.log("index.html", page.length, "bytes", cells);
