#include "semantic.h"

#include <regex.h>
#include <stdio.h>
#include <string.h>

#include "diagnostics.h"

/* Simple but adequate email shape check: something@something.something,
 * no whitespace or '@' inside either half. */
#define EMAIL_PATTERN "^[^@[:space:]]+@[^@[:space:]]+\\.[^@[:space:]]+$"

static void check_name(const Resume *r) {
    if (r->name == NULL || r->name[0] == '\0') {
        diag_error(1, 1, "resume must have a 'name' declaration");
    }
}

static void check_email(const Resume *r) {
    if (r->email == NULL) {
        return; /* email is optional */
    }

    regex_t re;
    if (regcomp(&re, EMAIL_PATTERN, REG_EXTENDED | REG_NOSUB) != 0) {
        /* Should never happen -- the pattern is a fixed literal. */
        return;
    }
    int matched = regexec(&re, r->email, 0, NULL, 0) == 0;
    regfree(&re);

    if (!matched) {
        diag_error(r->email_line, r->email_col, "invalid email format: \"%s\"", r->email);
    }
}

static void check_cgpa_range(const Resume *r, double cgpa_scale) {
    for (int i = 0; i < r->edu_count; i++) {
        double cgpa = r->edu_entries[i].cgpa;
        if (cgpa < 0.0 || cgpa > cgpa_scale) {
            diag_error(r->edu_entries[i].line, r->edu_entries[i].col,
                       "CGPA %.2f out of valid range (0-%g)", cgpa, cgpa_scale);
        }
    }
}

static void check_duplicate_skills(const Resume *r) {
    /* For each skill, flag it as a duplicate the first time it matches
     * (case-insensitively) an earlier skill -- and then stop looking,
     * so three occurrences of "Python" produce one diagnostic instead
     * of one per repeated pair. */
    for (int j = 0; j < r->skill_count; j++) {
        for (int i = 0; i < j; i++) {
            if (strcasecmp(r->skills[i], r->skills[j]) == 0) {
                diag_error(r->skill_lines[j], r->skill_cols[j], "duplicate skill \"%s\"",
                           r->skills[j]);
                break;
            }
        }
    }
}

void semantic_analyze(const Resume *r, double cgpa_scale) {
    check_name(r);
    check_email(r);
    check_cgpa_range(r, cgpa_scale);
    check_duplicate_skills(r);
}
