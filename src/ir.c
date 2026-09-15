#include "ir.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ResumeIR *ir_lower(const Resume *ast) {
    ResumeIR *ir = calloc(1, sizeof(ResumeIR));
    if (!ir) {
        fprintf(stderr, "fatal: out of memory lowering to IR\n");
        exit(2);
    }

    ir->name = strdup(ast->name);
    ir->email = ast->email ? strdup(ast->email) : NULL;

    ir->education_count = ast->edu_count;
    ir->education = calloc((size_t)ast->edu_count, sizeof(IREduEntry));
    for (int i = 0; i < ast->edu_count; i++) {
        ir->education[i].college = strdup(ast->edu_entries[i].college);
        ir->education[i].cgpa = ast->edu_entries[i].cgpa;
    }

    ir->skill_count = ast->skill_count;
    ir->skills = calloc((size_t)ast->skill_count, sizeof(char *));
    for (int i = 0; i < ast->skill_count; i++) {
        ir->skills[i] = strdup(ast->skills[i]);
    }

    ir->project_count = ast->project_count;
    ir->projects = calloc((size_t)ast->project_count, sizeof(IRProject));
    for (int i = 0; i < ast->project_count; i++) {
        ir->projects[i].name = strdup(ast->projects[i].name);
        ir->projects[i].description =
            ast->projects[i].description ? strdup(ast->projects[i].description) : NULL;
    }

    return ir;
}

void ir_free(ResumeIR *ir) {
    if (!ir) {
        return;
    }
    free(ir->name);
    free(ir->email);

    for (int i = 0; i < ir->education_count; i++) {
        free(ir->education[i].college);
    }
    free(ir->education);

    for (int i = 0; i < ir->skill_count; i++) {
        free(ir->skills[i]);
    }
    free(ir->skills);

    for (int i = 0; i < ir->project_count; i++) {
        free(ir->projects[i].name);
        free(ir->projects[i].description);
    }
    free(ir->projects);

    free(ir);
}
