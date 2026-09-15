/*
 * Code generator (final compiler phase).
 *
 * Maps the IR onto LaTeX source text, in a single-column style loosely
 * based on the well-known "Jake's Resume" layout. Output is a complete,
 * self-contained, compilable .tex file -- that file IS the compiler's
 * deliverable; turning it into a PDF is build.c's job, not this
 * module's.
 */
#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdbool.h>

#include "ir.h"

/* Writes a complete .tex document for `ir` to `tex_path`.
 * Returns true on success, false if the file could not be written. */
bool codegen_write_tex(const ResumeIR *ir, const char *tex_path);

#endif /* CODEGEN_H */
