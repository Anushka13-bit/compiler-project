# resumec -- a DSL-to-PDF resume compiler

A small compiler, written in C with flex and bison, that translates a
custom resume-description DSL into a professionally formatted PDF,
following the standard compiler phases: lexical analysis, parsing,
semantic analysis, IR generation, and code generation (to LaTeX), with
`pdflatex` invoked as a separate build step to turn that LaTeX into a
PDF.

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

```
./bin/resumec examples/ananya.resume -o ananya.pdf
```

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

## Architecture

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

## Building

Requires `flex`, `bison`, and a C11 compiler. macOS ships a very old,
GPL-frozen bison (2.3) that doesn't understand this project's grammar
directives; the `Makefile` automatically prefers a Homebrew-installed
bison (`brew install bison`) if one is present at
`/opt/homebrew/opt/bison/bin/bison`, falling back to whatever `bison`
resolves to on `PATH` otherwise.

```bash
make          # builds bin/resumec
make clean    # removes build/ and bin/
make test     # builds, then runs tests/run_tests.sh
```

Turning the generated `.tex` into a PDF needs `pdflatex` on your
`PATH` (or at one of a few common install locations `build.c` checks).
Any TeX distribution works, e.g.:

```bash
brew install --cask basictex
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

## Folder structure

```
resume-dsl-compiler/
├── Makefile
├── GRAMMAR.md
├── README.md
├── src/
│   ├── lexer.l          flex lexer
│   ├── parser.y          bison grammar + AST-building actions
│   ├── ast.h / ast.c      AST node types + debug printer
│   ├── semantic.h/.c      semantic rules, batched diagnostics
│   ├── ir.h / ir.c        AST -> IR lowering
│   ├── codegen.h/.c       IR -> LaTeX, with escaping
│   ├── build.h/.c         pdflatex orchestration (outside the compiler core)
│   ├── diagnostics.h/.c   shared diagnostic collection/printing
│   └── main.c             CLI, wires the six phases together
├── tests/
│   ├── run_tests.sh
│   ├── invalid_cgpa.resume
│   ├── invalid_email.resume
│   ├── duplicate_skill.resume
│   ├── missing_name.resume
│   └── golden/
│       ├── sample1.resume
│       └── sample1.tex
├── examples/
│   └── ananya.resume
├── build/    (generated: flex/bison output + object files)
└── bin/      (generated: the resumec binary)
```
