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

static char **grow_str_array(char **arr, int *capacity, int count) {
    if (count < *capacity) return arr;
    *capacity = *capacity == 0 ? 4 : *capacity * 2;
    char **grown = realloc(arr, (size_t)*capacity * sizeof(char *));
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

void resume_add_project(Resume *r, const char *name, int line, int col) {
    r->projects = grow(r->projects, &r->project_capacity, r->project_count, sizeof(Project));
    Project *p = &r->projects[r->project_count++];
    p->name = strdup(name);
    p->bullets = NULL;
    p->bullet_count = 0;
    p->bullet_capacity = 0;
    p->line = line;
    p->col = col;
}

void resume_add_project_bullet(Resume *r, const char *bullet) {
    if (r->project_count == 0) return; /* safety */
    Project *p = &r->projects[r->project_count - 1];
    p->bullets = grow_str_array(p->bullets, &p->bullet_capacity, p->bullet_count);
    p->bullets[p->bullet_count++] = strdup(bullet);
}

void resume_add_exp_entry(Resume *r, const char *company, const char *role,
                          const char *duration, int line, int col) {
    r->exp_entries = grow(r->exp_entries, &r->exp_capacity, r->exp_count, sizeof(ExpEntry));
    ExpEntry *e = &r->exp_entries[r->exp_count++];
    e->company  = strdup(company);
    e->role     = role     ? strdup(role)     : NULL;
    e->duration = duration ? strdup(duration) : NULL;
    e->bullets  = NULL;
    e->bullet_count = 0;
    e->bullet_capacity = 0;
    e->line = line;
    e->col  = col;
}

void resume_add_exp_bullet(Resume *r, const char *bullet) {
    if (r->exp_count == 0) return; /* safety */
    ExpEntry *e = &r->exp_entries[r->exp_count - 1];
    e->bullets = grow_str_array(e->bullets, &e->bullet_capacity, e->bullet_count);
    e->bullets[e->bullet_count++] = strdup(bullet);
}

void resume_add_extra_section(Resume *r, const char *title, int line, int col) {
    r->extras = grow(r->extras, &r->extra_capacity, r->extra_count, sizeof(ExtraSection));
    ExtraSection *s = &r->extras[r->extra_count++];
    s->title = strdup(title);
    s->items = NULL;
    s->item_count = 0;
    s->item_capacity = 0;
    s->line = line;
    s->col  = col;
}

void resume_add_extra_item(Resume *r, const char *text) {
    if (r->extra_count == 0) return;
    ExtraSection *s = &r->extras[r->extra_count - 1];
    s->items = grow(s->items, &s->item_capacity, s->item_count, sizeof(ExtraItem));
    ExtraItem *it = &s->items[s->item_count++];
    it->text = strdup(text);
    it->subbullets = NULL;
    it->sub_count = 0;
    it->sub_capacity = 0;
}

void resume_add_extra_subbullet(Resume *r, const char *text) {
    if (r->extra_count == 0) return;
    ExtraSection *s = &r->extras[r->extra_count - 1];
    if (s->item_count == 0) return;
    ExtraItem *it = &s->items[s->item_count - 1];
    it->subbullets = grow_str_array(it->subbullets, &it->sub_capacity, it->sub_count);
    it->subbullets[it->sub_count++] = strdup(text);
}

void resume_free(Resume *r) {
    if (!r) {
        return;
    }
    free(r->name);
    free(r->email);
    free(r->phone);
    free(r->github);
    free(r->linkedin);
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
        for (int j = 0; j < r->projects[i].bullet_count; j++) {
            free(r->projects[i].bullets[j]);
        }
        free(r->projects[i].bullets);
    }
    free(r->projects);
    for (int i = 0; i < r->exp_count; i++) {
        free(r->exp_entries[i].company);
        free(r->exp_entries[i].role);
        free(r->exp_entries[i].duration);
        for (int j = 0; j < r->exp_entries[i].bullet_count; j++) {
            free(r->exp_entries[i].bullets[j]);
        }
        free(r->exp_entries[i].bullets);
    }
    free(r->exp_entries);
    for (int i = 0; i < r->extra_count; i++) {
        free(r->extras[i].title);
        for (int j = 0; j < r->extras[i].item_count; j++) {
            free(r->extras[i].items[j].text);
            for (int k = 0; k < r->extras[i].items[j].sub_count; k++) {
                free(r->extras[i].items[j].subbullets[k]);
            }
            free(r->extras[i].items[j].subbullets);
        }
        free(r->extras[i].items);
    }
    free(r->extras);
    free(r);
}

void resume_debug_print(const Resume *r) {
    printf("Resume:\n");
    printf("  name=%s\n",     r->name     ? r->name     : "(none)");
    printf("  email=%s\n",    r->email    ? r->email    : "(none)");
    printf("  phone=%s\n",    r->phone    ? r->phone    : "(none)");
    printf("  github=%s\n",   r->github   ? r->github   : "(none)");
    printf("  linkedin=%s\n", r->linkedin ? r->linkedin : "(none)");

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
        printf("    - name=%s [line %d, col %d]\n",
               r->projects[i].name, r->projects[i].line, r->projects[i].col);
        for (int j = 0; j < r->projects[i].bullet_count; j++) {
            printf("      * %s\n", r->projects[i].bullets[j]);
        }
    }

    printf("  experience (%d entries):\n", r->exp_count);
    for (int i = 0; i < r->exp_count; i++) {
        printf("    - company=%s role=%s duration=%s [line %d, col %d]\n",
               r->exp_entries[i].company,
               r->exp_entries[i].role     ? r->exp_entries[i].role     : "(none)",
               r->exp_entries[i].duration ? r->exp_entries[i].duration : "(none)",
               r->exp_entries[i].line, r->exp_entries[i].col);
        for (int j = 0; j < r->exp_entries[i].bullet_count; j++) {
            printf("      * %s\n", r->exp_entries[i].bullets[j]);
        }
    }

    printf("  extra sections (%d):\n", r->extra_count);
    for (int i = 0; i < r->extra_count; i++) {
        printf("    [%s]\n", r->extras[i].title);
        for (int j = 0; j < r->extras[i].item_count; j++) {
            printf("      - %s\n", r->extras[i].items[j].text);
            for (int k = 0; k < r->extras[i].items[j].sub_count; k++) {
                printf("          * %s\n", r->extras[i].items[j].subbullets[k]);
            }
        }
    }
}
