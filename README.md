# resumec -- a DSL-to-PDF resume compiler

A small compiler, written in C with flex and bison, that translates a
custom resume-description DSL into a professionally formatted PDF. It
follows the standard compiler phases -- lexical analysis, parsing,
semantic analysis, IR generation, and code generation (to LaTeX) --
with `pdflatex` invoked as a separate build step to turn that LaTeX
into a PDF.

```
name "Ananya Mehta"
email "ananya.mehta@example.com"

education
college "VIT"
cgpa 9.3

skills
C++
Python
Java

projects
project "Resume Compiler"
description "DSL-to-PDF resume generator built with flex and bison"
```

```bash
./bin/resumec examples/ananya.resume -o ananya.pdf
```

## Quick start

```bash
git clone https://github.com/Anushka13-bit/compiler-project.git
cd compiler-project
make            # builds bin/resumec
make test       # runs the test suite
./bin/resumec examples/ananya.resume -o ananya.pdf
```

See [Building](#building) below if `make` fails -- the only real
prerequisite beyond a C compiler is a reasonably modern `bison`.

## The DSL

A `.resume` file is a sequence of declarations, in any order:

- `name "..."` -- required exactly once.
- `email "..."` -- optional, at most once. Must look like an email address.
- `education` followed by one or more `college "..." cgpa N` entries.
  CGPA must fall within `[0, 10]` by default, or `[0, 4]` with
  `--cgpa-scale 4`.
- `skills` followed by one or more bare identifiers (`C++`, `Python`,
  `Node.js`, ...). No two skills may be the same, case-insensitively.
- `projects` followed by one or more `project "..."` entries, each with
  an optional `description "..."`.
- `# comment text` -- a line comment, runs to end of line.

The full grammar is in [`GRAMMAR.md`](GRAMMAR.md), including the one
deliberate way this project's grammar differs from a strict reading of
`name_decl email_decl? section*`: `name` and `email` may appear
anywhere among the sections, not just first, which is what lets a file
with no `name` at all still be *parsed* successfully so that "a resume
needs a name" is reported as a semantic diagnostic alongside the CGPA/
email/duplicate-skill checks, instead of as a hard syntax error.

## How the pipeline fits together

Six phases, each its own translation unit under `src/`:

| Phase | File(s) | Input -> Output |
|---|---|---|
| 1. Lexical analysis | `lexer.l` (flex) | source text -> token stream |
| 2. Parsing | `parser.y` (bison) | tokens -> `Resume` AST (`ast.h`) |
| 3. Semantic analysis | `semantic.c` | AST -> diagnostics (name/CGPA/email/duplicate-skill checks) |
| 4. IR generation | `ir.c` | validated AST -> `ResumeIR` (`ir.h`) |
| 5. Code generation | `codegen.c` | IR -> a complete `.tex` file |
| 6. Build (outside the compiler core) | `build.c` | `.tex` -> `pdflatex` subprocess -> `.pdf` |

`diagnostics.c` is shared by phases 1-3: the lexer, parser, and
semantic analyzer all *record* problems there rather than printing or
aborting immediately, so a single run can report several unrelated
errors. `main.c` checks for error-level diagnostics after semantic
analysis and halts before IR generation if any exist.

`codegen.c` depends only on `ir.h`, never on `ast.h` -- so IR lowering
is a real, separate step, not just parser output being handed straight
to code generation under a different name. `build.c` is explicitly
outside that pipeline: nothing in phases 1-5 calls it or knows it
exists; only `main.c` invokes it, and only when `--tex-only` wasn't
passed.

User-provided text (names, project descriptions, skill names, ...) is
escaped for LaTeX's special characters (`& % $ # _ { } ~ ^ \`) in
`codegen.c` before being written out, so input like `C++ & Java` can't
break the generated document.

## File-by-file guide

```
resume-dsl-compiler/
├── Makefile                  build rules: flex + bison + gcc -> bin/resumec
├── GRAMMAR.md                 the full EBNF grammar, plus the rationale for
│                              the one deliberate deviation from the spec
├── README.md                  this file
├── .gitignore                 ignores build/, bin/, and generated .pdf/.tex/.aux/.log/.out
│
├── src/
│   ├── lexer.l                flex lexer: keywords, STRING/NUMBER/IDENTIFIER
│   │                          tokens, comments, per-token line/col tracking
│   ├── parser.y                bison grammar; each rule's action builds one
│   │                          piece of the AST. Also where syntax-error
│   │                          recovery (the `error` token) lives
│   ├── ast.h / ast.c           the `Resume` AST struct (name, email, education
│   │                          entries, skills, projects) + resume_debug_print(),
│   │                          used by `--dump-ast`
│   ├── semantic.h / semantic.c walks the AST enforcing: name required, CGPA
│   │                          in range, email format (regex), no duplicate
│   │                          skills (case-insensitive) -- reports every
│   │                          violation instead of stopping at the first
│   ├── ir.h / ir.c             lowers a validated AST into `ResumeIR`, a
│   │                          simpler struct with no source-position fields;
│   │                          codegen.c depends only on this, never on ast.h
│   ├── codegen.h / codegen.c   writes the IR out as a complete .tex file
│   │                          (a "Jake's Resume"-style single-column layout),
│   │                          escaping LaTeX special characters as it goes
│   ├── build.h / build.c       runs `pdflatex` as a subprocess, parses its
│   │                          log for the first "! " error line on failure,
│   │                          and cleans up .aux/.log/.out afterward. This
│   │                          is the one file that is *not* part of the
│   │                          compiler proper -- it's a build step
│   ├── diagnostics.h / diagnostics.c  the shared error/warning collector used
│   │                          by the lexer, parser, and semantic analyzer;
│   │                          prints everything sorted by line at the end
│   └── main.c                  CLI argument parsing and the glue that calls
│                              the six phases in order
│
├── tests/
│   ├── run_tests.sh            the test runner (see Testing below)
│   ├── invalid_cgpa.resume      negative test: CGPA out of range
│   ├── invalid_email.resume     negative test: malformed email
│   ├── duplicate_skill.resume   negative test: same skill listed twice
│   ├── missing_name.resume      negative test: no `name` declaration
│   └── golden/
│       ├── sample1.resume       fixed input for the golden-file test
│       └── sample1.tex          the exact .tex output sample1.resume must produce
│
├── examples/
│   └── ananya.resume            the sample resume from the project brief
│
├── build/    (generated by `make`, git-ignored: flex/bison output + .o files)
└── bin/      (generated by `make`, git-ignored: the resumec binary)
```

## Building

Requires `flex`, `bison`, and a C11 compiler. macOS ships a very old,
GPL-frozen bison (2.3) that doesn't understand this project's grammar
directives; the `Makefile` automatically prefers a Homebrew-installed
bison (`brew install bison`) if one is present at
`/opt/homebrew/opt/bison/bin/bison`, falling back to whatever `bison`
resolves to on `PATH` otherwise. On Linux, the distro's `bison` package
is almost always new enough already.

```bash
make          # builds bin/resumec
make clean    # removes build/ and bin/
make test     # builds, then runs tests/run_tests.sh
```

Turning the generated `.tex` into a PDF needs `pdflatex` on your
`PATH` (or at one of a few common install locations `build.c` checks).
Any TeX distribution works, e.g.:

```bash
brew install --cask basictex     # macOS
sudo apt install texlive-latex-base   # Debian/Ubuntu
```

If `pdflatex` isn't found, `resumec` still succeeds at producing the
`.tex` file and prints a clear error explaining what's missing, rather
than failing silently or crashing.

## Usage

```
./bin/resumec <input.resume> [-o <output.pdf>] [options]

  -o, --output <path>   output PDF path (default: <input basename>.pdf)
      --keep-tex         keep the generated .tex file
      --tex-only         stop after code generation; skip pdflatex
      --cgpa-scale <N>   valid CGPA scale, 10 or 4 (default: 10)
      --dump-ast         print the parsed AST and exit
  -h, --help             show usage
```

Examples:

```bash
# full pipeline: produces ananya.pdf
./bin/resumec examples/ananya.resume -o ananya.pdf

# just the LaTeX, e.g. to inspect or hand-edit before compiling
./bin/resumec examples/ananya.resume --tex-only -o ananya.pdf

# a 0-4 CGPA scale instead of the 0-10 default
./bin/resumec examples/ananya.resume --cgpa-scale 4 -o ananya.pdf

# inspect exactly what the lexer + parser produced, independent of
# semantic analysis / IR / codegen
./bin/resumec examples/ananya.resume --dump-ast
```

### Diagnostics

Every error from every phase is collected and printed together, sorted
by source line, in this format:

```
error [line 4, col 1]: CGPA 11.5 out of valid range (0-10)
error [line 9, col 3]: duplicate skill "Python"
```

If any error-level diagnostic survives semantic analysis, the compiler
halts before generating any LaTeX (exit code 1).

## Testing

```bash
make test
```

runs `tests/run_tests.sh`, which covers:

- **Golden-file test**: `--tex-only` output for `tests/golden/sample1.resume`
  diffed byte-for-byte against `tests/golden/sample1.tex`.
- **Parser isolation**: `--dump-ast` output checked against the expected
  AST shape, independent of semantic analysis/IR/codegen.
- **Negative tests**: one `.resume` file per semantic rule
  (`invalid_cgpa.resume`, `invalid_email.resume`, `duplicate_skill.resume`,
  `missing_name.resume`) -- each asserted to exit non-zero with the
  expected diagnostic substring.
- **PDF smoke test**: the full pipeline (no `--tex-only`) run on a valid
  sample, asserting the resulting PDF exists and is non-empty. This step
  is skipped (clearly labeled, not silently) on a machine with no
  `pdflatex` installed.

A passing run looks like:

```
== golden-file test ==
  PASS: generated .tex byte-for-byte matches tests/golden/sample1.tex

== lexer/parser isolation (--dump-ast) ==
  PASS: --dump-ast shows the expected AST shape, independent of semantic/IR/codegen

== negative tests (one per semantic rule) ==
  PASS: bad CGPA: exits non-zero and reports "out of valid range"
  PASS: malformed email: exits non-zero and reports "invalid email format"
  PASS: duplicate skill: exits non-zero and reports "duplicate skill"
  PASS: missing name: exits non-zero and reports "must have a 'name'"

== PDF smoke test ==
  SKIP: no pdflatex found on this machine -- install a LaTeX distribution
        (e.g. 'brew install --cask basictex') to exercise this test.

-------------------------------------------
passed: 6   failed: 0
```
