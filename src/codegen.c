#include "codegen.h"

#include <stdio.h>
#include <string.h>

//escape special characters in the text so it is not printed as a command
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

    /* Preamble — hyperref for clickable links in the PDF */
    fputs(
        "\\documentclass[11pt]{article}\n"
        "\\usepackage[margin=0.75in]{geometry}\n"
        "\\usepackage[hidelinks]{hyperref}\n"
        "\\pagestyle{empty}\n"
        "\\setlength{\\parindent}{0pt}\n"
        "\\newcommand{\\resumesection}[1]{%\n"
        "  \\vspace{10pt}\\noindent{\\large\\bfseries #1}\\\\[-6pt]\n"
        "  \\noindent\\rule{\\linewidth}{0.6pt}\\vspace{4pt}\n"
        "}\n"
        "\\begin{document}\n\n",
        fp);

    /* ── Header ─────────────────────────────────────────── */
    fputs("\\begin{center}\n    {\\Huge \\textbf{", fp);
    write_escaped(fp, r->name);
    fputs("}}", fp);

    /* Contact line: phone | email | github | linkedin — only non-NULL ones */
    int contact_items = (r->phone    ? 1 : 0) +
                        (r->email    ? 1 : 0) +
                        (r->github   ? 1 : 0) +
                        (r->linkedin ? 1 : 0);
    if (contact_items > 0) {
        fputs(" \\\\[4pt]\n    ", fp);
        int printed = 0;
        if (r->phone) {
            write_escaped(fp, r->phone);
            printed++;
        }
        if (r->email) {
            if (printed) fputs(" $\\mid$ ", fp);
            fprintf(fp, "\\href{mailto:");
            write_escaped(fp, r->email);
            fputs("}{", fp);
            write_escaped(fp, r->email);
            fputs("}", fp);
            printed++;
        }
        if (r->github) {
            if (printed) fputs(" $\\mid$ ", fp);
            fprintf(fp, "\\href{https://github.com/");
            /* write raw for URL, escaped for display text */
            write_escaped(fp, r->github);
            fputs("}{github.com/", fp);
            write_escaped(fp, r->github);
            fputs("}", fp);
            printed++;
        }
        if (r->linkedin) {
            if (printed) fputs(" $\\mid$ ", fp);
            fprintf(fp, "\\href{https://linkedin.com/in/");
            write_escaped(fp, r->linkedin);
            fputs("}{linkedin.com/in/", fp);
            write_escaped(fp, r->linkedin);
            fputs("}", fp);
        }
    }
    fputs("\n\\end{center}\n\n", fp);

    /* ── Education ──────────────────────────────────────── */
    if (r->edu_count > 0) {
        fputs("\\resumesection{Education}\n", fp);
        for (int i = 0; i < r->edu_count; i++) {
            fputs("\\noindent\\textbf{", fp);
            write_escaped(fp, r->edu_entries[i].college);
            fprintf(fp, "} \\hfill CGPA: %.2f\\\\\n", r->edu_entries[i].cgpa);
        }
        fputs("\n", fp);
    }

    /* ── Skills ─────────────────────────────────────────── */
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

    /* ── Projects ───────────────────────────────────────── */
    if (r->project_count > 0) {
        fputs("\\resumesection{Projects}\n", fp);
        for (int i = 0; i < r->project_count; i++) {
            fputs("\\noindent\\textbf{", fp);
            write_escaped(fp, r->projects[i].name);
            fputs("}\n", fp);
            if (r->projects[i].bullet_count > 0) {
                fputs("\\begin{itemize}\\setlength{\\itemsep}{1pt}"
                      "\\setlength{\\parskip}{0pt}\n", fp);
                for (int j = 0; j < r->projects[i].bullet_count; j++) {
                    fputs("  \\item ", fp);
                    write_escaped(fp, r->projects[i].bullets[j]);
                    fputs("\n", fp);
                }
                fputs("\\end{itemize}\n", fp);
            } else {
                fputs("\\\\\n", fp);
            }
        }
        fputs("\n", fp);
    }

    /* ── Experience ─────────────────────────────────────── */
    if (r->exp_count > 0) {
        fputs("\\resumesection{Experience}\n", fp);
        for (int i = 0; i < r->exp_count; i++) {
            fputs("\\noindent\\textbf{", fp);
            write_escaped(fp, r->exp_entries[i].company);
            fputs("}", fp);
            if (r->exp_entries[i].role) {
                fputs(" --- ", fp);
                write_escaped(fp, r->exp_entries[i].role);
            }
            if (r->exp_entries[i].duration) {
                fputs(" \\hfill ", fp);
                write_escaped(fp, r->exp_entries[i].duration);
            }
            fputs("\n", fp);
            if (r->exp_entries[i].bullet_count > 0) {
                fputs("\\begin{itemize}\\setlength{\\itemsep}{1pt}"
                      "\\setlength{\\parskip}{0pt}\n", fp);
                for (int j = 0; j < r->exp_entries[i].bullet_count; j++) {
                    fputs("  \\item ", fp);
                    write_escaped(fp, r->exp_entries[i].bullets[j]);
                    fputs("\n", fp);
                }
                fputs("\\end{itemize}\n", fp);
            } else {
                fputs("\\\\\n", fp);
            }
        }
        fputs("\n", fp);
    }

    fputs("\\end{document}\n", fp);
    fclose(fp);
    return true;
}
