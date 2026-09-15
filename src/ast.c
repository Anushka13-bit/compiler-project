#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Resume *resume_new(void) {
    Resume *r = calloc(1, sizeof(Resume));
    if (!r) {
        fprintf(stderr, "fatal: out of memory allocating Resume\n");
        exit(2);
    }
    return r;
}

static void *grow(void *ptr, int *capacity, int needed_count, size_t elem_size) {
    if (needed_count < *capacity) {
        return ptr;
    }
    *capacity = *capacity == 0 ? 4 : *capacity * 2;
    void *grown = realloc(ptr, (size_t)*capacity * elem_size);
    if (!grown) {
        fprintf(stderr, "fatal: out of memory growing AST list\n");
        exit(2);
    }
    return grown;
}

void resume_add_edu_entry(Resume *r, const char *college, double cgpa, int line, int col) {
    r->edu_entries = grow(r->edu_entries, &r->edu_capacity, r->edu_count, sizeof(EduEntry));
    r->edu_entries[r->edu_count].college = strdup(college);
    r->edu_entries[r->edu_count].cgpa = cgpa;
    r->edu_entries[r->edu_count].line = line;
    r->edu_entries[r->edu_count].col = col;
    r->edu_count++;
}

void resume_add_skill(Resume *r, const char *text, int line, int col) {
    /* skills/skill_lines/skill_cols are three parallel arrays sharing one
     * count and one capacity, so they must grow together in a single
     * check -- growing them via three separate calls to the generic
     * grow() helper above would only reallocate the first one, since
     * growing it would already bump the shared capacity for the rest. */
    if (r->skill_count >= r->skill_capacity) {
        int new_capacity = r->skill_capacity == 0 ? 4 : r->skill_capacity * 2;
        char **new_skills = realloc(r->skills, (size_t)new_capacity * sizeof(char *));
        int *new_lines = realloc(r->skill_lines, (size_t)new_capacity * sizeof(int));
        int *new_cols = realloc(r->skill_cols, (size_t)new_capacity * sizeof(int));
        if (!new_skills || !new_lines || !new_cols) {
            fprintf(stderr, "fatal: out of memory growing AST list\n");
            exit(2);
        }
        r->skills = new_skills;
        r->skill_lines = new_lines;
        r->skill_cols = new_cols;
        r->skill_capacity = new_capacity;
    }
    r->skills[r->skill_count] = strdup(text);
    r->skill_lines[r->skill_count] = line;
    r->skill_cols[r->skill_count] = col;
    r->skill_count++;
}

void resume_add_project(Resume *r, const char *name, const char *description, int line, int col) {
    r->projects = grow(r->projects, &r->project_capacity, r->project_count, sizeof(Project));
    r->projects[r->project_count].name = strdup(name);
    r->projects[r->project_count].description = description ? strdup(description) : NULL;
    r->projects[r->project_count].line = line;
    r->projects[r->project_count].col = col;
    r->project_count++;
}

void resume_free(Resume *r) {
    if (!r) {
        return;
    }
    free(r->name);
    free(r->email);

    for (int i = 0; i < r->edu_count; i++) {
        free(r->edu_entries[i].college);
    }
    free(r->edu_entries);

    for (int i = 0; i < r->skill_count; i++) {
        free(r->skills[i]);
    }
    free(r->skills);
    free(r->skill_lines);
    free(r->skill_cols);

    for (int i = 0; i < r->project_count; i++) {
        free(r->projects[i].name);
        free(r->projects[i].description);
    }
    free(r->projects);

    free(r);
}

void resume_debug_print(const Resume *r) {
    printf("Resume:\n");
    printf("  name=%s\n", r->name ? r->name : "(none)");
    printf("  email=%s\n", r->email ? r->email : "(none)");

    printf("  education (%d entries):\n", r->edu_count);
    for (int i = 0; i < r->edu_count; i++) {
        printf("    - college=%s cgpa=%.2f [line %d, col %d]\n", r->edu_entries[i].college,
               r->edu_entries[i].cgpa, r->edu_entries[i].line, r->edu_entries[i].col);
    }

    printf("  skills (%d): [", r->skill_count);
    for (int i = 0; i < r->skill_count; i++) {
        printf("%s%s", r->skills[i], i + 1 < r->skill_count ? ", " : "");
    }
    printf("]\n");

    printf("  projects (%d entries):\n", r->project_count);
    for (int i = 0; i < r->project_count; i++) {
        printf("    - name=%s description=%s [line %d, col %d]\n", r->projects[i].name,
               r->projects[i].description ? r->projects[i].description : "(none)",
               r->projects[i].line, r->projects[i].col);
    }
}
