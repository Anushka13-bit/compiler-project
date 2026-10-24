#include "semantic.h"

#include <regex.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>

#include "diagnostics.h"

#define EMAIL_PATTERN    "^[^@[:space:]]+@[^@[:space:]]+\\.[^@[:space:]]+$"
#define PHONE_PATTERN    "^[+]?[0-9 ()\\-]{6,19}$"
#define GITHUB_PATTERN   "^[A-Za-z0-9]([A-Za-z0-9-]{0,37}[A-Za-z0-9])?$"
#define LINKEDIN_PATTERN "^[A-Za-z0-9_-]{3,100}$"

/* Count only digit characters in s */
static int count_digits(const char *s) {
    int n = 0;
    for (; *s; s++) if (*s >= '0' && *s <= '9') n++;
    return n;
}

/* Compile pattern and emit diag_error at (line, col) if value does not match */
static void check_regex(const char *pattern, const char *value,
                        int line, int col, const char *msg) {
    regex_t re;
    if (regcomp(&re, pattern, REG_EXTENDED | REG_NOSUB) == 0) {
        if (regexec(&re, value, 0, NULL, 0) != 0) {
            diag_error(line, col, "%s", msg);
        }
        regfree(&re);
    }
}

void semantic_analyze(const Resume *r, double cgpa_scale) {
    /* ── Name required ───────────────────────────────────── */
    if (r->name == NULL || r->name[0] == '\0') {
        diag_error(1, 1, "resume must have a 'name' declaration");
    }

    /* ── Email format ────────────────────────────────────── */
    if (r->email != NULL) {
        check_regex(EMAIL_PATTERN, r->email,
                    r->email_line, r->email_col,
                    "invalid email format: expected user@domain.tld");
    }

    /* ── Phone: allowed chars + must have 7-15 digits ────── */
    if (r->phone != NULL) {
        int digits = count_digits(r->phone);
        bool fmt_ok = false;
        regex_t re;
        if (regcomp(&re, PHONE_PATTERN, REG_EXTENDED | REG_NOSUB) == 0) {
            fmt_ok = (regexec(&re, r->phone, 0, NULL, 0) == 0);
            regfree(&re);
        }
        if (!fmt_ok || digits < 7 || digits > 15) {
            diag_error(r->phone_line, r->phone_col,
                       "invalid phone number \"%s\": expected 7-15 digits, "
                       "optionally with +, spaces, hyphens, or parentheses "
                       "(e.g. \"+91 98765 43210\" or \"(212) 555-0100\")",
                       r->phone);
        }
    }

    /* ── GitHub username: alphanumeric + hyphens only ────── */
    if (r->github != NULL) {
        check_regex(GITHUB_PATTERN, r->github,
                    r->github_line, r->github_col,
                    "invalid GitHub username: use only letters, digits, and hyphens "
                    "(do not include 'github.com/')");
    }

    /* ── LinkedIn username: alphanumeric + hyphens + _ ───── */
    if (r->linkedin != NULL) {
        check_regex(LINKEDIN_PATTERN, r->linkedin,
                    r->linkedin_line, r->linkedin_col,
                    "invalid LinkedIn username: use only letters, digits, hyphens, "
                    "and underscores (do not include 'linkedin.com/in/')");
    }

    /* ── CGPA range ──────────────────────────────────────── */
    for (int i = 0; i < r->edu_count; i++) {
        double cgpa = r->edu_entries[i].cgpa;
        if (cgpa < 0.0 || cgpa > cgpa_scale) {
            diag_error(r->edu_entries[i].line, r->edu_entries[i].col,
                       "CGPA %.2f out of valid range (0-%g)", cgpa, cgpa_scale);
        }
    }

    /* ── Duplicate skills (case-insensitive) ─────────────── */
    for (int j = 0; j < r->skill_count; j++) {
        for (int i = 0; i < j; i++) {
            if (strcasecmp(r->skills[i].text, r->skills[j].text) == 0) {
                diag_error(r->skills[j].line, r->skills[j].col,
                           "duplicate skill \"%s\"", r->skills[j].text);
                break;
            }
        }
    }
}
