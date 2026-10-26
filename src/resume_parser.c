#include "resume_parser.h"

#include <ctype.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "job_matcher.h"

/* ── String Utilities ─────────────────────────────────── */

static void trim(char *s) {
    if (!s) return;
    char *p = s;
    while (isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);

    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
}

static bool read_line(const char *prompt, char *buf, size_t size) {
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(buf, (int)size, stdin)) {
        return false;
    }
    trim(buf);
    return true;
}

static bool ask_yes_no(const char *prompt, bool default_val) {
    char buf[64];
    while (1) {
        printf("%s [%s]: ", prompt, default_val ? "Y/n" : "y/N");
        fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) {
            return default_val;
        }
        trim(buf);
        if (buf[0] == '\0') {
            return default_val;
        }
        if (buf[0] == 'y' || buf[0] == 'Y') {
            return true;
        }
        if (buf[0] == 'n' || buf[0] == 'N') {
            return false;
        }
        printf("Please enter 'y' or 'n'.\n");
    }
}

static void write_escaped_string(FILE *fp, const char *str) {
    fputc('"', fp);
    for (const char *p = str; *p; p++) {
        if (*p == '"' || *p == '\\') {
            fputc('\\', fp);
        }
        fputc(*p, fp);
    }
    fputc('"', fp);
}

/* ── Helper: Regex Extraction ────────────────────────── */

static char *extract_regex_match(const char *text, const char *pattern, int group) {
    regex_t re;
    if (regcomp(&re, pattern, REG_EXTENDED | REG_ICASE) != 0) return NULL;

    regmatch_t matches[group + 1];
    char *result = NULL;
    if (regexec(&re, text, group + 1, matches, 0) == 0) {
        if (matches[group].rm_so != -1) {
            int len = matches[group].rm_eo - matches[group].rm_so;
            result = malloc(len + 1);
            if (result) {
                strncpy(result, text + matches[group].rm_so, len);
                result[len] = '\0';
                trim(result);
            }
        }
    }
    regfree(&re);
    return result;
}

static void sanitize_string(char *s) {
    if (!s) return;
    char *r = s;
    char *w = s;
    while (*r) {
        if ((unsigned char)r[0] == 0xC2 && (unsigned char)r[1] == 0x88) {
            r += 2;
        } else if ((unsigned char)r[0] == 0xCB && (unsigned char)r[1] == 0x86) {
            r += 2;
        } else if ((unsigned char)*r < 32 && *r != '\t' && *r != '\n') {
            r++;
        } else {
            *w++ = *r++;
        }
    }
    *w = '\0';
    trim(s);
}

static bool is_bullet_line(const char *line, const char **out_text) {
    if (!line) return false;
    const unsigned char *u = (const unsigned char *)line;
    bool match = false;
    int skip = 0;

    if (u[0] == 0xC2 && u[1] == 0x88) { match = true; skip = 2; }
    else if (u[0] == 0xCB && u[1] == 0x86) { match = true; skip = 2; }
    else if (u[0] == 0xE2 && u[1] == 0x80 && u[2] == 0xA2) { match = true; skip = 3; }
    else if (u[0] == 0xE2 && u[1] == 0x80 && (u[2] == 0x93 || u[2] == 0x94)) { match = true; skip = 3; }
    else if (u[0] == 0xC2 && u[1] == 0xB7) { match = true; skip = 2; }
    else if (u[0] == '*' || u[0] == '-' || u[0] == '^' || u[0] == '>') { match = true; skip = 1; }

    if (!match) return false;

    const char *p = line + skip;
    while (*p && !isalnum((unsigned char)*p) && *p != '(' && *p != '[' && *p != '"' && *p != '\'') p++;
    if (out_text) *out_text = p;
    return true;
}

/* ── PDF Text Extraction and Parsing ─────────────────── */

