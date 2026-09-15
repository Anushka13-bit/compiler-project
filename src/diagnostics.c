#include "diagnostics.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Diagnostic *g_diags = NULL;
static int g_count = 0;
static int g_capacity = 0;

static void ensure_capacity(void) {
    if (g_count < g_capacity) {
        return;
    }
    g_capacity = g_capacity == 0 ? 16 : g_capacity * 2;
    g_diags = realloc(g_diags, (size_t)g_capacity * sizeof(Diagnostic));
    if (!g_diags) {
        fprintf(stderr, "fatal: out of memory recording diagnostics\n");
        exit(2);
    }
}

static void record(Severity severity, int line, int col, const char *fmt, va_list args) {
    ensure_capacity();

    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    char *message = malloc((size_t)needed + 1);
    if (!message) {
        fprintf(stderr, "fatal: out of memory recording diagnostics\n");
        exit(2);
    }
    vsnprintf(message, (size_t)needed + 1, fmt, args);

    g_diags[g_count].severity = severity;
    g_diags[g_count].message = message;
    g_diags[g_count].line = line;
    g_diags[g_count].col = col;
    g_count++;
}

void diag_error(int line, int col, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    record(DIAG_ERROR, line, col, fmt, args);
    va_end(args);
}

void diag_warning(int line, int col, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    record(DIAG_WARNING, line, col, fmt, args);
    va_end(args);
}

bool diag_has_errors(void) {
    for (int i = 0; i < g_count; i++) {
        if (g_diags[i].severity == DIAG_ERROR) {
            return true;
        }
    }
    return false;
}

int diag_count(void) {
    return g_count;
}

static int compare_diags(const void *a, const void *b) {
    const Diagnostic *da = (const Diagnostic *)a;
    const Diagnostic *db = (const Diagnostic *)b;
    if (da->line != db->line) {
        return da->line - db->line;
    }
    return da->col - db->col;
}

void diag_print_all(void) {
    if (g_count == 0) {
        return;
    }
    Diagnostic *sorted = malloc((size_t)g_count * sizeof(Diagnostic));
    memcpy(sorted, g_diags, (size_t)g_count * sizeof(Diagnostic));
    qsort(sorted, (size_t)g_count, sizeof(Diagnostic), compare_diags);

    for (int i = 0; i < g_count; i++) {
        const char *severity = sorted[i].severity == DIAG_ERROR ? "error" : "warning";
        fprintf(stderr, "%s [line %d, col %d]: %s\n", severity, sorted[i].line, sorted[i].col,
                sorted[i].message);
    }
    free(sorted);
}

void diag_reset(void) {
    for (int i = 0; i < g_count; i++) {
        free(g_diags[i].message);
    }
    free(g_diags);
    g_diags = NULL;
    g_count = 0;
    g_capacity = 0;
}
