/*
 * Intermediate representation (IR).
 *
 * A normalized, template-friendly view of a resume, deliberately
 * decoupled from ast.h: it drops the line/col source-position fields
 * (only useful for diagnostics, not for rendering) and is the *only*
 * thing codegen.c is allowed to depend on. That one-way dependency
 * (codegen.h includes ir.h, never ast.h) is what makes "lowering" a
 * real, separate phase instead of parser-output-equals-codegen-input.
 */
#ifndef IR_H
#define IR_H

#include "ast.h"

typedef struct IREduEntry {
    char *college;
    double cgpa;
} IREduEntry;

typedef struct IRProject {
    char *name;
    char *description; /* NULL if absent */
} IRProject;

typedef struct ResumeIR {
    char *name;
    char *email; /* NULL if absent */

    IREduEntry *education;
    int education_count;

    char **skills;
    int skill_count;

    IRProject *projects;
    int project_count;
} ResumeIR;

/* Lowers a validated AST into IR. Caller must have already confirmed
 * there are no error-level diagnostics before calling this. */
ResumeIR *ir_lower(const Resume *ast);
void ir_free(ResumeIR *ir);

#endif /* IR_H */
