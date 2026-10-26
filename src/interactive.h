#ifndef INTERACTIVE_H
#define INTERACTIVE_H

#include <stdbool.h>

/* Runs an interactive terminal prompt to gather resume details,
 * optionally fetches/tailors against a target job link or file,
 * verifies against job requirements, and generates a .resume file.
 * Returns true if successful, false if cancelled or an error occurred. */
bool run_interactive_wizard(char **out_resume_path, char **out_pdf_path, const char *initial_job_source);

#endif
