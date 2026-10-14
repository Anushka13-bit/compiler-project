#ifndef INTERACTIVE_H
#define INTERACTIVE_H

#include <stdbool.h>

/* Runs an interactive terminal prompt to gather resume details,
 * generates a .resume file, and sets the paths for input_resume and output_pdf.
 * Returns true if successful, false if cancelled or an error occurred. */
bool run_interactive_wizard(char **out_resume_path, char **out_pdf_path);

#endif
