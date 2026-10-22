# Project Gotham Racing (original Xbox) - static recompilation

Goal: a native Windows port built with the XboxXBE-Recomp toolkit in `sdk/` (a git submodule on the fork's `main`).
Read `sdk/README.md`, `sdk/harness/README.md`, `PROGRESS.md` and `FINDINGS.md` at the start of every session.

## Environment
- Headless Linux VM: build the Linux tools, regenerate code, run the game in the headless harness. The real Windows
  exe is built and play-tested on Windows separately.
- Game files (your own disc) are in `assets/`. Never commit, upload or paste them, or generated code.
  `assets/`, `generated/`, `build/`, `.local/` and `captures/` are git-ignored; keep it that way.
- Signatures in `sdk/xdk/*/signatures.toml` are hashes and lengths only. Never store library bytes.

## Commands
`scripts/*.sh` are wrappers for the shared scripts in `sdk/scripts`; this game's values are in `scripts/game.env`.
- `scripts/linux-tools.sh`   configure for Linux, build `xbe_symbols` and `xbe_recomp`
- `build/linux/xbe_symbols assets/default.xbe sdk/xdk --out config/game.toml`   library versions, XDK matches,
  rewrites config/game.toml (scan output only, never edited; hand entries go in a file that includes it)
- `scripts/recompile.sh`   regenerate `generated/game`
- `scripts/harness.sh [secs] [pad] [ENV=..]`   build + run headless (`REF=N` CPU frames into `captures/`, `KEEP_HDD=1`
  keeps the emulated disk, `snap`/`memdump`/`poke` files in the run folder)
- `scripts/smoke.sh`   title -> race headless; must print SMOKE OK before every fast-forward of `main`
- `scripts/wincheck.sh`   Windows-only sources against mingw headers; `scripts/shadercheck.sh`   generated HLSL with DXC
- `python3 scripts/gothpok.py <file> [out]` (or `--tree assets .local/unpacked`) unpacks the game's GOTHPOK files
- Ghidra: `python3 sdk/tools/symbols/xbe2pe.py <config> .local/game.exe`, import into `~/ghidra-projects/<GHIDRA_PROJECT>`,
  then `scripts/ghidra.sh Decomp|Disasm|FindOperand|Callers|DumpRange|PushBufferScan|DeviceRefs ...`

## Plan
1. Library versions; a new `sdk/xdk/<build>/` if needed (signatures, `layout.toml` with each offset cited to an
   instruction of this game).
2. Check push buffer use (CreatePushBuffer / RunPushBuffer / hand-written commands) and report it at once.
3. Boot: resolve trapping stubs one at a time. Then menus, a race (or the game's equivalent), smoke test.
4. Windows parity, then PC features (game hooks), achievements, the launcher and a release.

## Working rules
- One problem at a time. After each meaningful step, a dated entry at the top of PROGRESS.md's log (what changed,
  evidence, next step), its Status block updated, and a commit.
- Game fixes go in `src/game` (X86_FUNC overrides, [[hook]]/[[function]]/[[switch]] in the hand config), not in
  generated code. SDK changes go in `sdk/` as their own commits: a task branch of the fork, fast-forwarded into its
  `main`, keeping every game's smoke test passing.
- Game code registers from `GameInfo::init`, never a static initializer (one never ran in a Windows exe).
- If the same problem survives 3 genuinely different attempts, stop: write up what was tried in PROGRESS.md, commit,
  and say so.
- One branch per task; when it ends and everything builds, fast-forward `main` (`git merge --ff-only`), push both.
- Never force-push. Never rewrite history.
