/*
 * Lexer (lexical analysis phase).
 *
 * Scans the raw DSL source and produces the token stream bison's parser
 * consumes. Recognizes the fixed keyword set, double-quoted string
 * literals (with \" and \\ escapes), numeric literals (for CGPA), and
 * bare-word identifiers (skill names like "C++").
 *
 * yylineno (via %option yylineno) gives us line numbers for free;
 * column numbers are not tracked by flex natively, so YY_USER_ACTION
 * below updates a running yycolumn and fills in bison's yylloc before
 * every rule's action runs.
 */

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
"education"     { return EDUCATION; }
"college"       { return COLLEGE; }
"cgpa"          { return CGPA; }
"skills"        { return SKILLS; }
"projects"      { return PROJECTS; }
"project"       { return PROJECT; }
"description"   { return DESCRIPTION; }

\"([^"\n\\]|\\.)*\" {
    yylval.sval = unescape_string(yytext, yyleng);
    return STRING;
}

\"([^"\n\\]|\\.)* {
    /* Same body as the well-formed STRING rule above, just without the
     * closing quote requirement -- so it only wins the longest-match
     * contest against that rule when there truly is no closing quote
     * before end of line/input. (A naive `\"[^\n]*` fallback would
     * greedily out-match *every* valid string that has more tokens
     * after it on the same line, silently corrupting the token stream.) */
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

"#".*        { /* line comment: ignored */ }

\n           { yycolumn = 1; }
[ \t\r]+     { /* skip whitespace */ }

.            {
    diag_error(yylloc.first_line, yylloc.first_column, "unexpected character '%s'", yytext);
}

%%

/* Strip the surrounding quotes from a raw STRING token and resolve the
 * \" and \\ escapes inside it into a freshly allocated buffer. */
static char *unescape_string(const char *raw, int len) {
    /* raw[0] and raw[len-1] are the quote characters */
    char *buf = malloc((size_t)len); /* always more than enough room */
    int j = 0;
    for (int i = 1; i < len - 1; i++) {
        if (raw[i] == '\\' && i + 1 < len - 1) {
            i++;
            switch (raw[i]) {
                case '"': buf[j++] = '"'; break;
                case '\\': buf[j++] = '\\'; break;
                default: buf[j++] = raw[i]; break;
            }
        } else {
            buf[j++] = raw[i];
        }
    }
    buf[j] = '\0';
    return buf;
}
