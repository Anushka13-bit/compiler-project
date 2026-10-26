#include "diagnostics.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *message;
    int line, col;
} Diagnostic;

static Diagnostic *g_diags = NULL;
static int g_count = 0;
static int g_capacity = 0;

void diag_error(int line, int col, const char *fmt, ...) {
    if (g_count >= g_capacity) {
        g_capacity = g_capacity == 0 ? 16 : g_capacity * 2;
        g_diags = realloc(g_diags, (size_t)g_capacity * sizeof(Diagnostic));
        if (!g_diags) {
            fprintf(stderr, "fatal: out of memory\n");
            exit(2);
        }
    }

    va_list args;
    va_start(args, fmt);
    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    char *message = malloc((size_t)needed + 1);
    if (!message) {
        fprintf(stderr, "fatal: out of memory\n");
        exit(2);
    }
    va_start(args, fmt);
    vsnprintf(message, (size_t)needed + 1, fmt, args);
    va_end(args);

    g_diags[g_count++] = (Diagnostic){message, line, col};
}

bool diag_has_errors(void) {
    return g_count > 0;
}

int diag_count(void) {
    return g_count;
}

static int compare_diags(const void *a, const void *b) {
    const Diagnostic *da = a, *db = b;
    if (da->line != db->line) {
        return da->line - db->line;
    }
    return da->col - db->col;
}

const char *diag_get_message(int index) {
    if (index < 0 || index >= g_count) return NULL;
    return g_diags[index].message;
}

int diag_get_line(int index) {
    if (index < 0 || index >= g_count) return 0;
    return g_diags[index].line;
}

int diag_get_col(int index) {
    if (index < 0 || index >= g_count) return 0;
    return g_diags[index].col;
}

void diag_print_all(void) {
    qsort(g_diags, (size_t)g_count, sizeof(Diagnostic), compare_diags);
    for (int i = 0; i < g_count; i++) {
        fprintf(stderr, "error [line %d, col %d]: %s\n", g_diags[i].line, g_diags[i].col,
                g_diags[i].message);
    }
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
