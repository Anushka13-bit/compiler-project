#include "interactive.h"

#include <ctype.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "ast.h"
#include "job_matcher.h"
#include "resume_parser.h"

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

static void make_safe_filename(const char *src, char *dst, size_t dst_size) {
    size_t j = 0;
    for (size_t i = 0; src[i] && j + 1 < dst_size; i++) {
        unsigned char c = (unsigned char)src[i];
        if (isalnum(c)) {
            dst[j++] = (char)tolower(c);
        } else if (c == ' ' || c == '_' || c == '-') {
            if (j > 0 && dst[j - 1] != '_') {
                dst[j++] = '_';
            }
        }
    }
    if (j == 0 && dst_size > 1) {
        dst[j++] = 'r';
        dst[j++] = 'e';
        dst[j++] = 's';
        dst[j++] = 'u';
        dst[j++] = 'm';
        dst[j++] = 'e';
    }
    dst[j] = '\0';
}

static void write_escaped_dsl_string(FILE *fp, const char *str) {
    fputc('"', fp);
    for (const char *p = str; *p; p++) {
        if (*p == '"' || *p == '\\') {
            fputc('\\', fp);
        }
        fputc(*p, fp);
    }
    fputc('"', fp);
}

//create resume.dsl and pdf file with optional target job tailoring
bool run_interactive_wizard(char **out_resume_path, char **out_pdf_path, const char *initial_job_source) {
    char name[256]     = {0};
    char email[256]    = {0};
    char phone[128]    = {0};
    char github[256]   = {0};
    char linkedin[256] = {0};

    char job_source[512] = {0};
    char *job_text = NULL;

    printf("\n\033[1;36m=== resumec Interactive Resume Builder ===\033[0m\n");
    printf("Create your ATS-optimized resume tailored to your target opportunities.\n\n");

    bool has_existing = ask_yes_no("Do you have an existing resume PDF to upload and tailor?", false);
    if (has_existing) {
        char existing_path[512] = {0};
        read_line("Enter path to your existing resume PDF (e.g. resume.pdf): ", existing_path, sizeof(existing_path));
        if (existing_path[0] != '\0') {
            return run_tailor_existing_resume(existing_path, initial_job_source, out_resume_path, out_pdf_path);
        }
    }

    /* ── Target Job Application Setup (Before Entering Details) ── */
    char *target_skills[64] = {0};
    int target_skill_count = 0;

    if (initial_job_source && initial_job_source[0] != '\0') {
        strncpy(job_source, initial_job_source, sizeof(job_source) - 1);
        printf("\033[1;33m[+] Target Job Posting Specified:\033[0m %s\n", job_source);
        printf("    Fetching and extracting requirements...\n");
        job_text = job_fetch_description(job_source);
    } else {
        bool tailor = ask_yes_no("Would you like to tailor this resume to a specific job posting/URL?", true);
        if (tailor) {
            read_line("Enter Job Link (URL) or job file path: ", job_source, sizeof(job_source));
            if (job_source[0] != '\0') {
                printf("--> Fetching and scraping job requirements from: %s...\n", job_source);
                job_text = job_fetch_description(job_source);
            }
        }
    }

    /* If job text was retrieved, extract & preview job requirements BEFORE asking user details */
    if (job_text) {
        Resume *dummy = resume_new();
        JobAnalysis *prev = job_analyze_resume(dummy, job_text, job_source);
        if (prev && prev->job_skill_count > 0) {
            target_skill_count = prev->job_skill_count < 64 ? prev->job_skill_count : 64;
            for (int i = 0; i < target_skill_count; i++) {
                target_skills[i] = strdup(prev->job_skills[i]);
            }
            job_print_target_profile(job_source, target_skills, target_skill_count);
        } else {
            printf("\033[1;33mNote: Job text retrieved, but no standard keywords matched.\033[0m\n\n");
        }
        job_analysis_free(prev);
        resume_free(dummy);
    } else if (job_source[0] != '\0') {
        printf("\033[1;31mCould not fetch job description from '%s'. Proceeding in standard mode.\033[0m\n\n", job_source);
    }

    /* 1. Name */
    while (1) {
        if (!read_line("Enter your full name: ", name, sizeof(name))) {
            for (int i = 0; i < target_skill_count; i++) free(target_skills[i]);
            free(job_text);
            return false;
        }
        if (name[0] != '\0') break;
        printf("Name cannot be empty.\n");
    }

    /* 2. Contact info */

    /* Phone */
    {
        regex_t re;
        regcomp(&re, "^[+]?[0-9 ()\\-]{6,19}$", REG_EXTENDED | REG_NOSUB);
        while (1) {
            read_line("Phone number (press Enter to skip): ", phone, sizeof(phone));
            if (phone[0] == '\0') break;
            int digits = 0;
            for (const char *p = phone; *p; p++) if (*p >= '0' && *p <= '9') digits++;
            if (regexec(&re, phone, 0, NULL, 0) == 0 && digits >= 7 && digits <= 15) break;
            printf("  Invalid phone number format.\n"
                   "  Expected 7-15 digits, optionally with +, spaces, hyphens, or parentheses\n"
                   "  (e.g. \"+91 98765 43210\" or \"(212) 555-0100\").\n"
                   "  Please re-enter your phone number.\n");
        }
        regfree(&re);
    }

    /* Email */
    {
        regex_t re;
        const char *email_pattern = "^[^@[:space:]]+@[^@[:space:]]+\\.[^@[:space:]]+$";
        regcomp(&re, email_pattern, REG_EXTENDED | REG_NOSUB);
        while (1) {
            read_line("Email address (press Enter to skip): ", email, sizeof(email));
            if (email[0] == '\0') break;
            if (regexec(&re, email, 0, NULL, 0) == 0) break;
            printf("  Invalid email format. Expected format: user@domain.tld\n"
                   "  Please re-enter your email address.\n");
        }
        regfree(&re);
    }

    /* GitHub username */
    {
        regex_t re;
        regcomp(&re, "^[A-Za-z0-9]([A-Za-z0-9-]{0,37}[A-Za-z0-9])?$", REG_EXTENDED | REG_NOSUB);
        while (1) {
            read_line("GitHub username (press Enter to skip): ", github, sizeof(github));
            if (github[0] == '\0') break;
            if (regexec(&re, github, 0, NULL, 0) == 0) break;
            printf("  Invalid GitHub username.\n"
                   "  Only letters, digits, and hyphens are allowed (cannot start/end with a hyphen).\n"
                   "  Do NOT include 'github.com/' — just the username (e.g. octocat).\n"
                   "  Please re-enter your GitHub username.\n");
        }
        regfree(&re);
    }

    /* LinkedIn username */
    {
        regex_t re;
        regcomp(&re, "^[A-Za-z0-9_-]{3,100}$", REG_EXTENDED | REG_NOSUB);
        while (1) {
            read_line("LinkedIn username (press Enter to skip): ", linkedin, sizeof(linkedin));
            if (linkedin[0] == '\0') break;
            if (regexec(&re, linkedin, 0, NULL, 0) == 0) break;
            printf("  Invalid LinkedIn username.\n"
                   "  Use 3-100 characters: letters, digits, hyphens, and underscores only.\n"
                   "  Do NOT include 'linkedin.com/in/' — just the username (e.g. john-doe).\n"
                   "  Please re-enter your LinkedIn username.\n");
        }
        regfree(&re);
    }

    /* 3. Education */
    typedef struct {
        char college[256];
        double cgpa;
    } EduInput;
    EduInput edu[16];
    int edu_count = 0;

    printf("\n-- Education Section --\n");
    while (edu_count < 16) {
        bool add = ask_yes_no(edu_count == 0 ? "Add an education entry?" : "Add another education entry?",
                              edu_count == 0);
        if (!add) break;

        while (1) {
            read_line("  College / University name: ", edu[edu_count].college, sizeof(edu[edu_count].college));
            if (edu[edu_count].college[0] != '\0') break;
            printf("  College name cannot be empty.\n");
        }

        while (1) {
            char cgpa_str[64];
            read_line("  CGPA (e.g. 9.3 or 3.8): ", cgpa_str, sizeof(cgpa_str));
            char *endptr = NULL;
            double val = strtod(cgpa_str, &endptr);
            if (endptr != cgpa_str && val >= 0.0) {
                edu[edu_count].cgpa = val;
                break;
            }
            printf("  Please enter a valid positive number for CGPA.\n");
        }
        edu_count++;
    }

    /* 4. Skills */
    char skills_buf[1024] = {0};
    printf("\n-- Skills Section --\n");
    if (target_skill_count > 0) {
        printf("\033[1;33m┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\033[0m\n");
        printf("\033[1;33m┃ 💡 TARGET JOB SKILLS TO COVER:                                      ┃\033[0m\n");
        printf("\033[1;33m┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\033[0m\n");
        printf("\033[1;33m┃\033[0m ");
        int col = 2;
        for (int i = 0; i < target_skill_count; i++) {
            int len = (int)strlen(target_skills[i]) + 2;
            if (col + len > 66) {
                printf("%*s\033[1;33m┃\033[0m\n\033[1;33m┃\033[0m ", 69 - col, "");
                col = 2;
            }
            printf("\033[1;37m[%s]\033[0m ", target_skills[i]);
            col += len + 1;
        }
        printf("%*s\033[1;33m┃\033[0m\n", 69 - col, "");
        printf("\033[1;33m┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\033[0m\n");
        printf("👉 \033[1;36mSuggestion:\033[0m Include any of these technologies you have worked with to boost your ATS match!\n");
    }
    read_line("Enter skills separated by commas (e.g. C++, Python, Git, Docker):\n> ",
              skills_buf, sizeof(skills_buf));

    /* Parse skills — reject duplicates immediately */
    char *skills[128];
    int skill_count = 0;
    char *token = strtok(skills_buf, ",");
    while (token && skill_count < 128) {
        trim(token);
        if (token[0] != '\0') {
            bool is_dup = false;
            for (int k = 0; k < skill_count; k++) {
                if (strcasecmp(skills[k], token) == 0) {
                    printf("  Warning: duplicate skill \"%s\" ignored.\n", token);
                    is_dup = true;
                    break;
                }
            }
            if (!is_dup) {
                skills[skill_count++] = strdup(token);
            }
        }
        token = strtok(NULL, ",");
    }

    /* Live feedback on entered skills vs target job with re-prompt loop */
    if (target_skill_count > 0) {
        int matched = 0;
        printf("\n\033[1;32m✓ Matched from Target Job (%d):\033[0m ", target_skill_count);
        for (int i = 0; i < target_skill_count; i++) {
            bool has = false;
            for (int s = 0; s < skill_count; s++) {
                if (strcasecmp(skills[s], target_skills[i]) == 0) { has = true; break; }
            }
            if (has) {
                printf("\033[1;32m[%s]\033[0m ", target_skills[i]);
                matched++;
            }
        }
        if (matched == 0) printf("(none yet)");
        printf("\n");

        if (matched < target_skill_count) {
            printf("\033[1;33m⚠️  Still Missing from Job Requirements:\033[0m ");
            for (int i = 0; i < target_skill_count; i++) {
                bool has = false;
                for (int s = 0; s < skill_count; s++) {
                    if (strcasecmp(skills[s], target_skills[i]) == 0) { has = true; break; }
                }
                if (!has) {
                    printf("\033[33m[%s]\033[0m ", target_skills[i]);
                }
            }
            printf("\n");
        }

        /* If skills are still missing, prompt user to add skills according to the job requirements */
        while (matched < target_skill_count) {
            bool want_more = ask_yes_no("\nWould you like to enter additional skills to match these target job requirements?", true);
            if (!want_more) break;

            printf("Enter additional skills (comma-separated, or type 'all' to include all remaining target skills):\n> ");
            char more_buf[1024] = {0};
            read_line("", more_buf, sizeof(more_buf));
            if (more_buf[0] == '\0') break;

            if (strcasecmp(more_buf, "all") == 0) {
                for (int i = 0; i < target_skill_count && skill_count < 128; i++) {
                    bool has = false;
                    for (int s = 0; s < skill_count; s++) {
                        if (strcasecmp(skills[s], target_skills[i]) == 0) { has = true; break; }
                    }
                    if (!has) {
                        skills[skill_count++] = strdup(target_skills[i]);
                        printf("  \033[1;32m✓ Added:\033[0m %s\n", target_skills[i]);
                    }
                }
            } else {
                char *tok = strtok(more_buf, ",");
                while (tok && skill_count < 128) {
                    trim(tok);
                    if (tok[0] != '\0') {
                        bool is_dup = false;
                        for (int k = 0; k < skill_count; k++) {
                            if (strcasecmp(skills[k], tok) == 0) {
                                is_dup = true;
                                break;
                            }
                        }
                        if (!is_dup) {
                            skills[skill_count++] = strdup(tok);
                            printf("  \033[1;32m✓ Added:\033[0m %s\n", tok);
                        }
                    }
                    tok = strtok(NULL, ",");
                }
            }

            /* Recalculate matched skills */
            matched = 0;
            printf("\n\033[1;32m✓ Matched from Target Job (%d):\033[0m ", target_skill_count);
            for (int i = 0; i < target_skill_count; i++) {
                bool has = false;
                for (int s = 0; s < skill_count; s++) {
                    if (strcasecmp(skills[s], target_skills[i]) == 0) { has = true; break; }
                }
                if (has) {
                    printf("\033[1;32m[%s]\033[0m ", target_skills[i]);
                    matched++;
                }
            }
            if (matched == 0) printf("(none yet)");
            printf("\n");

            if (matched < target_skill_count) {
                printf("\033[1;33m⚠️  Still Missing from Job Requirements:\033[0m ");
                for (int i = 0; i < target_skill_count; i++) {
                    bool has = false;
                    for (int s = 0; s < skill_count; s++) {
                        if (strcasecmp(skills[s], target_skills[i]) == 0) { has = true; break; }
                    }
                    if (!has) {
                        printf("\033[33m[%s]\033[0m ", target_skills[i]);
                    }
                }
                printf("\n");
            } else {
                printf("\033[1;32m★ Excellent! You have covered 100%% of the target job requirements in your skills!\033[0m\n");
            }
        }
    }

    /* 5. Projects */
    typedef struct {
        char title[256];
        char bullets[8][512];
        int bullet_count;
    } ProjectInput;
    ProjectInput projects[16];
    int project_count = 0;

    printf("\n-- Projects Section --\n");
    if (target_skill_count > 0) {
        printf("\033[1;33m💡 Project Suggestion:\033[0m Highlight projects that applied key job requirements (e.g. ");
        for (int i = 0; i < target_skill_count && i < 4; i++) {
            printf("\033[1;37m%s\033[0m%s", target_skills[i], (i + 1 < target_skill_count && i + 1 < 4) ? ", " : "");
        }
        printf(") in your descriptions and bullet points!\n");
    }
    while (project_count < 16) {
        bool add = ask_yes_no(project_count == 0 ? "Add a project entry?" : "Add another project entry?",
                              project_count == 0);
        if (!add) break;

        while (1) {
            read_line("  Project name: ", projects[project_count].title, sizeof(projects[project_count].title));
            if (projects[project_count].title[0] != '\0') break;
            printf("  Project name cannot be empty.\n");
        }

        projects[project_count].bullet_count = 0;
        printf("  Enter bullet points describing the project (press Enter on empty line to finish):\n");
        while (projects[project_count].bullet_count < 8) {
            char bbuf[512] = {0};
            read_line("  • ", bbuf, sizeof(bbuf));
            if (bbuf[0] == '\0') break;
            strncpy(projects[project_count].bullets[projects[project_count].bullet_count], bbuf, 511);
            projects[project_count].bullets[projects[project_count].bullet_count][511] = '\0';
            projects[project_count].bullet_count++;
        }
        project_count++;
    }

    /* 6. Experience */
    typedef struct {
        char company[256];
        char role[256];
        char duration[128];
        char bullets[8][512];
        int bullet_count;
    } ExpInput;
    ExpInput exp[16];
    int exp_count = 0;

    printf("\n-- Experience Section --\n");
    if (target_skill_count > 0) {
        printf("\033[1;33m💡 Experience Suggestion:\033[0m Frame your work achievements around skills expected by the role.\n");
    }
    while (exp_count < 16) {
        bool add = ask_yes_no(exp_count == 0 ? "Add an experience entry?" : "Add another experience entry?", false);
        if (!add) break;

        while (1) {
            read_line("  Company name: ", exp[exp_count].company, sizeof(exp[exp_count].company));
            if (exp[exp_count].company[0] != '\0') break;
            printf("  Company name cannot be empty.\n");
        }

        read_line("  Role / Title (optional, press Enter to skip): ", exp[exp_count].role, sizeof(exp[exp_count].role));
        read_line("  Duration (e.g. \"Jun 2023 - Aug 2023\", optional): ", exp[exp_count].duration, sizeof(exp[exp_count].duration));

        exp[exp_count].bullet_count = 0;
        printf("  Enter bullet points for this role (press Enter on empty line to finish):\n");
        while (exp[exp_count].bullet_count < 8) {
            char bbuf[512] = {0};
            read_line("  • ", bbuf, sizeof(bbuf));
            if (bbuf[0] == '\0') break;
            strncpy(exp[exp_count].bullets[exp[exp_count].bullet_count], bbuf, 511);
            exp[exp_count].bullets[exp[exp_count].bullet_count][511] = '\0';
            exp[exp_count].bullet_count++;
        }
        exp_count++;
    }

    /* 7. Extra / Custom sections */
    typedef struct {
        char text[512];
        char subbullets[4][512];
        int sub_count;
    } ExtraItemInput;
    typedef struct {
        char title[256];
        ExtraItemInput items[16];
        int item_count;
    } ExtraSectionInput;

    ExtraSectionInput extras[8];
    int extra_count = 0;

    printf("\n-- Extra Sections (Awards, Publications, Certifications, etc.) --\n");
    while (extra_count < 8) {
        bool add = ask_yes_no("Add a custom section (e.g. Publications, Awards, Certifications)?", false);
        if (!add) break;

        while (1) {
            read_line("  Section Title: ", extras[extra_count].title, sizeof(extras[extra_count].title));
            if (extras[extra_count].title[0] != '\0') break;
            printf("  Section title cannot be empty.\n");
        }

        extras[extra_count].item_count = 0;
        printf("  Enter items for this section (press Enter on empty item to finish):\n");
        while (extras[extra_count].item_count < 16) {
            char ibuf[512] = {0};
            read_line("  Item: ", ibuf, sizeof(ibuf));
            if (ibuf[0] == '\0') break;
            int idx = extras[extra_count].item_count;
            strncpy(extras[extra_count].items[idx].text, ibuf, 511);
            extras[extra_count].items[idx].text[511] = '\0';
            extras[extra_count].items[idx].sub_count = 0;

            /* optional subbullets */
            while (extras[extra_count].items[idx].sub_count < 4) {
                char sub[512] = {0};
                read_line("    - sub-bullet (optional, press Enter to finish item): ", sub, sizeof(sub));
                if (sub[0] == '\0') break;
                int si = extras[extra_count].items[idx].sub_count;
                strncpy(extras[extra_count].items[idx].subbullets[si], sub, 511);
                extras[extra_count].items[idx].subbullets[si][511] = '\0';
                extras[extra_count].items[idx].sub_count++;
            }
            extras[extra_count].item_count++;
        }
        extra_count++;
    }

    /* ── Target Job Verification & Gap Analysis ─────────── */
    if (job_text) {
        Resume *temp_r = resume_new();
        temp_r->name = strdup(name);
        if (email[0]) temp_r->email = strdup(email);
        for (int i = 0; i < skill_count; i++) {
            resume_add_skill(temp_r, skills[i], 0, 0);
        }
        for (int i = 0; i < project_count; i++) {
            resume_add_project(temp_r, projects[i].title, 0, 0);
            for (int j = 0; j < projects[i].bullet_count; j++) {
                resume_add_project_bullet(temp_r, projects[i].bullets[j]);
            }
        }
        for (int i = 0; i < exp_count; i++) {
            resume_add_exp_entry(temp_r, exp[i].company,
                                 exp[i].role[0] ? exp[i].role : NULL,
                                 exp[i].duration[0] ? exp[i].duration : NULL, 0, 0);
            for (int j = 0; j < exp[i].bullet_count; j++) {
                resume_add_exp_bullet(temp_r, exp[i].bullets[j]);
            }
        }
        for (int i = 0; i < extra_count; i++) {
            resume_add_extra_section(temp_r, extras[i].title, 0, 0);
            for (int j = 0; j < extras[i].item_count; j++) {
                resume_add_extra_item(temp_r, extras[i].items[j].text);
                for (int k = 0; k < extras[i].items[j].sub_count; k++) {
                    resume_add_extra_subbullet(temp_r, extras[i].items[j].subbullets[k]);
                }
            }
        }

        JobAnalysis *analysis = job_analyze_resume(temp_r, job_text, job_source);
        job_print_report(analysis);

        /* Prompt user if missing skills should be added */
        if (analysis->missing_count > 0) {
            printf("\033[1;33mActionable Recommendation:\033[0m Your resume is currently missing %d skill(s) mentioned in the job requirements.\n",
                   analysis->missing_count);
            bool add_missing = ask_yes_no("Would you like to add any of these missing skills to your resume now?", true);
            if (add_missing) {
                printf("Enter missing skills to add (comma-separated, or type 'all' to add all):\n> ");
                char to_add[512] = {0};
                read_line("", to_add, sizeof(to_add));

                if (strcasecmp(to_add, "all") == 0) {
                    for (int k = 0; k < analysis->missing_count && skill_count < 128; k++) {
                        skills[skill_count++] = strdup(analysis->missing_skills[k]);
                        printf("  \033[1;32m✓ Added skill:\033[0m %s\n", analysis->missing_skills[k]);
                    }
                } else if (to_add[0] != '\0') {
                    char *tok = strtok(to_add, ",");
                    while (tok && skill_count < 128) {
                        trim(tok);
                        if (tok[0] != '\0') {
                            bool is_dup = false;
                            for (int s = 0; s < skill_count; s++) {
                                if (strcasecmp(skills[s], tok) == 0) {
                                    is_dup = true;
                                    break;
                                }
                            }
                            if (!is_dup) {
                                skills[skill_count++] = strdup(tok);
                                printf("  \033[1;32m✓ Added skill:\033[0m %s\n", tok);
                            }
                        }
                        tok = strtok(NULL, ",");
                    }
                }

                /* Re-run analysis to show new score */
                resume_free(temp_r);
                job_analysis_free(analysis);

                temp_r = resume_new();
                temp_r->name = strdup(name);
                for (int i = 0; i < skill_count; i++) resume_add_skill(temp_r, skills[i], 0, 0);
                for (int i = 0; i < project_count; i++) {
                    resume_add_project(temp_r, projects[i].title, 0, 0);
                    for (int j = 0; j < projects[i].bullet_count; j++) resume_add_project_bullet(temp_r, projects[i].bullets[j]);
                }
                for (int i = 0; i < exp_count; i++) {
                    resume_add_exp_entry(temp_r, exp[i].company, exp[i].role[0] ? exp[i].role : NULL,
                                         exp[i].duration[0] ? exp[i].duration : NULL, 0, 0);
                    for (int j = 0; j < exp[i].bullet_count; j++) resume_add_exp_bullet(temp_r, exp[i].bullets[j]);
                }
                analysis = job_analyze_resume(temp_r, job_text, job_source);
                printf("\n\033[1;32m★ Updated Job Match Score:\033[0m\n");
                job_print_report(analysis);
            }
        }

        job_analysis_free(analysis);
        resume_free(temp_r);
        free(job_text);
    }

    /* 8. Output filenames */
    char safe_base[256];
    make_safe_filename(name, safe_base, sizeof(safe_base));

    char default_resume[300], default_pdf[300];
    snprintf(default_resume, sizeof(default_resume), "%s.resume", safe_base);
    snprintf(default_pdf, sizeof(default_pdf), "%s.pdf", safe_base);

    printf("\n-- Output Configuration --\n");
    char chosen_resume[512];
    char prompt_resume[350];
    snprintf(prompt_resume, sizeof(prompt_resume), "File path to save .resume DSL [%s]: ", default_resume);
    read_line(prompt_resume, chosen_resume, sizeof(chosen_resume));
    if (chosen_resume[0] == '\0') {
        strncpy(chosen_resume, default_resume, sizeof(chosen_resume));
    }

    char chosen_pdf[512];
    char prompt_pdf[350];
    snprintf(prompt_pdf, sizeof(prompt_pdf), "Output PDF file path [%s]: ", default_pdf);
    read_line(prompt_pdf, chosen_pdf, sizeof(chosen_pdf));
    if (chosen_pdf[0] == '\0') {
        strncpy(chosen_pdf, default_pdf, sizeof(chosen_pdf));
    }

    /* Write the DSL file */
    FILE *fp = fopen(chosen_resume, "w");
    if (!fp) {
        fprintf(stderr, "error: could not write to file '%s'\n", chosen_resume);
        for (int i = 0; i < skill_count; i++) free(skills[i]);
        return false;
    }

    fprintf(fp, "name ");
    write_escaped_dsl_string(fp, name);
    fprintf(fp, "\n");

    if (phone[0] != '\0') {
        fprintf(fp, "phone ");
        write_escaped_dsl_string(fp, phone);
        fprintf(fp, "\n");
    }

    if (email[0] != '\0') {
        fprintf(fp, "email ");
        write_escaped_dsl_string(fp, email);
        fprintf(fp, "\n");
    }

    if (github[0] != '\0') {
        fprintf(fp, "github ");
        write_escaped_dsl_string(fp, github);
        fprintf(fp, "\n");
    }

    if (linkedin[0] != '\0') {
        fprintf(fp, "linkedin ");
        write_escaped_dsl_string(fp, linkedin);
        fprintf(fp, "\n");
    }

    if (edu_count > 0) {
        fprintf(fp, "\neducation\n");
        for (int i = 0; i < edu_count; i++) {
            fprintf(fp, "college ");
            write_escaped_dsl_string(fp, edu[i].college);
            fprintf(fp, "\ncgpa %.2f\n", edu[i].cgpa);
        }
    }

    if (skill_count > 0) {
        fprintf(fp, "\nskills\n");
        for (int i = 0; i < skill_count; i++) {
            fprintf(fp, "%s\n", skills[i]);
            free(skills[i]);
        }
    }

    if (project_count > 0) {
        fprintf(fp, "\nprojects\n");
        for (int i = 0; i < project_count; i++) {
            fprintf(fp, "project ");
            write_escaped_dsl_string(fp, projects[i].title);
            fprintf(fp, "\n");
            for (int j = 0; j < projects[i].bullet_count; j++) {
                fprintf(fp, "bullet ");
                write_escaped_dsl_string(fp, projects[i].bullets[j]);
                fprintf(fp, "\n");
            }
        }
    }

    if (exp_count > 0) {
        fprintf(fp, "\nexperience\n");
        for (int i = 0; i < exp_count; i++) {
            fprintf(fp, "company ");
            write_escaped_dsl_string(fp, exp[i].company);
            fprintf(fp, "\n");
            if (exp[i].role[0] != '\0') {
                fprintf(fp, "role ");
                write_escaped_dsl_string(fp, exp[i].role);
                fprintf(fp, "\n");
            }
            if (exp[i].duration[0] != '\0') {
                fprintf(fp, "duration ");
                write_escaped_dsl_string(fp, exp[i].duration);
                fprintf(fp, "\n");
            }
            for (int j = 0; j < exp[i].bullet_count; j++) {
                fprintf(fp, "bullet ");
                write_escaped_dsl_string(fp, exp[i].bullets[j]);
                fprintf(fp, "\n");
            }
        }
    }

    /* Custom / extra sections */
    for (int i = 0; i < extra_count; i++) {
        fprintf(fp, "\nextra ");
        write_escaped_dsl_string(fp, extras[i].title);
        fprintf(fp, "\n");
        for (int j = 0; j < extras[i].item_count; j++) {
            fprintf(fp, "item ");
            write_escaped_dsl_string(fp, extras[i].items[j].text);
            fprintf(fp, "\n");
            for (int k = 0; k < extras[i].items[j].sub_count; k++) {
                fprintf(fp, "bullet ");
                write_escaped_dsl_string(fp, extras[i].items[j].subbullets[k]);
                fprintf(fp, "\n");
            }
        }
    }

    fclose(fp);
    printf("\n\033[1;32m✓ Successfully generated DSL file: %s\033[0m\n", chosen_resume);

    for (int i = 0; i < target_skill_count; i++) {
        free(target_skills[i]);
    }

    *out_resume_path = strdup(chosen_resume);
    *out_pdf_path = strdup(chosen_pdf);
    return true;
}
