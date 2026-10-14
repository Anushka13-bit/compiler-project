#ifndef AST_H
#define AST_H

typedef struct {
    char *college;
    double cgpa;
    int line, col;
} EduEntry;

typedef struct {
    char *text;
    int line, col;
} Skill;

typedef struct {
    char *name;
    char *description; /* NULL if omitted */
    int line, col;
} Project;

typedef struct {
    char *name;  /* NULL if missing */
    int name_line, name_col;

    char *email; /* NULL if omitted */
    int email_line, email_col;

    EduEntry *edu_entries;
    int edu_count, edu_capacity;

    Skill *skills;
    int skill_count, skill_capacity;

    Project *projects;
    int project_count, project_capacity;
} Resume;

Resume *resume_new(void);
void resume_free(Resume *r);

void resume_add_edu_entry(Resume *r, const char *college, double cgpa, int line, int col);
void resume_add_skill(Resume *r, const char *text, int line, int col);
void resume_add_project(Resume *r, const char *name, const char *description, int line, int col);
void resume_debug_print(const Resume *r);

#endif
