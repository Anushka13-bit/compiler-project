#include "codegen.h"

#include <stdio.h>

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

bool codegen_write_tex(const Resume *r, const char *tex_path) {
    FILE *fp = fopen(tex_path, "w");
    if (!fp) {
        return false;
    }

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

    fputs("\\begin{center}\n    {\\Huge \\textbf{", fp);
    write_escaped(fp, r->name);
    fputs("}}", fp);
    if (r->email) {
        fputs(" \\\\[4pt]\n    ", fp);
        write_escaped(fp, r->email);
    }
    fputs("\n\\end{center}\n\n", fp);

    if (r->edu_count > 0) {
        fputs("\\resumesection{Education}\n", fp);
        for (int i = 0; i < r->edu_count; i++) {
            fputs("\\noindent\\textbf{", fp);
            write_escaped(fp, r->edu_entries[i].college);
            fprintf(fp, "} \\hfill CGPA: %.2f\\\\\n", r->edu_entries[i].cgpa);
        }
        fputs("\n", fp);
    }

    if (r->skill_count > 0) {
        fputs("\\resumesection{Skills}\n\\noindent ", fp);
        for (int i = 0; i < r->skill_count; i++) {
            write_escaped(fp, r->skills[i].text);
            if (i + 1 < r->skill_count) {
                fputs(", ", fp);
            }
        }
        fputs("\n\n", fp);
    }

    if (r->project_count > 0) {
        fputs("\\resumesection{Projects}\n", fp);
        for (int i = 0; i < r->project_count; i++) {
            fputs("\\noindent\\textbf{", fp);
            write_escaped(fp, r->projects[i].name);
            fputs("}", fp);
            if (r->projects[i].description) {
                fputs("\\\\\n\\noindent ", fp);
                write_escaped(fp, r->projects[i].description);
            }
            fputs("\\\\[4pt]\n", fp);
        }
        fputs("\n", fp);
    }

    fputs("\\end{document}\n", fp);
    fclose(fp);
    return true;
}
