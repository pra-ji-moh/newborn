"""Check an answer key against the documents themselves: read every fact from
each prompt's text (every wording the generators use, the latest move
winning), answer the questions by following the facts, and compare with
key.txt. Run: python3 verify_key.py PROMPT_DIR
"""
import re
import sys

d = sys.argv[1]
# (pattern, relation, reversed?): reversed means the object comes first
PATTERNS = [
    (r"(\S+) is liked by (\S+)", "likes", True), (r"(\S+) is adored by (\S+)", "likes", True),
    (r"(\S+) is respected by (\S+)", "likes", True), (r"(\S+) likes (\S+)", "likes", False),
    (r"(\S+) is a fan of (\S+)", "likes", False), (r"(\S+) admires (\S+)", "likes", False),
    (r"(\S+) adores (\S+)", "likes", False), (r"(\S+) respects (\S+)", "likes", False),
    (r"(\S+) moved to (\S+)", "home", False), (r"(\S+) lives in (\S+)", "home", False),
    (r"(\S+) has a home in (\S+)", "home", False), (r"(\S+) calls (\S+) home", "home", False),
    (r"(\S+) resides in (\S+)", "home", False), (r"(\S+) is where (\S+) lives", "home", True),
    (r"in (\S+) lives (\S+)", "home", True), (r"(\S+) is home to (\S+)", "home", True),
    (r"(\S+) is the home of (\S+)", "home", True),
    (r"(\S+) is made up of (\S+)", "in", True), (r"(\S+) includes (\S+)", "in", True),
    (r"(\S+) contains (\S+)", "in", True), (r"(\S+) holds (\S+)", "in", True),
    (r"(\S+) is located in (\S+)", "in", False), (r"(\S+) is part of (\S+)", "in", False),
    (r"(\S+) belongs to (\S+)", "in", False), (r"(\S+) lies in (\S+)", "in", False),
    (r"(\S+) is in (\S+)", "in", False),
]
key = {l.split()[0]: l.split()[1:] for l in open(f"{d}/key.txt")}
bad = 0
for name, answers in key.items():
    text = open(f"{d}/prompt_{name}.txt").read()
    doc, qs = text.split("DOCUMENT:\n")[1].split("\n\nQUESTIONS:\n")
    likes, home, inside = {}, {}, {}
    for s in doc.split(" . "):
        s = s.strip().rstrip(" .")
        if not re.search(r"\b(person|city|country|continent)\d", s):
            continue
        for pat, rel, rev in PATTERNS:
            m = re.fullmatch(pat, s)
            if m:
                a, b = (m.group(2), m.group(1)) if rev else (m.group(1), m.group(2))
                {"likes": likes, "home": home, "in": inside}[rel][a if rel != "likes" else b] = b if rel != "likes" else a
                break
        else:
            print(f"{name}: no wording matched: {s!r}")
    for i, (q, ans) in enumerate(zip(qs.strip().splitlines(), answers), 1):
        q = q.split(". ", 1)[1]
        if q.startswith("Who likes"):
            mine = likes[q.split()[2].rstrip("?")]
        else:
            p = re.search(r"person\d+", q).group(0)
            mine = home[p]
            if "country" in q or "continent" in q:
                mine = inside[mine]
            if "continent" in q:
                mine = inside[mine]
        if mine != ans:
            bad += 1
            print(f"{name} question {i}: key says {ans}, the document says {mine}")
print(f"{len(key)} documents, {sum(len(a) for a in key.values())} answers checked, {bad} disagreements")
