#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <stdbool.h>

void diag_error(int line, int col, const char *fmt, ...);
bool diag_has_errors(void);
int diag_count(void);
const char *diag_get_message(int index);
int diag_get_line(int index);
int diag_get_col(int index);
void diag_print_all(void);
void diag_reset(void);

#endif
