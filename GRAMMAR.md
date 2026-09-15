# Resume DSL grammar

## Tokens (recognized by `src/lexer.l`)

| Token           | Matches                                                              |
|-----------------|-----------------------------------------------------------------------|
| Keywords        | `name` `email` `education` `college` `cgpa` `skills` `projects` `project` `description` |
| `STRING`        | double-quoted text, e.g. `"Ananya Mehta"`, with `\"` and `\\` escapes |
| `NUMBER`        | an integer or decimal, e.g. `9.3`, `-1`                               |
| `IDENTIFIER`    | a bare word starting with a letter, e.g. `Python`, `C++`, `Node.js`   |
| (comment)       | `#` to end of line -- discarded, not a token                          |

## EBNF

```ebnf
resume           := decl*
decl              := name_decl | email_decl | section
name_decl        := "name" STRING
email_decl       := "email" STRING
section          := education_sec | skills_sec | projects_sec
education_sec    := "education" education_entry+
education_entry  := "college" STRING "cgpa" NUMBER
skills_sec       := "skills" IDENTIFIER+
projects_sec     := "projects" project_entry+
project_entry    := "project" STRING description_decl?
description_decl := "description" STRING
```

This is implemented directly as the bison grammar in `src/parser.y`
(see that file for the exact rule names and actions).

## One deliberate refinement over the original starting-point grammar

The brief's starting EBNF was `resume := name_decl email_decl? section*`,
which forces `name` to be the very first thing in the file. This
grammar instead allows `name`, `email`, and the three sections in any
order and any repetition of the ones that make sense (`decl*`).

That is what lets a file with **no** `name` declaration at all still
parse successfully, so "a resume must have a name" can be caught by
`semantic.c` as a *semantic* diagnostic, exactly like the CGPA-range,
email-format, and duplicate-skill rules -- instead of being a syntax
error that aborts parsing before semantic analysis (and the other three
phases after it) ever runs. This matches the project's own negative-test
list, which expects "missing name" to behave like the other three
semantic violations, not like a malformed file.

(`name`/`email` declared twice, and `education`/`skills`/`projects`
sections declared twice, are still rejected -- as duplicate-declaration
errors raised directly from the parser actions in `parser.y`, since
that's purely a structural/grammar-level property of the file, not
something that needs a semantic walk over the finished tree.)

## Error recovery

The parser uses bison's standard `error` token at four synchronization
points: the top-level `decl_list`, and each of the three entry lists
(`education_entry`, `skill_item`, `project_entry`). This lets one
malformed construct get skipped without aborting the whole parse, so a
single run can report several unrelated syntax errors instead of
stopping at the first one -- see the comment block at the top of
`src/parser.y` for the mechanics (and a subtle but important detail:
every recovery rule pairs `yyerrok` with `yyclearin`, since without
`yyclearin` the reduction can fire repeatedly on the same un-consumed
lookahead token and never actually advance -- an easy way to accidentally
write an infinite loop into a yacc/bison error rule).
