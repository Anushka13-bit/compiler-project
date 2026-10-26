#include "interactive.h"

#include <ctype.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

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

//create resume.dsl and pdf file
bool run_interactive_wizard(char **out_resume_path, char **out_pdf_path) {
    char name[256]     = {0};
    char email[256]    = {0};
    char phone[128]    = {0};
    char github[256]   = {0};
    char linkedin[256] = {0};

    printf("\n=== resumec Interactive Resume Builder ===\n");
    printf("Answer the following questions to create your customized resume.\n\n");

    /* 1. Name */
    while (1) {
        if (!read_line("Enter your full name: ", name, sizeof(name))) {
            return false;
        }
        if (name[0] != '\0') break;
        printf("Name cannot be empty.\n");
    }

    /* 2. Contact info */

    /* Phone — digits, spaces, +, -, () only; 7-15 digits total */
    {
        regex_t re;
        /* allow optional leading +, then digits/spaces/parens/hyphens, 7..15 total digits */
        regcomp(&re, "^[+]?[0-9 ()\\-]{6,19}$", REG_EXTENDED | REG_NOSUB);
        while (1) {
            read_line("Phone number (press Enter to skip): ", phone, sizeof(phone));
            if (phone[0] == '\0') break;
            /* count only digits */
            int digits = 0;
            for (const char *p = phone; *p; p++) if (*p >= '0' && *p <= '9') digits++;
            if (regexec(&re, phone, 0, NULL, 0) == 0 && digits >= 7 && digits <= 15) break;
            printf("  Invalid phone number.\n"
                   "  Expected: 7-15 digits, optionally with +, spaces, hyphens, or parentheses.\n"
                   "  Example: +91 98765 43210  or  (212) 555-0100\n"
                   "  Please re-enter your phone number.\n");
        }
        regfree(&re);
    }

    /* Email — validate format if provided */
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

    /* GitHub username — alphanumeric and hyphens only, no slashes or spaces */
    {
        regex_t re;
        regcomp(&re, "^[A-Za-z0-9]([A-Za-z0-9-]{0,37}[A-Za-z0-9])?$", REG_EXTENDED | REG_NOSUB);
        while (1) {
            read_line("GitHub username (press Enter to skip): ", github, sizeof(github));
            if (github[0] == '\0') break;
            if (regexec(&re, github, 0, NULL, 0) == 0) break;
            printf("  Invalid GitHub username.\n"
                   "  Only letters, digits, and hyphens; must not start or end with a hyphen.\n"
                   "  Do NOT include 'github.com/' — just the username (e.g. octocat).\n"
                   "  Please re-enter your GitHub username.\n");
        }
        regfree(&re);
    }

    /* LinkedIn username — alphanumeric, hyphens, underscores; no slashes */
    {
        regex_t re;
        regcomp(&re, "^[A-Za-z0-9_-]{3,100}$", REG_EXTENDED | REG_NOSUB);
        while (1) {
            read_line("LinkedIn username (press Enter to skip): ", linkedin, sizeof(linkedin));
            if (linkedin[0] == '\0') break;
            if (regexec(&re, linkedin, 0, NULL, 0) == 0) break;
            printf("  Invalid LinkedIn username.\n"
                   "  Only letters, digits, hyphens, and underscores (3-100 chars).\n"
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
    read_line("Enter skills separated by commas (e.g. C++, Python, Git, Docker):\n> ",
              skills_buf, sizeof(skills_buf));

    /* Parse skills — reject duplicates immediately */
    char *skills[64];
    int skill_count = 0;
    char *token = strtok(skills_buf, ",");
    while (token && skill_count < 64) {
        trim(token);
        if (token[0] != '\0') {
            /* Check for duplicates (case-insensitive) */
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

    /* 5. Projects */
    typedef struct {
        char title[256];
        char bullets[8][512];
        int bullet_count;
    } ProjectInput;
    ProjectInput projects[16];
    int project_count = 0;

    printf("\n-- Projects Section --\n");
    while (project_count < 16) {
        bool add = ask_yes_no(project_count == 0 ? "Add a project entry?" : "Add another project entry?",
                              project_count == 0);
        if (!add) break;

        while (1) {
            read_line("  Project title: ", projects[project_count].title, sizeof(projects[project_count].title));
            if (projects[project_count].title[0] != '\0') break;
            printf("  Project title cannot be empty.\n");
        }

        projects[project_count].bullet_count = 0;
        printf("  Add bullet points (press Enter with no text to stop):\n");
        while (projects[project_count].bullet_count < 8) {
            char bul[512];
            read_line("    bullet> ", bul, sizeof(bul));
            if (bul[0] == '\0') break;
            strncpy(projects[project_count].bullets[projects[project_count].bullet_count],
                    bul, 511);
            projects[project_count].bullets[projects[project_count].bullet_count][511] = '\0';
            projects[project_count].bullet_count++;
        }
        project_count++;
    }

    /* 6. Experience */
    typedef struct {
        char company[256];
        char role[256];
        char duration[256];
        char bullets[8][512];
        int bullet_count;
    } ExpInput;
    ExpInput exps[16];
    int exp_count = 0;

    printf("\n-- Experience Section --\n");
    while (exp_count < 16) {
        bool add = ask_yes_no(exp_count == 0 ? "Add an experience entry?" : "Add another experience entry?",
                              false);
        if (!add) break;

        while (1) {
            read_line("  Company name: ", exps[exp_count].company, sizeof(exps[exp_count].company));
            if (exps[exp_count].company[0] != '\0') break;
            printf("  Company name cannot be empty.\n");
        }

        read_line("  Role / Job title (press Enter to skip): ",
                  exps[exp_count].role, sizeof(exps[exp_count].role));
        read_line("  Duration (e.g. Jun 2024 - Aug 2024, press Enter to skip): ",
                  exps[exp_count].duration, sizeof(exps[exp_count].duration));

        exps[exp_count].bullet_count = 0;
        printf("  Add bullet points (press Enter with no text to stop):\n");
        while (exps[exp_count].bullet_count < 8) {
            char bul[512];
            read_line("    bullet> ", bul, sizeof(bul));
            if (bul[0] == '\0') break;
            strncpy(exps[exp_count].bullets[exps[exp_count].bullet_count], bul, 511);
            exps[exp_count].bullets[exps[exp_count].bullet_count][511] = '\0';
            exps[exp_count].bullet_count++;
        }
        exp_count++;
    }

    /* 7. Custom / extra sections (Publications, Awards, Certifications, etc.) */
    typedef struct {
        char title[128];
        struct {
            char text[512];
            char subbullets[4][512];
            int sub_count;
        } items[32];
        int item_count;
    } ExtraSection;
    ExtraSection extras[8];
    int extra_count = 0;

    printf("\n-- Custom Sections --\n");
    printf("You can add extra sections such as Publications, Awards, Certifications, etc.\n");
    while (extra_count < 8) {
        bool add = ask_yes_no(
            extra_count == 0 ? "Add a custom section?" : "Add another custom section?",
            false);
        if (!add) break;

        while (1) {
            read_line("  Section title (e.g. Publications, Awards): ",
                      extras[extra_count].title, sizeof(extras[extra_count].title));
            if (extras[extra_count].title[0] != '\0') break;
            printf("  Section title cannot be empty.\n");
        }

        extras[extra_count].item_count = 0;
        printf("  Add entries (press Enter with no text to stop):\n");
        while (extras[extra_count].item_count < 32) {
            int idx = extras[extra_count].item_count;
            char entry[512];
            read_line("    entry> ", entry, sizeof(entry));
            if (entry[0] == '\0') break;
            strncpy(extras[extra_count].items[idx].text, entry, 511);
            extras[extra_count].items[idx].text[511] = '\0';
            extras[extra_count].items[idx].sub_count = 0;
            /* optional sub-bullets for this entry */
            printf("    Add sub-bullets for this entry (press Enter to skip):\n");
            while (extras[extra_count].items[idx].sub_count < 4) {
                int si = extras[extra_count].items[idx].sub_count;
                char sub[512];
                read_line("      sub> ", sub, sizeof(sub));
                if (sub[0] == '\0') break;
                strncpy(extras[extra_count].items[idx].subbullets[si], sub, 511);
                extras[extra_count].items[idx].subbullets[si][511] = '\0';
                extras[extra_count].items[idx].sub_count++;
            }
            extras[extra_count].item_count++;
        }
        extra_count++;
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
            for (int b = 0; b < projects[i].bullet_count; b++) {
                fprintf(fp, "bullet ");
                write_escaped_dsl_string(fp, projects[i].bullets[b]);
                fprintf(fp, "\n");
            }
        }
    }

    if (exp_count > 0) {
        fprintf(fp, "\nexperience\n");
        for (int i = 0; i < exp_count; i++) {
            fprintf(fp, "company ");
            write_escaped_dsl_string(fp, exps[i].company);
            fprintf(fp, "\n");
            if (exps[i].role[0] != '\0') {
                fprintf(fp, "role ");
                write_escaped_dsl_string(fp, exps[i].role);
                fprintf(fp, "\n");
            }
            if (exps[i].duration[0] != '\0') {
                fprintf(fp, "duration ");
                write_escaped_dsl_string(fp, exps[i].duration);
                fprintf(fp, "\n");
            }
            for (int b = 0; b < exps[i].bullet_count; b++) {
                fprintf(fp, "bullet ");
                write_escaped_dsl_string(fp, exps[i].bullets[b]);
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
    printf("\nSuccessfully generated DSL file: %s\n", chosen_resume);

    *out_resume_path = strdup(chosen_resume);
    *out_pdf_path = strdup(chosen_pdf);
    return true;
}
