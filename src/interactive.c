#include "interactive.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

bool run_interactive_wizard(char **out_resume_path, char **out_pdf_path) {
    char name[256] = {0};
    char email[256] = {0};

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

    /* 2. Email */
    read_line("Enter your email address (press Enter to skip): ", email, sizeof(email));

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

    /* Parse skills */
    char *skills[64];
    int skill_count = 0;
    char *token = strtok(skills_buf, ",");
    while (token && skill_count < 64) {
        trim(token);
        if (token[0] != '\0') {
            skills[skill_count++] = strdup(token);
        }
        token = strtok(NULL, ",");
    }

    /* 5. Projects */
    typedef struct {
        char title[256];
        char desc[512];
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

        read_line("  Description (optional, press Enter to skip): ",
                  projects[project_count].desc, sizeof(projects[project_count].desc));
        project_count++;
    }

    /* 6. Output filenames */
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

    if (email[0] != '\0') {
        fprintf(fp, "email ");
        write_escaped_dsl_string(fp, email);
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
            if (projects[i].desc[0] != '\0') {
                fprintf(fp, "description ");
                write_escaped_dsl_string(fp, projects[i].desc);
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
