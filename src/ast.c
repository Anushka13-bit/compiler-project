#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Resume *resume_new(void) {
    Resume *r = calloc(1, sizeof(Resume));
    if (!r) {
        fprintf(stderr, "fatal: out of memory\n");
        exit(2);
    }
    return r;
}

static void *grow(void *ptr, int *capacity, int count, size_t elem_size) {
    if (count < *capacity) {
        return ptr;
    }
    *capacity = *capacity == 0 ? 4 : *capacity * 2;
    void *grown = realloc(ptr, (size_t)*capacity * elem_size);
    if (!grown) {
        fprintf(stderr, "fatal: out of memory\n");
        exit(2);
    }
    return grown;
}

void resume_add_edu_entry(Resume *r, const char *college, double cgpa, int line, int col) {
    r->edu_entries = grow(r->edu_entries, &r->edu_capacity, r->edu_count, sizeof(EduEntry));
    r->edu_entries[r->edu_count++] = (EduEntry){strdup(college), cgpa, line, col};
}

void resume_add_skill(Resume *r, const char *text, int line, int col) {
    r->skills = grow(r->skills, &r->skill_capacity, r->skill_count, sizeof(Skill));
    r->skills[r->skill_count++] = (Skill){strdup(text), line, col};
}

void resume_add_project(Resume *r, const char *name, const char *description, int line, int col) {
    r->projects = grow(r->projects, &r->project_capacity, r->project_count, sizeof(Project));
    r->projects[r->project_count++] =
        (Project){strdup(name), description ? strdup(description) : NULL, line, col};
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
        free(r->skills[i].text);
    }
    free(r->skills);
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
        printf("%s%s", r->skills[i].text, i + 1 < r->skill_count ? ", " : "");
    }
    printf("]\n");

    printf("  projects (%d entries):\n", r->project_count);
    for (int i = 0; i < r->project_count; i++) {
        printf("    - name=%s description=%s [line %d, col %d]\n", r->projects[i].name,
               r->projects[i].description ? r->projects[i].description : "(none)",
               r->projects[i].line, r->projects[i].col);
    }
}
