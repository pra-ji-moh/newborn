// The order router: sends a query to the big model only when its work does not
// commute. Two things make work non-commutative:
//   CREATE  it asks for something that does not exist yet (novelty)
//   CHAIN   its quantities have to be combined in order (step 2 needs step 1)
// Everything else is a lookup of existing material: the facts can be fetched in
// any order and merged, so the small model is enough.
// The query's topic is never used for the decision; it is only reported, so the
// 2x2 cell can be shown. Same file runs in the browser and in eval.js.
(function (root) {
  // A make-something verb counts only as a command ("write a...", "can you invent"),
  // not inside a question about the past ("what did Edison invent?").
  const CMD = /(^|[.!?]\s+|\b(please|can you|could you|would you|i want you to|help me|i need you to|i'd like you to|let's)\s+)(write|compose|draft|create|invent|imagine|design|come up with|generate|craft|pretend|make up|make|rewrite|describe|plan|suggest|give me|tell me a|tell a|brainstorm|summarize|explain to|convince|persuade)\b/i;
  const ARTEFACT = /\b(poem|story|essay|letter|song|lyrics|haiku|limerick|slogan|tweet|speech|monologue|dialogue|script|diary|blog post|toast|eulogy|pitch|advert|ad copy|tagline|short story|menu|itinerary|plan|outline)\b/i;
  const STANCE = /\b(argument (for|against)|point of view|from the perspective|as if you|in the style of|you are (a|an|the)|imagine you|pretend you|should (we|i|you|people)|why should|do you think|what do you think|your opinion|pros and cons|recommend|convince|persuade|ideas (for|to|on)|some ideas|why (is|are) .{2,60} (the best|so (good|great|bad)|such an?|better)|why i (want|love|should)|you (stumble|wake up|find yourself|discover))\b/i;
  const TAILOR = /\b(i am|i'm|i have|i've|i love|i like|i want|my|for me|for my)\b.*\b(what|which|how|should|recommend|suggest|ideas?|help)\b/i;
  const QTY = /(\$?\d[\d,]*(\.\d+)?%?)|\b(one|two|three|four|five|six|seven|eight|nine|ten|eleven|twelve|twenty|thirty|forty|fifty|hundred|thousand|half|twice|double|triple|quarter|dozen)\b/gi;
  const COMBINE = /\b(how (many|much|long|far|old|fast)|in total|total|altogether|left|remain|remaining|each|per|every|times|more than|less than|fewer|twice|half|average|profit|cost|costs|spend|spent|earn|earns|save|saves|percent|%|calculate|compute|how much money)\b/i;
  const SCIENCE = /\b(atom|atoms|molecule|cell|cells|energy|force|mass|gravity|electron|proton|neutron|chemical|chemistry|physics|biology|organism|species|dna|gene|genes|protein|enzyme|acid|base|ion|bond|reaction|evolution|photosynthesis|planet|star|solar|orbit|temperature|heat|pressure|volume|density|velocity|acceleration|wave|light|magnet|magnetic|electric|electricity|circuit|nucleus|isotope|element|compound|equation|calculate|math|algebra|geometry|probability|theorem|formula|quantum|climate|mineral|rock|fossil|virus|bacteria|tissue|organ|blood|nerve|hormone|metabolism|ecosystem|carbon|oxygen|hydrogen|nitrogen|water cycle|scientist|experiment)\b|\d/i;
  const PEOPLE_MEDIA = /\b(who|actor|actress|singer|band|album|song|movie|film|show|tv|series|celebrity|famous|star|president|king|queen|player|team|novel|author|book|character|music|artist|rapper|director|married|wife|husband|born|died|played|sing|sang|wrote)\b/i;

  const sentences = (q) => q.split(/(?<=[.!?])\s+(?=[A-Z0-9$])/).filter((s) => s.trim().length > 3).length;

  function route(q) {
    const why = [];
    let score = 0;
    const cmd = q.match(CMD), art = q.match(ARTEFACT), stance = q.match(STANCE), tailor = q.match(TAILOR);
    if (cmd && (art || !/^\s*(who|what|when|where|which|whose|whom)\b/i.test(q))) { score += 2; why.push(`asks for something new ("${cmd[0].trim()}")`); }
    else if (stance) { score += 2; why.push(`asks for a stance or argument nobody wrote yet ("${stance[0]}")`); }
    else if (tailor) { score += 2; why.push("needs an answer built for this person's situation"); }
    const qty = (q.match(QTY) || []).length;
    const combine = q.match(COMBINE);
    if (qty >= 2 && combine) { score += 2; why.push(`${qty} quantities to combine in order ("${combine[0]}")`); }
    const n = sentences(q);
    if (n >= 3) { score += 1; why.push(`${n} sentences that build on each other`); }
    const words = q.trim().split(/\s+/).length;
    if (score === 0) why.push(words <= 20 ? "a short question about existing facts" : "asks for existing material");
    const big = score >= 2;
    const topic = SCIENCE.test(q) ? "science" : "people & media";
    return {
      model: big ? "big" : "small",
      kind: big ? "non-abelian" : "abelian",
      need: big ? "novelty / ordered steps" : "existing material",
      topic, score, why,
    };
  }

  // The baseline a topic router would use: science goes big, everything else small.
  function routeByTopic(q) {
    const topic = SCIENCE.test(q) ? "science" : "people & media";
    return { model: topic === "science" ? "big" : "small", topic };
  }

  // Prices per million tokens (Anthropic first-party list prices, Nov 2026).
  const PRICES = {
    big: { name: "Claude Opus 5.5", in: 4.0, out: 20.0 },
    small: { name: "Claude Haiku 5.5", in: 0.1, out: 0.5 },
  };
  const cost = (model, inTok, outTok) => (inTok * PRICES[model].in + outTok * PRICES[model].out) / 1e6;

  const api = { route, routeByTopic, PRICES, cost };
  if (typeof module !== "undefined") module.exports = api; else root.Router = api;
})(this);
