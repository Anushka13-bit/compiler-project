#include "semantic.h"

#include <regex.h>
#include <string.h>
#include <strings.h>

#include "diagnostics.h"

#define EMAIL_PATTERN "^[^@[:space:]]+@[^@[:space:]]+\\.[^@[:space:]]+$"

void semantic_analyze(const Resume *r, double cgpa_scale) {
    if (r->name == NULL || r->name[0] == '\0') {
        diag_error(1, 1, "resume must have a 'name' declaration");
    }

    if (r->email != NULL) {
        regex_t re;
        if (regcomp(&re, EMAIL_PATTERN, REG_EXTENDED | REG_NOSUB) == 0) {
            if (regexec(&re, r->email, 0, NULL, 0) != 0) {
                diag_error(r->email_line, r->email_col, "invalid email format: \"%s\"", r->email);
            }
            regfree(&re);
        }
    }

    for (int i = 0; i < r->edu_count; i++) {
        double cgpa = r->edu_entries[i].cgpa;
        if (cgpa < 0.0 || cgpa > cgpa_scale) {
            diag_error(r->edu_entries[i].line, r->edu_entries[i].col,
                       "CGPA %.2f out of valid range (0-%g)", cgpa, cgpa_scale);
        }
    }

    for (int j = 0; j < r->skill_count; j++) {
        for (int i = 0; i < j; i++) {
            if (strcasecmp(r->skills[i].text, r->skills[j].text) == 0) {
                diag_error(r->skills[j].line, r->skills[j].col, "duplicate skill \"%s\"",
                           r->skills[j].text);
                break;
            }
        }
    }
}
