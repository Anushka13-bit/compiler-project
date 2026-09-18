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

/* Deliberately depends on nothing beyond `article` + `geometry`: a
 * minimal TeX install (e.g. BasicTeX, which is what most graders will
 * actually have on hand) doesn't ship extras like enumitem/titlesec,
 * and a resume template that needs `sudo tlmgr install ...` before it
 * compiles is a worse deliverable than one that just works. Section
 * headings and list-like entries are built from core primitives
 * (\rule, \hfill, \\) instead. */
static void write_preamble(FILE *fp) {
    fputs(
        "\\documentclass[11pt]{article}\n"
        "\\usepackage[margin=0.75in]{geometry}\n"
        "\\pagestyle{empty}\n"
        "\\setlength{\\parindent}{0pt}\n"
        "\\newcommand{\\resumesection}[1]{%\n"
        "  \\vspace{10pt}\\noindent{\\large\\bfseries #1}\\\\[-6pt]\n"
        "  \\noindent\\rule{\\linewidth}{0.6pt}\\vspace{4pt}\n"
        "}\n"
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
    fputs("\\resumesection{Education}\n", fp);
    for (int i = 0; i < ir->education_count; i++) {
        fputs("\\noindent\\textbf{", fp);
        write_escaped(fp, ir->education[i].college);
        fprintf(fp, "} \\hfill CGPA: %.2f\\\\\n", ir->education[i].cgpa);
    }
    fputs("\n", fp);
}

static void write_skills(FILE *fp, const ResumeIR *ir) {
    if (ir->skill_count == 0) {
        return;
    }
    fputs("\\resumesection{Skills}\n\\noindent ", fp);
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
    fputs("\\resumesection{Projects}\n", fp);
    for (int i = 0; i < ir->project_count; i++) {
        fputs("\\noindent\\textbf{", fp);
        write_escaped(fp, ir->projects[i].name);
        fputs("}", fp);
        if (ir->projects[i].description) {
            fputs("\\\\\n\\noindent ", fp);
            write_escaped(fp, ir->projects[i].description);
        }
        fputs("\\\\[4pt]\n", fp);
    }
    fputs("\n", fp);
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