Resume *resume_load_from_pdf(const char *pdf_path) {
    if (!pdf_path) return NULL;

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "pdftotext \"%s\" -", pdf_path);
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        fprintf(stderr, "error: failed to run pdftotext on '%s'\n", pdf_path);
        return NULL;
    }

    Resume *r = resume_new();
    char line[1024];
    int line_num = 0;

    typedef enum {
        SEC_HEADER = 0,
        SEC_EDUCATION,
        SEC_SKILLS,
        SEC_PROJECTS,
        SEC_EXPERIENCE,
        SEC_CUSTOM
    } ParseSection;

    ParseSection current_sec = SEC_HEADER;
    char last_edu_college[256] = {0};
    double last_edu_cgpa = 0.0;
    bool has_edu_cgpa = false;

    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (line[0] == '\0' || line[0] == '\f') continue;

        /* Section Header Detection */
        if (strcasecmp(line, "Education") == 0) {
            if (last_edu_college[0] != '\0') {
                resume_add_edu_entry(r, last_edu_college, has_edu_cgpa ? last_edu_cgpa : 9.0, 0, 0);
                last_edu_college[0] = '\0';
                has_edu_cgpa = false;
            }
            current_sec = SEC_EDUCATION;
            continue;
        } else if (strcasecmp(line, "Skills") == 0) {
            if (last_edu_college[0] != '\0') {
                resume_add_edu_entry(r, last_edu_college, has_edu_cgpa ? last_edu_cgpa : 9.0, 0, 0);
                last_edu_college[0] = '\0';
                has_edu_cgpa = false;
            }
            current_sec = SEC_SKILLS;
            continue;
        } else if (strcasecmp(line, "Projects") == 0) {
            current_sec = SEC_PROJECTS;
            continue;
        } else if (strcasecmp(line, "Experience") == 0) {
            current_sec = SEC_EXPERIENCE;
            continue;
        }

        /* 1. Header parsing (Name and Contact details) */
        if (current_sec == SEC_HEADER) {
            line_num++;
            if (line_num == 1) {
                sanitize_string(line);
                r->name = strdup(line);
                continue;
            }

            /* Extract Email */
            if (!r->email) {
                char *email = extract_regex_match(line, "([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,})", 1);
                if (email) r->email = email;
            }

            /* Extract Phone */
            if (!r->phone) {
                char *phone = extract_regex_match(line, "([+]?[0-9][0-9 ()-]{6,18}[0-9])", 1);
                if (phone) r->phone = phone;
            }

            /* Extract GitHub */
            if (!r->github) {
                char *gh = extract_regex_match(line, "github\\.com/([a-zA-Z0-9_-]+)", 1);
                if (gh) r->github = gh;
            }

            /* Extract LinkedIn */
            if (!r->linkedin) {
                char *li = extract_regex_match(line, "linkedin\\.com/in/([a-zA-Z0-9_-]+)", 1);
                if (li) r->linkedin = li;
            }
            continue;
        }

        /* 2. Education */
        if (current_sec == SEC_EDUCATION) {
            char *p_cgpa = strcasestr(line, "CGPA");
            if (!p_cgpa) p_cgpa = strcasestr(line, "GPA");
            if (p_cgpa) {
                while (*p_cgpa && !isdigit((unsigned char)*p_cgpa)) p_cgpa++;
                if (*p_cgpa) {
                    last_edu_cgpa = strtod(p_cgpa, NULL);
                    has_edu_cgpa = true;
                    if (last_edu_college[0] != '\0') {
                        sanitize_string(last_edu_college);
                        resume_add_edu_entry(r, last_edu_college, last_edu_cgpa, 0, 0);
                        last_edu_college[0] = '\0';
                        has_edu_cgpa = false;
                    }
                }
            } else {
                if (last_edu_college[0] != '\0') {
                    sanitize_string(last_edu_college);
                    resume_add_edu_entry(r, last_edu_college, has_edu_cgpa ? last_edu_cgpa : 9.0, 0, 0);
                    has_edu_cgpa = false;
                }
                strncpy(last_edu_college, line, sizeof(last_edu_college) - 1);
            }
            continue;
        }

        /* 3. Skills */
        if (current_sec == SEC_SKILLS) {
            char buf[512];
            strncpy(buf, line, sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';
            char *tok = strtok(buf, ",;|");
            while (tok) {
                trim(tok);
                sanitize_string(tok);
                if (tok[0] != '\0') {
                    resume_add_skill(r, tok, 0, 0);
                }
                tok = strtok(NULL, ",;|");
            }
            continue;
        }

        /* 4. Projects */
        if (current_sec == SEC_PROJECTS) {
            const char *bullet_text = NULL;
            if (is_bullet_line(line, &bullet_text)) {
                if (r->project_count > 0 && bullet_text && *bullet_text) {
                    char clean[1024];
                    strncpy(clean, bullet_text, sizeof(clean) - 1);
                    clean[sizeof(clean) - 1] = '\0';
                    sanitize_string(clean);
                    if (clean[0] != '\0') {
                        resume_add_project_bullet(r, clean);
                    }
                }
            } else {
                sanitize_string(line);
                if (line[0] != '\0') {
                    resume_add_project(r, line, 0, 0);
                }
            }
            continue;
        }

        /* 5. Experience */
        if (current_sec == SEC_EXPERIENCE) {
            const char *bullet_text = NULL;
            if (is_bullet_line(line, &bullet_text)) {
                if (r->exp_count > 0 && bullet_text && *bullet_text) {
                    char clean[1024];
                    strncpy(clean, bullet_text, sizeof(clean) - 1);
                    clean[sizeof(clean) - 1] = '\0';
                    sanitize_string(clean);
                    if (clean[0] != '\0') {
                        resume_add_exp_bullet(r, clean);
                    }
                }
            } else {
                sanitize_string(line);
                /* Check if it's duration or company/role */
                char *date_match = extract_regex_match(line, "([A-Za-z]{3,9}[ ]+[0-9]{4}[ ]*[-–—][ ]*([A-Za-z]{3,9}[ ]+[0-9]{4}|Present))", 1);
                if (date_match && r->exp_count > 0) {
                    ExpEntry *last_e = &r->exp_entries[r->exp_count - 1];
                    if (!last_e->duration) {
                        last_e->duration = date_match;
                        continue;
                    }
                    free(date_match);
                }

                /* Split company and role if separated by — or - */
                char *dash = strstr(line, "—");
                if (!dash) dash = strstr(line, " - ");
                if (!dash) dash = strstr(line, " – ");

                if (dash) {
                    char comp[128] = {0};
                    int c_len = (int)(dash - line);
                    strncpy(comp, line, c_len < 127 ? c_len : 127);
                    trim(comp);

                    const char *role_ptr = dash + (strncmp(dash, "—", 3) == 0 ? 3 : 2);
                    while (*role_ptr && (!isalnum((unsigned char)*role_ptr))) role_ptr++;
                    char role[128] = {0};
                    strncpy(role, role_ptr, sizeof(role) - 1);
                    trim(role);

                    resume_add_exp_entry(r, comp, role[0] ? role : NULL, NULL, 0, 0);
                } else {
                    resume_add_exp_entry(r, line, NULL, NULL, 0, 0);
                }
            }
            continue;
        }
    }

    if (last_edu_college[0] != '\0') {
        resume_add_edu_entry(r, last_edu_college, has_edu_cgpa ? last_edu_cgpa : 9.0, 0, 0);
    }

    pclose(fp);
    return r;
}

