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

%token NAME EMAIL EDUCATION COLLEGE CGPA SKILLS PROJECTS PROJECT DESCRIPTION
%token <sval> STRING
%token <sval> IDENTIFIER
%token <dval> NUMBER

%type <sval> description_opt

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
  | education_sec
  | skills_sec
  | projects_sec
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
  ;

projects_sec:
    PROJECTS project_entry_list
  ;

project_entry_list:
    project_entry
  | project_entry_list project_entry
  | project_entry_list error { yyerrok; yyclearin; }
  ;

project_entry:
    PROJECT STRING description_opt {
        resume_add_project(g_resume, $2, $3, @1.first_line, @1.first_column);
        free($2);
        free($3);
    }
  ;

description_opt:
    %empty { $$ = NULL; }
  | DESCRIPTION STRING { $$ = $2; }
  ;

%%

void yyerror(const char *msg) {
    diag_error(yylloc.first_line, yylloc.first_column, "%s", msg);
}
