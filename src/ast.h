/*
 * AST node definitions.
 *
 * The bison grammar (parser.y) builds one of these Resume structs as it
 * reduces the token stream. Every leaf that can be blamed for a
 * diagnostic carries the source line/col it came from, so semantic
 * analysis can point errors back at the original DSL text.
 *
 * Note: the fields here go slightly beyond a minimal AST (a `col` next
 * to every `line`, and a line/col per skill) so that every diagnostic
 * emitted later can report a precise column, not just a line.
 */
#ifndef AST_H
#define AST_H

typedef struct EduEntry {
    char *college;
    double cgpa;
    int line;
    int col;
} EduEntry;

typedef struct Project {
    char *name;
    char *description; /* NULL if absent */
    int line;
    int col;
} Project;

typedef struct Resume {
    char *name; /* NULL if never declared */
    int name_line, name_col;

    char *email; /* NULL if absent */
    int email_line, email_col;

    EduEntry *edu_entries;
    int edu_count;
    int edu_capacity;

    char **skills;
    int *skill_lines;
    int *skill_cols;
    int skill_count;
    int skill_capacity;

    Project *projects;
    int project_count;
    int project_capacity;
} Resume;

Resume *resume_new(void);
void resume_free(Resume *r);

void resume_add_edu_entry(Resume *r, const char *college, double cgpa, int line, int col);
void resume_add_skill(Resume *r, const char *text, int line, int col);
void resume_add_project(Resume *r, const char *name, const char *description, int line, int col);

/* Pretty-prints the whole tree to stdout. Useful for --dump-ast and for
 * exercising the lexer/parser independently of the rest of the pipeline. */
void resume_debug_print(const Resume *r);

#endif /* AST_H */