Resume *resume_load_from_any(const char *path) {
    if (!path) return NULL;
    const char *ext = strrchr(path, '.');
    if (ext && strcasecmp(ext, ".pdf") == 0) {
        return resume_load_from_pdf(path);
    }
    return NULL;
}

/* ── DSL Serializer ──────────────────────────────────── */

static bool is_valid_ident(const char *s) {
    if (!s || !isalpha((unsigned char)s[0])) return false;
    for (int i = 1; s[i]; i++) {
        char c = s[i];
        if (!isalnum((unsigned char)c) && c != '_' && c != '.' && c != '+' && c != '#' && c != '-') {
            return false;
        }
    }
    return true;
}

bool resume_write_dsl(const Resume *r, const char *dsl_path) {
    if (!r || !dsl_path) return false;

    FILE *fp = fopen(dsl_path, "w");
    if (!fp) {
        fprintf(stderr, "error: could not write DSL file '%s'\n", dsl_path);
        return false;
    }

    if (r->name) {
        fprintf(fp, "name ");
        write_escaped_string(fp, r->name);
        fprintf(fp, "\n");
    }

    if (r->phone) {
        fprintf(fp, "phone ");
        write_escaped_string(fp, r->phone);
        fprintf(fp, "\n");
    }

    if (r->email) {
        fprintf(fp, "email ");
        write_escaped_string(fp, r->email);
        fprintf(fp, "\n");
    }

    if (r->github) {
        fprintf(fp, "github ");
        write_escaped_string(fp, r->github);
        fprintf(fp, "\n");
    }

    if (r->linkedin) {
        fprintf(fp, "linkedin ");
        write_escaped_string(fp, r->linkedin);
        fprintf(fp, "\n");
    }

    if (r->edu_count > 0) {
        fprintf(fp, "\neducation\n");
        for (int i = 0; i < r->edu_count; i++) {
            fprintf(fp, "college ");
            write_escaped_string(fp, r->edu_entries[i].college);
            fprintf(fp, "\ncgpa %.2f\n", r->edu_entries[i].cgpa);
        }
    }

    if (r->skill_count > 0) {
        fprintf(fp, "\nskills\n");
        for (int i = 0; i < r->skill_count; i++) {
            if (is_valid_ident(r->skills[i].text)) {
                fprintf(fp, "%s\n", r->skills[i].text);
            } else {
                write_escaped_string(fp, r->skills[i].text);
                fprintf(fp, "\n");
            }
        }
    }

    if (r->project_count > 0) {
        fprintf(fp, "\nprojects\n");
        for (int i = 0; i < r->project_count; i++) {
            fprintf(fp, "project ");
            write_escaped_string(fp, r->projects[i].name);
            fprintf(fp, "\n");
            for (int j = 0; j < r->projects[i].bullet_count; j++) {
                fprintf(fp, "bullet ");
                write_escaped_string(fp, r->projects[i].bullets[j]);
                fprintf(fp, "\n");
            }
        }
    }

    if (r->exp_count > 0) {
        fprintf(fp, "\nexperience\n");
        for (int i = 0; i < r->exp_count; i++) {
            fprintf(fp, "company ");
            write_escaped_string(fp, r->exp_entries[i].company);
            fprintf(fp, "\n");
            if (r->exp_entries[i].role) {
                fprintf(fp, "role ");
                write_escaped_string(fp, r->exp_entries[i].role);
                fprintf(fp, "\n");
            }
            if (r->exp_entries[i].duration) {
                fprintf(fp, "duration ");
                write_escaped_string(fp, r->exp_entries[i].duration);
                fprintf(fp, "\n");
            }
            for (int j = 0; j < r->exp_entries[i].bullet_count; j++) {
                fprintf(fp, "bullet ");
                write_escaped_string(fp, r->exp_entries[i].bullets[j]);
                fprintf(fp, "\n");
            }
        }
    }

    if (r->extra_count > 0) {
        for (int i = 0; i < r->extra_count; i++) {
            fprintf(fp, "\nextra ");
            write_escaped_string(fp, r->extras[i].title);
            fprintf(fp, "\n");
            for (int j = 0; j < r->extras[i].item_count; j++) {
                fprintf(fp, "item ");
                write_escaped_string(fp, r->extras[i].items[j].text);
                fprintf(fp, "\n");
                for (int k = 0; k < r->extras[i].items[j].sub_count; k++) {
                    fprintf(fp, "bullet ");
                    write_escaped_string(fp, r->extras[i].items[j].subbullets[k]);
                    fprintf(fp, "\n");
                }
            }
        }
    }

    fclose(fp);
    return true;
}

