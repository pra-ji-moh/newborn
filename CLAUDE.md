# Notes for Claude

- The author's research is in `research/RESEARCH.md`. It is the source of
  truth for the newborn's brain. Read it before any brain work.
- Build from the research at full fidelity. Don't reduce a named mechanism to
  a simplified stand-in. If something can't be built as described yet, say so
  plainly rather than shipping a sketch.
- The goal is to compete with the frontier models across the board. The plan
  and its tests are in `PLAN.md`. Report results honestly, losses included.
- The original PDF is `research/newborn.pdf`.

## Layout

Everything was moved here from the Ciall repo (`pra-ji-moh/ciall`, commit
81af06e, branch `claude/previous-ai-model-task-7f2yel`) on Oct 23, 2026.

- `brain/`: the grown brain (`ob_grow.c`, `ob_womb.c`, `ob_one.c`,
  `ob_frontal.c`), the life brain (`ob_life.c`), their tests. Build a test:
  `cc -std=c99 -O2 ob_grow.c ob_view.c ob_womb.c ob_life.c ob_one.c ob_crib.c ob_box.c ob_frontal.c test_X.c -o test_X -lm`
  (`test_frontal`, `test_one`, `test_cortex`, `test_novel` take many minutes.)
- `brain/ob_novel.c`: ideas nobody writes (MFG, PPA, DMN, Broca + FEF
  hyper-direct; DLPFC holds, ACC checks against remembered results).
  `test_novel SNAPSHOT [q]` keeps the three-year life on file to rerun the
  boxes in two minutes; a life loaded from file needs `ob_one_resume`.
- No reward and no punishment anywhere new: dopamine and oxytocin for
  success (learning follows their bursts above the steady level), cortisol
  for failure (`ob_one.c` step 3). `ob_life.c` (the sketch) and the womb's
  mobile still use the old critic.
- `mind/`: baby Ciall's cognition (`smarsh_*`). Build a test:
  `cc -std=c99 -O2 smarsh_*.c arc_worldgen.c arc_standin.c test_X.c -o test_X -lm`
- `phone/`: builds the page (`build_phone.sh`, needs clang with wasm32);
  `index.html` at the root is the built page.
- `bench/`: the hidden-rule benchmark against frontier models.
- `hartley/`: the Hartley Non-Abelian Model's first test (ordered
  elimination over hidden S5 acts). `cc -std=c99 -O2 hnam.c -o hnam -lm`,
  then `./hnam 50 0` or `./hnam 50 1`. Hartley means the 1928 measure
  (research Part 4).
- `research/ciall/`: design notes from the earlier chats (`ONE_BRAIN.md`,
  `BEING.md`, `ARCHITECTURE.md`).
