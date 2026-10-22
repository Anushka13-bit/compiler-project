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
    char **bullets;      /* dynamic array of bullet strings */
    int bullet_count, bullet_capacity;
    int line, col;
} Project;

typedef struct {
    char *company;
    char *role;          /* NULL if omitted */
    char *duration;      /* NULL if omitted */
    char **bullets;      /* dynamic array of bullet strings */
    int bullet_count, bullet_capacity;
    int line, col;
} ExpEntry;

typedef struct {
    char *name;  /* NULL if missing */
    int name_line, name_col;

    char *email; /* NULL if omitted */
    int email_line, email_col;

    char *phone;   /* NULL if omitted */
    int phone_line, phone_col;

    char *github;  /* NULL if omitted */
    int github_line, github_col;

    char *linkedin; /* NULL if omitted */
    int linkedin_line, linkedin_col;

    EduEntry *edu_entries;
    int edu_count, edu_capacity;

    Skill *skills;
    int skill_count, skill_capacity;

    Project *projects;
    int project_count, project_capacity;

    ExpEntry *exp_entries;
    int exp_count, exp_capacity;
} Resume;

Resume *resume_new(void);
void resume_free(Resume *r);

void resume_add_edu_entry(Resume *r, const char *college, double cgpa, int line, int col);
void resume_add_skill(Resume *r, const char *text, int line, int col);
/* Start a new project; subsequent bullets added via resume_add_project_bullet */
void resume_add_project(Resume *r, const char *name, int line, int col);
void resume_add_project_bullet(Resume *r, const char *bullet);
/* Start a new experience entry; subsequent bullets added via resume_add_exp_bullet */
void resume_add_exp_entry(Resume *r, const char *company, const char *role,
                          const char *duration, int line, int col);
void resume_add_exp_bullet(Resume *r, const char *bullet);
void resume_debug_print(const Resume *r);

#endif
