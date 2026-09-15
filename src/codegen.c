#include "codegen.h"

#include <stdio.h>

/* Writes `text` to fp with LaTeX's special characters escaped, so that
 * user-provided strings (project names, skill names like "C++", etc.)
 * can never break the surrounding document or -- worse -- inject
 * arbitrary LaTeX. Anything not explicitly listed here passes through
 * unchanged. */
static void write_escaped(FILE *fp, const char *text) {
    for (const char *p = text; *p; p++) {
        switch (*p) {
            case '&': fputs("\\&", fp); break;
            case '%': fputs("\\%", fp); break;
            case '$': fputs("\\$", fp); break;
            case '#': fputs("\\#", fp); break;
            case '_': fputs("\\_", fp); break;
            case '{': fputs("\\{", fp); break;
            case '}': fputs("\\}", fp); break;
            case '~': fputs("\\textasciitilde{}", fp); break;
            case '^': fputs("\\textasciicircum{}", fp); break;
            case '\\': fputs("\\textbackslash{}", fp); break;
            default: fputc(*p, fp); break;
        }
    }
}

static void write_preamble(FILE *fp) {
    fputs(
        "\\documentclass[11pt]{article}\n"
        "\\usepackage[margin=0.75in]{geometry}\n"
        "\\usepackage{enumitem}\n"
        "\\usepackage{titlesec}\n"
        "\\pagestyle{empty}\n"
        "\\setlength{\\parindent}{0pt}\n"
        "\\titleformat{\\section}{\\large\\bfseries}{}{0em}{}[\\titlerule]\n"
        "\\titlespacing{\\section}{0pt}{10pt}{6pt}\n"
        "\\begin{document}\n\n",
        fp);
}

static void write_header(FILE *fp, const ResumeIR *ir) {
    fputs("\\begin{center}\n    {\\Huge \\textbf{", fp);
    write_escaped(fp, ir->name);
    fputs("}}", fp);
    if (ir->email) {
        fputs(" \\\\[4pt]\n    ", fp);
        write_escaped(fp, ir->email);
    }
    fputs("\n\\end{center}\n\n", fp);
}

static void write_education(FILE *fp, const ResumeIR *ir) {
    if (ir->education_count == 0) {
        return;
    }
    fputs("\\section*{Education}\n\\begin{itemize}[leftmargin=*, label={}]\n", fp);
    for (int i = 0; i < ir->education_count; i++) {
        fputs("  \\item \\textbf{", fp);
        write_escaped(fp, ir->education[i].college);
        fprintf(fp, "} \\hfill CGPA: %.2f\n", ir->education[i].cgpa);
    }
    fputs("\\end{itemize}\n\n", fp);
}

static void write_skills(FILE *fp, const ResumeIR *ir) {
    if (ir->skill_count == 0) {
        return;
    }
    fputs("\\section*{Skills}\n", fp);
    for (int i = 0; i < ir->skill_count; i++) {
        write_escaped(fp, ir->skills[i]);
        if (i + 1 < ir->skill_count) {
            fputs(", ", fp);
        }
    }
    fputs("\n\n", fp);
}

static void write_projects(FILE *fp, const ResumeIR *ir) {
    if (ir->project_count == 0) {
        return;
    }
    fputs("\\section*{Projects}\n\\begin{itemize}[leftmargin=*, label={}]\n", fp);
    for (int i = 0; i < ir->project_count; i++) {
        fputs("  \\item \\textbf{", fp);
        write_escaped(fp, ir->projects[i].name);
        fputs("}", fp);
        if (ir->projects[i].description) {
            fputs(" \\\\\n  ", fp);
            write_escaped(fp, ir->projects[i].description);
        }
        fputs("\n", fp);
    }
    fputs("\\end{itemize}\n\n", fp);
}

bool codegen_write_tex(const ResumeIR *ir, const char *tex_path) {
    FILE *fp = fopen(tex_path, "w");
    if (!fp) {
        return false;
    }

    write_preamble(fp);
    write_header(fp, ir);
    write_education(fp, ir);
    write_skills(fp, ir);
    write_projects(fp, ir);

    fputs("\\end{document}\n", fp);
    fclose(fp);
    return true;
}
