/*
 * Shared diagnostics module.
 *
 * The lexer, parser, and semantic analyzer all report problems through
 * this module instead of printing directly or aborting immediately.
 * That lets every phase batch up its findings so the compiler can show
 * several unrelated errors from one run instead of stopping at the
 * first one. main.c decides, after each phase, whether any error-level
 * diagnostic means the pipeline should halt before the next phase.
 */
#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <stdbool.h>

typedef enum { DIAG_ERROR, DIAG_WARNING } Severity;

typedef struct Diagnostic {
    Severity severity;
    char *message;
    int line;
    int col;
} Diagnostic;

/* Record a diagnostic. fmt/... work like printf. */
void diag_error(int line, int col, const char *fmt, ...);
void diag_warning(int line, int col, const char *fmt, ...);

bool diag_has_errors(void);
int diag_count(void);

/* Print every recorded diagnostic, sorted by (line, col), in the form:
 *   error [line 4, col 1]: CGPA 11.5 out of valid range (0-10)
 */
void diag_print_all(void);

/* Free all recorded diagnostics and reset the module (used by tests
 * and between compiler runs in the same process). */
void diag_reset(void);

#endif /* DIAGNOSTICS_H */
