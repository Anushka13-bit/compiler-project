/*
 * Build orchestrator -- explicitly OUTSIDE the compiler's core logic.
 *
 * codegen.c's job ends at producing a valid .tex file; this module is
 * a thin wrapper that shells out to `pdflatex` to turn that file into
 * a PDF, and tidies up afterward. None of the compiler's phases
 * (lexer/parser/semantic/ir/codegen) call into this module or know it
 * exists -- only main.c does, and only when the user didn't pass
 * --tex-only.
 */
#ifndef BUILD_H
#define BUILD_H

#include <stdbool.h>

typedef struct BuildOptions {
    const char *tex_path;   /* input .tex file, already written by codegen */
    const char *output_pdf; /* where the final PDF should end up (-o) */
    bool keep_tex;           /* if false, the .tex is deleted after a successful build */
} BuildOptions;

/* Runs pdflatex on opts->tex_path, moves the resulting PDF to
 * opts->output_pdf, and removes the .aux/.log/.out auxiliary files
 * (and the .tex itself, unless keep_tex is set). On failure an
 * explanatory message has already been printed to stderr -- either
 * "pdflatex not found" or the first "! ..." error line pulled out of
 * pdflatex's own log. Returns true on success. */
bool build_pdf(const BuildOptions *opts);

#endif /* BUILD_H */
