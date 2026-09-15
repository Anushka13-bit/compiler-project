/*
 * Semantic analyzer.
 *
 * Walks the completed AST and enforces rules syntax alone can't catch.
 * Every violation is recorded via diagnostics.h (diag_error /
 * diag_warning); this module never aborts on the first problem it
 * finds. After calling semantic_analyze(), the caller checks
 * diag_has_errors() to decide whether to proceed to IR generation.
 */
#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

/* cgpa_scale is the top of the valid CGPA range (bottom is always 0),
 * e.g. 10.0 for a 0-10 scale or 4.0 for a 0-4 scale. */
void semantic_analyze(const Resume *r, double cgpa_scale);

#endif /* SEMANTIC_H */
