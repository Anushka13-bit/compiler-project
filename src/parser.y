/*
 * Parser (syntax analysis phase).
 *
 * Bison grammar matching:
 *
 *   resume          := decl*
 *   decl            := name_decl | email_decl | section
 *   name_decl       := "name" STRING
 *   email_decl      := "email" STRING
 *   section         := education_sec | skills_sec | projects_sec
 *   education_sec   := "education" education_entry+
 *   education_entry := "college" STRING "cgpa" NUMBER
 *   skills_sec      := "skills" IDENTIFIER+
 *   projects_sec    := "projects" project_entry+
 *   project_entry   := "project" STRING description_decl?
 *   description_decl:= "description" STRING
 *
 * This is one deliberate refinement over the EBNF's "name_decl
 * email_decl? section*": here name/email/sections may appear in any
 * order (decl*) rather than name being forced first. That is what lets
 * a resume with a missing `name` still parse successfully -- so
 * "name is required" can be a *semantic* diagnostic (as the spec's own
 * negative-test list requires) instead of a hard syntax error that
 * would abort the parse before semantic analysis even runs.
 *
 * Grammar actions only ever build the AST (ast.h) -- no validation
 * happens here. Error recovery uses bison's standard `error` token at
 * four synchronization points (top-level decl list, and each of the
 * education/skills/projects entry lists) so one malformed construct
 * doesn't stop the whole parse. Every one of those four rules pairs
 * `yyerrok` with `yyclearin` -- see the comment on the first one for
 * why `yyclearin` is not optional (without it, recovery loops forever
 * instead of discarding the bad token).
 *
 * One known, accepted gap: an entry list can only resynchronize once
 * it holds at least one successfully-parsed entry (the `error`
 * alternative needs that nonterminal already reduced once to attach
 * to). So a section that fails on its very *first* entry -- most
 * visibly, a required section left completely empty, e.g. `skills`
 * followed immediately by the next keyword -- has no local recovery
 * point yet and bubbles all the way up to the top-level decl_list,
 * which then discards tokens (possibly swallowing whatever the next,
 * otherwise-valid section was) until it finds a fresh NAME/EMAIL/
 * EDUCATION/SKILLS/PROJECTS keyword to resume at.
 *
 * Bison reports 2 shift/reduce conflicts here, both on the `error`
 * token where an education/projects entry list could either recover
 * locally (shift) or bubble the error up to the top-level decl_list
 * (reduce). The default resolution (shift) is also the one we want:
 * a bad `college`/`project` entry gets resynchronized inside its own
 * section instead of discarding the rest of that section too.
 */

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

Resume *g_resume; /* the tree being built; owned by main.c */
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
  | decl_list error {
        /* yyerrok re-enables error reporting for the *next* syntax
         * error. yyclearin discards the token that caused this one --
         * without it, this reduction (which needs no token of its own)
         * would fire again and again on that same still-pending
         * lookahead and the parser would never advance, looping
         * forever instead of resynchronizing. */
        yyerrok;
        yyclearin;
    }
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
  | edu_entry_list error {
        /* yyerrok re-enables error reporting for the *next* syntax
         * error. yyclearin discards the token that caused this one --
         * without it, this reduction (which needs no token of its own)
         * would fire again and again on that same still-pending
         * lookahead and the parser would never advance, looping
         * forever instead of resynchronizing. */
        yyerrok;
        yyclearin;
    }
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
  | skill_list error {
        yyerrok;
        yyclearin;
    }
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
  | project_entry_list error {
        /* yyerrok re-enables error reporting for the *next* syntax
         * error. yyclearin discards the token that caused this one --
         * without it, this reduction (which needs no token of its own)
         * would fire again and again on that same still-pending
         * lookahead and the parser would never advance, looping
         * forever instead of resynchronizing. */
        yyerrok;
        yyclearin;
    }
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
