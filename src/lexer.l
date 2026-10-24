%{
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "diagnostics.h"
#include "parser.tab.h"

static int yycolumn = 1;

#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;

static char *unescape_string(const char *raw, int len);
%}

%option noyywrap
%option yylineno

%%

"name"          { return NAME; }
"email"         { return EMAIL; }
"phone"         { return PHONE; }
"github"        { return GITHUB; }
"linkedin"      { return LINKEDIN; }
"education"     { return EDUCATION; }
"college"       { return COLLEGE; }
"cgpa"          { return CGPA; }
"skills"        { return SKILLS; }
"projects"      { return PROJECTS; }
"project"       { return PROJECT; }
"bullet"        { return BULLET; }
"experience"    { return EXPERIENCE; }
"company"       { return COMPANY; }
"role"          { return ROLE; }
"duration"      { return DURATION; }
"extra"         { return EXTRA; }
"item"          { return ITEM; }

\"([^"\n\\]|\\.)*\" {
    yylval.sval = unescape_string(yytext, yyleng);
    return STRING;
}

\"([^"\n\\]|\\.)* {
    diag_error(yylloc.first_line, yylloc.first_column, "unterminated string literal");
    yylval.sval = strdup(yytext + 1);
    return STRING;
}

-?[0-9]+(\.[0-9]+)? {
    yylval.dval = atof(yytext);
    return NUMBER;
}

[A-Za-z][A-Za-z0-9_.+#-]* {
    yylval.sval = strdup(yytext);
    return IDENTIFIER;
}

"#".*        { /* skip comments */ }
\n           { yycolumn = 1; }
[ \t\r]+     { /* skip whitespace */ }

.            {
    diag_error(yylloc.first_line, yylloc.first_column, "unexpected character '%s'", yytext);
}

%%

static char *unescape_string(const char *raw, int len) {
    char *buf = malloc((size_t)len);
    int j = 0;
    for (int i = 1; i < len - 1; i++) {
        if (raw[i] == '\\' && i + 1 < len - 1) {
            i++;
            buf[j++] = raw[i];
        } else {
            buf[j++] = raw[i];
        }
    }
    buf[j] = '\0';
    return buf;
}
