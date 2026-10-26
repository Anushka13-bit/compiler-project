# resumec

A compiler, written in C, that turns a small resume description language into a formatted PDF.

You write a `.resume` file. The compiler lexes it, parses it into an AST, checks semantic rules, emits LaTeX, and runs `pdflatex`. The point of the project is that compiler pipeline — not a GUI resume builder.

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
make
./bin/resumec examples/ananya.resume -o ananya.pdf
```

## What we use to build it

| Tool | Role |
|---|---|
| **C11 compiler** (`gcc` or `clang`) | Compiles `src/*.c` plus the C that flex/bison generate |
| **flex** | Generates the lexer from `src/lexer.l` → `build/lexer.c` |
| **bison** | Generates the parser from `src/parser.y` → `build/parser.tab.c` and `parser.tab.h` |
| **make** | Runs those tools in order and links `bin/resumec` |
| **pdflatex** | Turns the generated `.tex` into a PDF (not needed for `--tex-only`) |

macOS ships bison 2.3, which cannot parse `%define parse.error verbose`. The Makefile prefers Homebrew bison if it exists:

- `/opt/homebrew/opt/bison/bin/bison` (Apple Silicon)
- `/usr/local/opt/bison/bin/bison` (Intel Homebrew)

```bash
brew install flex bison          # scanner/parser generators
brew install --cask basictex     # pdflatex (optional, for PDF output)
make                             # bin/resumec
make test                        # golden .tex + semantic error cases
```

On Linux, distro `flex`, `bison`, `gcc`, and `texlive-latex-base` are enough.

Generated files live under `build/` (flex/bison C and object files) and `bin/` (the compiler). Both are gitignored.

## Idea

A resume is a regular structure: name, optional email, education rows, a skill list, projects. That maps cleanly onto a grammar. Using a compiler instead of string templates means:

- illegal files are rejected with line/column diagnostics
- several errors can be reported in one run
- user text is escaped before it is written into LaTeX
- the PDF layout is one code-generation pass, not hand-edited TeX

Declarations may appear in any order. A missing `name` is therefore a **semantic** error (same family as bad CGPA or duplicate skills), not a parse failure. Duplicate `name` / `email` is caught in the parser while the tree is being built.

The full token list and EBNF are in [`GRAMMAR.md`](GRAMMAR.md).

## How the compiler works

```
.resume  →  lexer  →  parser  →  AST  →  semantic checks  →  .tex  →  pdflatex  →  .pdf
              flex      bison    ast.c     semantic.c        codegen.c    build.c
```

`src/main.c` is the only place that sequences these steps.

1. **CLI.** Flags: `-o` output PDF, `--keep-tex`, `--tex-only`, `--cgpa-scale 10|4`, `--dump-ast`. Default PDF path is the input basename with `.pdf`.
2. **Lex + parse.** `yyin` is the input file. `resume_new()` allocates an empty `Resume`. `yyparse()` pulls tokens from `yylex()` and runs grammar actions that fill `g_resume`.
3. **`--dump-ast`.** Prints the tree and exits. Used to inspect lex/parse without semantic analysis or codegen.
4. **Semantic analysis.** Walks the AST. If any error was recorded, diagnostics are printed (sorted by line, then column) and the process exits `1` with no `.tex`.
5. **Code generation.** Writes a self-contained `article` document: preamble, centered header, then Education / Skills / Projects only if those lists are non-empty. `& % $ # _ { } ~ ^ \` in user strings are escaped.
6. **PDF build.** Unless `--tex-only`, `build.c` forks `pdflatex -interaction=nonstopmode -halt-on-error`, moves the PDF to `-o`, and deletes `.aux` / `.log` / `.out` (and the `.tex` unless `--keep-tex`). If `pdflatex` is missing, the `.tex` is still on disk and the error says so.

Diagnostics from lexer, parser, and semantic analysis all go through `src/diagnostics.c`. They look like:

```
error [line 4, col 1]: CGPA 11.5 out of valid range (0-10)
error [line 9, col 3]: duplicate skill "Python"
```

## Language rules

| Construct | Form | Rules |
|---|---|---|
| Name | `name "..."` | Required exactly once |
| Email | `email "..."` | Optional, at most once; must look like `local@host.tld` |
| Education | `education` then `college "..." cgpa N` | CGPA in `[0, 10]` (or `[0, 4]` with `--cgpa-scale 4`) |
| Skills | `skills` then identifiers | `C++`, `Python`, `Node.js`; no case-insensitive duplicates |
| Projects | `projects` then `project "..."` | Optional `description "..."` |
| Comments | `# ...` | Rest of line ignored |

## Codebase

```
src/lexer.l          flex rules: keywords, STRING, NUMBER, IDENTIFIER, comments, line/col
src/parser.y         bison grammar; actions build the AST; error token for recovery
src/ast.h, ast.c     Resume tree (name, email, edu entries, skills, projects)
src/semantic.c       name required, email regex, CGPA range, duplicate skills
src/codegen.c        AST → complete .tex (Jake's-resume-style single column)
src/build.c          pdflatex subprocess + cleanup
src/diagnostics.c    collect / sort / print errors
src/main.c           CLI and phase order
Makefile             flex → bison → cc → bin/resumec
tests/               golden sample1.tex, --dump-ast check, four negative cases, PDF smoke
examples/            sample .resume file
GRAMMAR.md           tokens + EBNF
```

**Lexer.** Keywords are longest-match reserved words. Strings support `\"` and `\\`. An unterminated quote is an error but still yields a `STRING` so the parse can continue. `YY_USER_ACTION` fills bison locations so later errors can point at a column.

**Parser.** `resume := decl*`. Each section list is one-or-more entries. `yyerrok` + `yyclearin` on `error` alternatives skip a bad construct instead of aborting the whole file.

**AST.** Dynamic arrays of `EduEntry`, `Skill`, and `Project`, each storing source line/col for diagnostics. Codegen reads the same tree (there is no separate IR copy).

**Semantic analyzer.** Name non-empty; email optional POSIX regex `^[^@space]+@[^@space]+\.[^@space]+$`; each CGPA in range; each skill compared to earlier skills with `strcasecmp`.

**Codegen.** Depends only on the AST and `stdio`. Preamble uses `article` + `geometry` so a minimal TeX install (BasicTeX) is enough — no `enumitem` / `titlesec`.

**Build.** Tries `pdflatex` on `PATH`, then a few macOS TeX paths. On failure it prints the first `! ` line from the log rather than the whole log.

## Usage

```
./bin/resumec [<input.resume> | -] [-o <output.pdf>] [options]
./bin/resumec -i | --interactive
```

```bash
# Interactive wizard mode (prompts for name, contact, education, skills, projects, experience)
./bin/resumec -i

# Interactive wizard tailored to a specific job posting (URL or file)
./bin/resumec -i --job-url "https://jobs.example.com/backend-engineer"
./bin/resumec -i --job-desc job_requirements.txt

# Verify an existing resume against target job requirements during compilation
./bin/resumec examples/ananya.resume --job-url "https://jobs.example.com/backend-engineer"
./bin/resumec examples/ananya.resume --job-desc tests/fixtures/sample_job.txt -o tailored_resume.pdf

# Standard compilation with specified files
./bin/resumec examples/ananya.resume -o ananya.pdf
./bin/resumec examples/ananya.resume --tex-only -o ananya.pdf
./bin/resumec examples/ananya.resume --dump-ast
./bin/resumec examples/ananya.resume --cgpa-scale 4 -o ananya.pdf
```

## Job Tailoring & Requirements Matcher

`resumec` can scrape and analyze real job postings to tailor your resume:

- **Web Scraping:** Pass any job posting URL (`--job-url <url>`). The scraper fetches the posting via `curl`, strips HTML/script tags, and extracts technical requirements.
- **Skill Extraction & Catalog:** Matches against a comprehensive catalog of 50+ languages, frameworks, cloud tools, databases, and engineering practices (with aliases like K8s, Postgres, AWS, ML).
- **Gap Analysis & Compatibility Report:** Calculates a match percentage, highlights matched skills, and pinpoints missing requirements.
- **Interactive Assistance:** In wizard mode (`-i`), it recommends missing skills while you type and allows one-click adding of missing requirements to optimize ATS scores.

## Upload Existing Resume PDF & Incremental Tailoring

Already have a resume PDF? You don't need to retype everything:

```bash
# Upload existing resume PDF and tailor against target job posting
./bin/resumec --resume-pdf my_resume.pdf --job-url "https://jobs.example.com/posting"

# Or tailor against a local job description file
./bin/resumec --resume-pdf my_resume.pdf --job-desc target_job.txt
```

1. **Extracts Existing Data:** Parses candidate name, contact links, education, existing skills, projects, and work experience directly from your PDF.
2. **Pinpoints Exact Gaps:** Shows what skills are already covered vs. missing for that specific job posting.
3. **Targeted Prompts Only:** Prompts *only* for the missing requirements (e.g., adding missing skills or tailored project bullets).
4. **Preserves Everything Else:** All existing contact information, education, and previous experiences remain 100% intact.
5. **Generates Tailored Outputs:** Saves `<name>_tailored.resume` and compiles `<name>_tailored.pdf`.