/* ── Targeted Tailoring Assistant ────────────────────── */

bool run_tailor_existing_resume(const char *resume_input_path, const char *job_source,
                               char **out_resume, char **out_pdf) {
    if (!resume_input_path) return false;

    printf("\n\033[1;36m======================================================================\033[0m\n");
    printf("\033[1;36m       🎯 RESUME TAILORING ASSISTANT: INCREMENTAL JOB ADAPTATION      \033[0m\n");
    printf("\033[1;36m======================================================================\033[0m\n");
    printf("Loading existing resume from: \033[1;37m%s\033[0m\n", resume_input_path);

    Resume *r = resume_load_from_any(resume_input_path);
    if (!r || !r->name) {
        fprintf(stderr, "\033[1;31merror: failed to parse existing resume content from '%s'\033[0m\n", resume_input_path);
        if (r) resume_free(r);
        return false;
    }

    /* Obtain target job description */
    char resolved_job[512] = {0};
    char *job_text = NULL;

    if (job_source && *job_source) {
        strncpy(resolved_job, job_source, sizeof(resolved_job) - 1);
        job_text = job_fetch_description(resolved_job);
    } else {
        read_line("\nEnter target job link (URL) or job file path:\n> ", resolved_job, sizeof(resolved_job));
        if (resolved_job[0] != '\0') {
            job_text = job_fetch_description(resolved_job);
        }
    }

    if (!job_text) {
        fprintf(stderr, "\033[1;31merror: could not fetch job description from target\033[0m\n");
        resume_free(r);
        return false;
    }

    /* Initial Job Gap Analysis */
    JobAnalysis *analysis = job_analyze_resume(r, job_text, resolved_job);

    printf("\n\033[1;35m┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\033[0m\n");
    printf("\033[1;35m┃             📄 EXISTING RESUME IMPORTED & ANALYZED                  ┃\033[0m\n");
    printf("\033[1;35m┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\033[0m\n");
    printf("\033[1;35m┃\033[0m Candidate: \033[1;37m%-56s\033[0m \033[1;35m┃\033[0m\n", r->name);
    printf("\033[1;35m┃\033[0m Target Job: %-56.56s \033[1;35m┃\033[0m\n", resolved_job);
    printf("\033[1;35m┃\033[0m Current Match Score: \033[1;33m%5.1f%%\033[0m (%d of %d skills covered)               \033[1;35m┃\033[0m\n",
           analysis->match_percentage, analysis->matched_count, analysis->job_skill_count);
    printf("\033[1;35m┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\033[0m\n");
    printf("\033[1;35m┃\033[0m \033[1;32m✓ Already in your resume (%d):\033[0m%*s\033[1;35m┃\033[0m\n",
           analysis->matched_count, (int)(40 - (analysis->matched_count >= 10 ? 1 : 0)), "");
    if (analysis->matched_count == 0) {
        printf("\033[1;35m┃\033[0m   (none yet)                                                        \033[1;35m┃\033[0m\n");
    } else {
        printf("\033[1;35m┃\033[0m   ");
        int col = 3;
        for (int i = 0; i < analysis->matched_count; i++) {
            int len = (int)strlen(analysis->matched_skills[i]) + 2;
            if (col + len > 66) {
                printf("%*s\033[1;35m┃\033[0m\n\033[1;35m┃\033[0m   ", 69 - col, "");
                col = 3;
            }
            printf("\033[32m•\033[0m %s%s", analysis->matched_skills[i], (i + 1 < analysis->matched_count) ? ", " : "");
            col += len;
        }
        printf("%*s\033[1;35m┃\033[0m\n", 69 - col, "");
    }
    printf("\033[1;35m┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\033[0m\n");
    printf("\033[1;35m┃\033[0m \033[1;31m⚠️  Missing Target Requirements (%d):\033[0m%*s\033[1;35m┃\033[0m\n",
           analysis->missing_count, (int)(34 - (analysis->missing_count >= 10 ? 1 : 0)), "");
    if (analysis->missing_count == 0) {
        printf("\033[1;35m┃\033[0m   \033[1;32m★ 100%% Match! No missing requirements found.\033[0m                     \033[1;35m┃\033[0m\n");
    } else {
        printf("\033[1;35m┃\033[0m   ");
        int col = 3;
        for (int i = 0; i < analysis->missing_count; i++) {
            int len = (int)strlen(analysis->missing_skills[i]) + 2;
            if (col + len > 66) {
                printf("%*s\033[1;35m┃\033[0m\n\033[1;35m┃\033[0m   ", 69 - col, "");
                col = 3;
            }
            printf("\033[31m✕\033[0m %s%s", analysis->missing_skills[i], (i + 1 < analysis->missing_count) ? ", " : "");
            col += len;
        }
        printf("%*s\033[1;35m┃\033[0m\n", 69 - col, "");
    }
    printf("\033[1;35m┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\033[0m\n");
    printf("\033[1;35m┃\033[0m \033[36m💡 All existing resume details (Name, Contact, Education, etc.)      \033[1;35m┃\033[0m\n");
    printf("\033[1;35m┃\033[0m \033[36m   stay 100%% the same. We will prompt ONLY for extra additions!      \033[1;35m┃\033[0m\n");
    printf("\033[1;35m┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\033[0m\n\n");

    /* ── STEP 1: Add Missing Skills ──────────────────────── */
    if (analysis->missing_count > 0) {
        printf("\033[1;33m[Area 1 / 3] Extra Technical Skills to Add:\033[0m\n");
        printf("Missing from job: ");
        for (int i = 0; i < analysis->missing_count; i++) {
            printf("[%s] ", analysis->missing_skills[i]);
        }
        printf("\n");

        bool add_s = ask_yes_no("Would you like to add any of these missing skills to your resume?", true);
        if (add_s) {
            printf("Enter skills to add (comma-separated, or type 'all', or press Enter to skip):\n> ");
            char add_buf[512] = {0};
            read_line("", add_buf, sizeof(add_buf));

            if (strcasecmp(add_buf, "all") == 0) {
                for (int i = 0; i < analysis->missing_count; i++) {
                    resume_add_skill(r, analysis->missing_skills[i], 0, 0);
                    printf("  \033[1;32m✓ Added skill:\033[0m %s\n", analysis->missing_skills[i]);
                }
            } else if (add_buf[0] != '\0') {
                char *tok = strtok(add_buf, ",");
                while (tok) {
                    trim(tok);
                    if (tok[0] != '\0') {
                        resume_add_skill(r, tok, 0, 0);
                        printf("  \033[1;32m✓ Added skill:\033[0m %s\n", tok);
                    }
                    tok = strtok(NULL, ",");
                }
            }
        }
    }

    /* ── STEP 2: Optional Project Highlight ──────────────── */
    printf("\n\033[1;33m[Area 2 / 3] Projects Enhancement:\033[0m\n");
    bool add_proj_bullet = ask_yes_no("Would you like to add a tailored bullet point to your projects?", false);
    if (add_proj_bullet) {
        if (r->project_count > 0) {
            printf("Existing Projects:\n");
            for (int i = 0; i < r->project_count; i++) {
                printf("  [%d] %s\n", i + 1, r->projects[i].name);
            }
            printf("  [%d] Add a brand new project\n", r->project_count + 1);

            char choice_str[32];
            read_line("Select project number: ", choice_str, sizeof(choice_str));
            int p_idx = atoi(choice_str);

            if (p_idx > 0 && p_idx <= r->project_count) {
                char bullet[512] = {0};
                read_line("Enter tailored bullet point: \n• ", bullet, sizeof(bullet));
                if (bullet[0] != '\0') {
                    Project *p = &r->projects[p_idx - 1];
                    p->bullets = realloc(p->bullets, (p->bullet_count + 1) * sizeof(char *));
                    p->bullets[p->bullet_count++] = strdup(bullet);
                    printf("  \033[1;32m✓ Bullet added to '%s'\033[0m\n", p->name);
                }
            } else {
                char p_name[256] = {0};
                read_line("New Project title: ", p_name, sizeof(p_name));
                if (p_name[0] != '\0') {
                    resume_add_project(r, p_name, 0, 0);
                    char bullet[512] = {0};
                    read_line("Enter project bullet: \n• ", bullet, sizeof(bullet));
                    if (bullet[0] != '\0') {
                        resume_add_project_bullet(r, bullet);
                    }
                    printf("  \033[1;32m✓ New project added!\033[0m\n");
                }
            }
        } else {
            char p_name[256] = {0};
            read_line("New Project title: ", p_name, sizeof(p_name));
            if (p_name[0] != '\0') {
                resume_add_project(r, p_name, 0, 0);
                char bullet[512] = {0};
                read_line("Enter project bullet: \n• ", bullet, sizeof(bullet));
                if (bullet[0] != '\0') {
                    resume_add_project_bullet(r, bullet);
                }
            }
        }
    }

    /* ── STEP 3: Optional Experience Enhancement ─────────── */
    printf("\n\033[1;33m[Area 3 / 3] Experience Enhancement:\033[0m\n");
    if (r->exp_count > 0) {
        bool add_exp_b = ask_yes_no("Would you like to add a tailored bullet to your work experience?", false);
        if (add_exp_b) {
            printf("Adding to: %s\n", r->exp_entries[r->exp_count - 1].company);
            char bullet[512] = {0};
            read_line("Enter experience bullet point: \n• ", bullet, sizeof(bullet));
            if (bullet[0] != '\0') {
                resume_add_exp_bullet(r, bullet);
                printf("  \033[1;32m✓ Bullet added to experience!\033[0m\n");
            }
        }
    }

    /* ── Updated Analysis & Final Report ─────────────────── */
    job_analysis_free(analysis);
    analysis = job_analyze_resume(r, job_text, resolved_job);

    printf("\n\033[1;32m======================================================================\033[0m\n");
    printf("\033[1;32m                  ★ UPDATED JOB COMPATIBILITY REPORT                  \033[0m\n");
    printf("\033[1;32m======================================================================\033[0m\n");
    job_print_report(analysis);

    /* Generate safe file names */
    char safe_name[128] = "resume";
    if (r->name) {
        int k = 0;
        for (int i = 0; r->name[i] && k < 120; i++) {
            if (isalnum((unsigned char)r->name[i])) {
                safe_name[k++] = (char)tolower((unsigned char)r->name[i]);
            } else if (r->name[i] == ' ' && k > 0 && safe_name[k - 1] != '_') {
                safe_name[k++] = '_';
            }
        }
        safe_name[k] = '\0';
    }

    char dsl_out[256], pdf_out[256];
    snprintf(dsl_out, sizeof(dsl_out), "%s_tailored.resume", safe_name);
    snprintf(pdf_out, sizeof(pdf_out), "%s_tailored.pdf", safe_name);

    resume_write_dsl(r, dsl_out);
    printf("✓ Updated tailored DSL saved to: \033[1;37m%s\033[0m\n", dsl_out);

    *out_resume = strdup(dsl_out);
    *out_pdf = strdup(pdf_out);

    job_analysis_free(analysis);
    resume_free(r);
    free(job_text);
    return true;
}
