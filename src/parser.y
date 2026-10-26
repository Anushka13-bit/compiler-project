%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "diagnostics.h"

extern int yylex(void);
extern int yylineno;
extern FILE *yyin;
extern char *yytext;

void yyerror(const char *msg);

Resume *g_resume;
%}

%locations

%union {
    double dval;
    char *sval;
}

%token NAME EMAIL PHONE GITHUB LINKEDIN
%token EDUCATION COLLEGE CGPA
%token SKILLS
%token PROJECTS PROJECT BULLET
%token EXPERIENCE COMPANY ROLE DURATION
%token EXTRA ITEM
%token <sval> STRING
%token <sval> IDENTIFIER
%token <dval> NUMBER

%define parse.error verbose

%%

resume:
    decl_list
  ;

decl_list:
    %empty
  | decl_list decl
  | decl_list error { yyerrok; yyclearin; }
  ;

decl:
    name_decl
  | email_decl
  | phone_decl
  | github_decl
  | linkedin_decl
  | education_sec
  | skills_sec
  | projects_sec
  | experience_sec
  | extra_sec
  ;

name_decl:
    NAME STRING {
        if (g_resume->name != NULL) {
            diag_error(@1.first_line, @1.first_column, "duplicate 'name' declaration");
            free($2);
        } else {
            g_resume->name = $2;
            g_resume->name_line = @1.first_line;
            g_resume->name_col = @1.first_column;
        }
    }
  ;

email_decl:
    EMAIL STRING {
        if (g_resume->email != NULL) {
            diag_error(@1.first_line, @1.first_column, "duplicate 'email' declaration");
            free($2);
        } else {
            g_resume->email = $2;
            g_resume->email_line = @1.first_line;
            g_resume->email_col = @1.first_column;
        }
    }
  ;

phone_decl:
    PHONE STRING {
        if (g_resume->phone != NULL) {
            diag_error(@1.first_line, @1.first_column, "duplicate 'phone' declaration");
            free($2);
        } else {
            g_resume->phone = $2;
            g_resume->phone_line = @1.first_line;
            g_resume->phone_col = @1.first_column;
        }
    }
  ;

github_decl:
    GITHUB STRING {
        if (g_resume->github != NULL) {
            diag_error(@1.first_line, @1.first_column, "duplicate 'github' declaration");
            free($2);
        } else {
            g_resume->github = $2;
            g_resume->github_line = @1.first_line;
            g_resume->github_col = @1.first_column;
        }
    }
  ;

linkedin_decl:
    LINKEDIN STRING {
        if (g_resume->linkedin != NULL) {
            diag_error(@1.first_line, @1.first_column, "duplicate 'linkedin' declaration");
            free($2);
        } else {
            g_resume->linkedin = $2;
            g_resume->linkedin_line = @1.first_line;
            g_resume->linkedin_col = @1.first_column;
        }
    }
  ;

/* ── Education ────────────────────────────────────────── */

education_sec:
    EDUCATION edu_entry_list
  ;

edu_entry_list:
    edu_entry
  | edu_entry_list edu_entry
  | edu_entry_list error { yyerrok; yyclearin; }
  ;

edu_entry:
    COLLEGE STRING CGPA NUMBER {
        resume_add_edu_entry(g_resume, $2, $4, @1.first_line, @1.first_column);
        free($2);
    }
  ;

/* ── Skills ───────────────────────────────────────────── */

skills_sec:
    SKILLS skill_list
  ;

skill_list:
    skill_item
  | skill_list skill_item
  | skill_list error { yyerrok; yyclearin; }
  ;

skill_item:
    IDENTIFIER {
        resume_add_skill(g_resume, $1, @1.first_line, @1.first_column);
        free($1);
    }
  | STRING {
        resume_add_skill(g_resume, $1, @1.first_line, @1.first_column);
        free($1);
    }
  ;

/* ── Projects ─────────────────────────────────────────── */

projects_sec:
    PROJECTS project_entry_list
  ;

project_entry_list:
    project_entry
  | project_entry_list project_entry
  | project_entry_list error { yyerrok; yyclearin; }
  ;

/*
 * project_open is a mid-rule that opens the project in the AST
 * as soon as we have its name, so that bullet items added
 * in project_bullet_list land on the correct (latest) project.
 */
project_open:
    PROJECT STRING {
        resume_add_project(g_resume, $2, @1.first_line, @1.first_column);
        free($2);
    }
  ;

project_entry:
    project_open project_bullet_list
  ;

project_bullet_list:
    %empty
  | project_bullet_list BULLET STRING {
        resume_add_project_bullet(g_resume, $3);
        free($3);
    }
  ;

/* ── Experience ───────────────────────────────────────── */

experience_sec:
    EXPERIENCE exp_entry_list
  ;

exp_entry_list:
    exp_entry
  | exp_entry_list exp_entry
  | exp_entry_list error { yyerrok; yyclearin; }
  ;

/*
 * exp_open opens the experience entry as soon as company name is known,
 * so optional role/duration and bullets are attached to the right entry.
 */
exp_open:
    COMPANY STRING {
        resume_add_exp_entry(g_resume, $2, NULL, NULL,
                             @1.first_line, @1.first_column);
        free($2);
    }
  ;

exp_entry:
    exp_open exp_role_opt exp_duration_opt exp_bullet_list_opt
  ;

exp_role_opt:
    %empty
  | ROLE STRING {
        if (g_resume->exp_count > 0) {
            ExpEntry *e = &g_resume->exp_entries[g_resume->exp_count - 1];
            free(e->role);
            e->role = $2;
        } else {
            free($2);
        }
    }
  ;

exp_duration_opt:
    %empty
  | DURATION STRING {
        if (g_resume->exp_count > 0) {
            ExpEntry *e = &g_resume->exp_entries[g_resume->exp_count - 1];
            free(e->duration);
            e->duration = $2;
        } else {
            free($2);
        }
    }
  ;

exp_bullet_list_opt:
    %empty
  | exp_bullet_list_opt BULLET STRING {
        resume_add_exp_bullet(g_resume, $3);
        free($3);
    }
  ;

/* ── Extra / Custom Sections ──────────────────────────── */

extra_sec:
    extra_open extra_item_list
  ;

extra_open:
    EXTRA STRING {
        resume_add_extra_section(g_resume, $2, @1.first_line, @1.first_column);
        free($2);
    }
  ;

extra_item_list:
    %empty
  | extra_item_list extra_item_entry
  | extra_item_list error { yyerrok; yyclearin; }
  ;

extra_item_open:
    ITEM STRING {
        resume_add_extra_item(g_resume, $2);
        free($2);
    }
  ;

extra_item_entry:
    extra_item_open extra_subbullet_list
  ;

extra_subbullet_list:
    %empty
  | extra_subbullet_list BULLET STRING {
        resume_add_extra_subbullet(g_resume, $3);
        free($3);
    }
  ;

%%

void yyerror(const char *msg) {
    diag_error(yylloc.first_line, yylloc.first_column, "%s", msg);
}
