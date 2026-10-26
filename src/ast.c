#include "ast.h"
#include "job_matcher.h"

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

static void write_json_escaped_string(FILE *fp, const char *s) {
    if (!s) {
        fputs("null", fp);
        return;
    }
    fputc('"', fp);
    for (const char *p = s; *p; p++) {
        if (*p == '"') fputs("\\\"", fp);
        else if (*p == '\\') fputs("\\\\", fp);
        else if (*p == '\n') fputs("\\n", fp);
        else if (*p == '\r') fputs("\\r", fp);
        else if (*p == '\t') fputs("\\t", fp);
        else if ((unsigned char)*p < 32) fprintf(fp, "\\u%04x", (unsigned char)*p);
        else fputc(*p, fp);
    }
    fputc('"', fp);
}

void resume_print_json(FILE *fp, const Resume *r, const struct JobAnalysis *ja) {
    const JobAnalysis *analysis = (const JobAnalysis *)ja;
    fputs("{\n", fp);
    fputs("  \"resume\": {\n", fp);
    fputs("    \"name\": ", fp); write_json_escaped_string(fp, r ? r->name : NULL); fputs(",\n", fp);
    fputs("    \"email\": ", fp); write_json_escaped_string(fp, r ? r->email : NULL); fputs(",\n", fp);
    fputs("    \"phone\": ", fp); write_json_escaped_string(fp, r ? r->phone : NULL); fputs(",\n", fp);
    fputs("    \"github\": ", fp); write_json_escaped_string(fp, r ? r->github : NULL); fputs(",\n", fp);
    fputs("    \"linkedin\": ", fp); write_json_escaped_string(fp, r ? r->linkedin : NULL); fputs(",\n", fp);

    fputs("    \"education\": [\n", fp);
    if (r) {
        for (int i = 0; i < r->edu_count; i++) {
            fprintf(fp, "      {\"college\": ");
            write_json_escaped_string(fp, r->edu_entries[i].college);
            fprintf(fp, ", \"cgpa\": %.2f}%s\n", r->edu_entries[i].cgpa, (i + 1 < r->edu_count) ? "," : "");
        }
    }
    fputs("    ],\n", fp);

    fputs("    \"skills\": [\n", fp);
    if (r) {
        for (int i = 0; i < r->skill_count; i++) {
            fputs("      ", fp);
            write_json_escaped_string(fp, r->skills[i].text);
            if (i + 1 < r->skill_count) fputs(",", fp);
            fputs("\n", fp);
        }
    }
    fputs("    ],\n", fp);

    fputs("    \"projects\": [\n", fp);
    if (r) {
        for (int i = 0; i < r->project_count; i++) {
            fputs("      {\n", fp);
            fputs("        \"title\": ", fp); write_json_escaped_string(fp, r->projects[i].name); fputs(",\n", fp);
            fputs("        \"bullets\": [\n", fp);
            for (int j = 0; j < r->projects[i].bullet_count; j++) {
                fputs("          ", fp);
                write_json_escaped_string(fp, r->projects[i].bullets[j]);
                if (j + 1 < r->projects[i].bullet_count) fputs(",", fp);
                fputs("\n", fp);
            }
            fputs("        ]\n", fp);
            fprintf(fp, "      }%s\n", (i + 1 < r->project_count) ? "," : "");
        }
    }
    fputs("    ],\n", fp);

    fputs("    \"experience\": [\n", fp);
    if (r) {
        for (int i = 0; i < r->exp_count; i++) {
            fputs("      {\n", fp);
            fputs("        \"company\": ", fp); write_json_escaped_string(fp, r->exp_entries[i].company); fputs(",\n", fp);
            fputs("        \"role\": ", fp); write_json_escaped_string(fp, r->exp_entries[i].role); fputs(",\n", fp);
            fputs("        \"duration\": ", fp); write_json_escaped_string(fp, r->exp_entries[i].duration); fputs(",\n", fp);
            fputs("        \"bullets\": [\n", fp);
            for (int j = 0; j < r->exp_entries[i].bullet_count; j++) {
                fputs("          ", fp);
                write_json_escaped_string(fp, r->exp_entries[i].bullets[j]);
                if (j + 1 < r->exp_entries[i].bullet_count) fputs(",", fp);
                fputs("\n", fp);
            }
            fputs("        ]\n", fp);
            fprintf(fp, "      }%s\n", (i + 1 < r->exp_count) ? "," : "");
        }
    }
    fputs("    ],\n", fp);

    fputs("    \"extras\": [\n", fp);
    if (r) {
        for (int i = 0; i < r->extra_count; i++) {
            fputs("      {\n", fp);
            fputs("        \"title\": ", fp); write_json_escaped_string(fp, r->extras[i].title); fputs(",\n", fp);
            fputs("        \"items\": [\n", fp);
            for (int j = 0; j < r->extras[i].item_count; j++) {
                fputs("          {\n", fp);
                fputs("            \"text\": ", fp); write_json_escaped_string(fp, r->extras[i].items[j].text); fputs(",\n", fp);
                fputs("            \"subbullets\": [\n", fp);
                for (int k = 0; k < r->extras[i].items[j].sub_count; k++) {
                    fputs("              ", fp);
                    write_json_escaped_string(fp, r->extras[i].items[j].subbullets[k]);
                    if (k + 1 < r->extras[i].items[j].sub_count) fputs(",", fp);
                    fputs("\n", fp);
                }
                fputs("            ]\n", fp);
                fprintf(fp, "          }%s\n", (j + 1 < r->extras[i].item_count) ? "," : "");
            }
            fputs("        ]\n", fp);
            fprintf(fp, "      }%s\n", (i + 1 < r->extra_count) ? "," : "");
        }
    }
    fputs("    ]\n", fp);
    fputs("  }", fp);

    if (analysis) {
        fputs(",\n  \"jobAnalysis\": {\n", fp);
        fputs("    \"source\": ", fp); write_json_escaped_string(fp, analysis->source_info); fputs(",\n", fp);
        fprintf(fp, "    \"matchPercentage\": %.1f,\n", analysis->match_percentage);
        fprintf(fp, "    \"matchedCount\": %d,\n", analysis->matched_count);
        fputs("    \"matchedSkills\": [\n", fp);
        for (int i = 0; i < analysis->matched_count; i++) {
            fputs("      ", fp); write_json_escaped_string(fp, analysis->matched_skills[i]);
            if (i + 1 < analysis->matched_count) fputs(",", fp);
            fputs("\n", fp);
        }
        fputs("    ],\n", fp);
        fprintf(fp, "    \"missingCount\": %d,\n", analysis->missing_count);
        fputs("    \"missingSkills\": [\n", fp);
        for (int i = 0; i < analysis->missing_count; i++) {
            fputs("      ", fp); write_json_escaped_string(fp, analysis->missing_skills[i]);
            if (i + 1 < analysis->missing_count) fputs(",", fp);
            fputs("\n", fp);
        }
        fputs("    ],\n", fp);
        fprintf(fp, "    \"jobSkillCount\": %d,\n", analysis->job_skill_count);
        fputs("    \"jobSkills\": [\n", fp);
        for (int i = 0; i < analysis->job_skill_count; i++) {
            fputs("      ", fp); write_json_escaped_string(fp, analysis->job_skills[i]);
            if (i + 1 < analysis->job_skill_count) fputs(",", fp);
            fputs("\n", fp);
        }
        fputs("    ]\n", fp);
        fputs("  }", fp);
    }

    fputs("\n}\n", fp);
}
