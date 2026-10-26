#ifndef RESUME_PARSER_H
#define RESUME_PARSER_H

#include <stdbool.h>
#include "ast.h"

/*
 * Extracts text from an existing PDF file using pdftotext (or fallback),
 * and parses it into a populated Resume AST struct.
 * Returns NULL on error.
 */
Resume *resume_load_from_pdf(const char *pdf_path);

/*
 * Loads an existing resume from either a .pdf or .resume DSL file.
 */
Resume *resume_load_from_any(const char *path);

/*
 * Serializes a Resume AST struct back into clean .resume DSL syntax.
 * Returns true on success, false on error.
 */
bool resume_write_dsl(const Resume *r, const char *dsl_path);

/*
 * Targeted Tailoring Assistant:
 * Takes an existing resume (PDF or .resume) and a target job link/file.
 * Shows what is already covered vs what is missing.
 * Prompts ONLY for missing skills and targeted bullet points.
 * All other existing resume details stay the same.
 * Generates an updated tailored DSL file and compiles it to PDF.
 */
bool run_tailor_existing_resume(const char *resume_input_path, const char *job_source,
                               char **out_resume, char **out_pdf);

#endif /* RESUME_PARSER_H */
