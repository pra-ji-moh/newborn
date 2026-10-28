"""Why a chat model's answers in bench/gpt_test went wrong: for every
question, which kinds of fact the answer depends on - a fact worded in
reverse (passive "Q is liked by P", "C is where P lives", "K includes C",
"X contains K"), a move ("P moved to C") - and how often the replies were
right in each case. Run: python3 gpt_errors.py bench/gpt_test
"""
import collections
import re
import sys

out = sys.argv[1]
REVERSED = [r"(\S+) is liked by (\S+)", r"(\S+) is where (\S+) lives", r"in (\S+) lives (\S+)",
            r"(\S+) includes (\S+)", r"(\S+) contains (\S+)", r"(\S+) is home to (\S+)", r"(\S+) holds (\S+)"]
FORWARD = [r"(\S+) likes (\S+)", r"(\S+) is a fan of (\S+)", r"(\S+) admires (\S+)", r"(\S+) lives in (\S+)",
           r"(\S+) has a home in (\S+)", r"(\S+) calls (\S+) home", r"(\S+) is in (\S+)", r"(\S+) belongs to (\S+)",
           r"(\S+) is part of (\S+)", r"(\S+) lies in (\S+)", r"(\S+) moved to (\S+)"]

tally = collections.defaultdict(lambda: [0, 0])
key = {l.split()[0]: l.split()[1:] for l in open(f"{out}/key.txt")}
for name, answers in key.items():
    try:
        reply = {int(m.group(1)): m.group(2).lower() for m in re.finditer(r"^\s*(\d+)[.):]\s*(\S+)", open(f"{out}/reply_{name}.txt").read(), re.M)}
    except FileNotFoundError:
        continue
    text = open(f"{out}/prompt_{name}.txt").read()
    doc, qs = text.split("DOCUMENT:\n")[1].split("\n\nQUESTIONS:\n")
    # facts as (subject, object, reversed?) along each relation: who-likes, home, land, cont
    likes, home, moved, land, cont = {}, {}, set(), {}, {}
    for s in doc.split(" . "):
        s = s.strip()
        if not re.search(r"person|city|country|continent", s):
            continue
        for pat in REVERSED:
            m = re.fullmatch(pat, s)
            if m:
                b, a = m.group(1), m.group(2)          # reversed: object first
                break
        else:
            for pat in FORWARD:
                m = re.fullmatch(pat, s)
                if m:
                    a, b = m.group(1), m.group(2)
                    break
            else:
                continue
        rev = any(re.fullmatch(p, s) for p in REVERSED)
        if "likes" in s or "liked" in s or "fan" in s or "admires" in s:
            likes[b] = (a, rev)
        elif a.startswith("person"):
            if "moved" in s:
                moved.add(a)
            home[a] = (b, rev)
        elif a.startswith("city"):
            land[a] = (b, rev)
        elif a.startswith("country"):
            cont[a] = (b, rev)
    for i, (q, ans) in enumerate(zip(qs.strip().splitlines(), answers), 1):
        q = q.split(". ", 1)[1]
        ok = reply.get(i) == ans
        if q.startswith("Who likes"):
            target = q.split()[2].rstrip("?")
            kind = "who-likes, passive ('is liked by')" if likes[target][1] else "who-likes, active"
            tally[kind][0] += ok; tally[kind][1] += 1
            continue
        p = re.search(r"person\d+", q).group(0)
        steps = [home[p]]
        if "country" in q or "continent" in q:
            steps.append(land[steps[0][0]])
        if "continent" in q:
            steps.append(cont[steps[1][0]])
        tag = ("one step" if len(steps) == 1 else f"{len(steps)} steps")
        conds = []
        if p in moved:
            conds.append("after a move")
        if any(r for _, r in steps):
            conds.append("a reversed fact")
        kind = tag + (", " + " and ".join(conds) if conds else ", plain")
        tally[kind][0] += ok; tally[kind][1] += 1
        tally["any question with a reversed fact" if any(r for _, r in steps) else "any question, no reversed fact"][0] += ok
        tally["any question with a reversed fact" if any(r for _, r in steps) else "any question, no reversed fact"][1] += 1
for k, (r, n) in sorted(tally.items()):
    print(f"{k:45s} {r:3d}/{n:<3d} {100 * r / n:5.1f}%")
