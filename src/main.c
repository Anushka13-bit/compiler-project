#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ast.h"
#include "build.h"
#include "codegen.h"
#include "diagnostics.h"
#include "interactive.h"
#include "job_matcher.h"
#include "parser.tab.h"
#include "resume_parser.h"
#include "semantic.h"

extern FILE *yyin;
extern Resume *g_resume;

static void print_usage(const char *prog) {
    fprintf(stderr,
            "usage: %s [<input.resume> | -] [-o <output.pdf>] [options]\n"
            "       %s -i | --interactive [--job-url <url> | --job-desc <file>]\n"
            "       %s --resume-pdf <file.pdf> --job-url <url>\n\n"
            "options:\n"
            "  -i, --interactive       run interactive resume builder wizard\n"
            "  -o, --output <path>     output PDF path (default: <input basename>.pdf)\n"
            "      --resume-pdf <file> import existing resume PDF to tailor against job link\n"
            "      --job-url <url>     fetch target job posting and verify resume requirements\n"
            "      --job-desc <file>   analyze resume against local job description file\n"
            "      --keep-tex          keep the generated .tex file\n"
            "      --tex-only          stop after code generation; skip pdflatex\n"
            "      --cgpa-scale <N>    valid CGPA scale, 10 or 4 (default: 10)\n"
            "      --dump-ast          print the parsed AST and exit (skips semantic analysis)\n"
            "  -h, --help              show this help\n",
            prog, prog, prog);
}

//if path is a.resume -> a.pdf

static char *replace_extension(const char *path, const char *new_ext) {
    const char *dot = strrchr(path, '.');
    const char *slash = strrchr(path, '/');
    size_t base_len = (dot && (!slash || dot > slash)) ? (size_t)(dot - path) : strlen(path);

    char *result = malloc(base_len + strlen(new_ext) + 1);
    if (!result) {
        fprintf(stderr, "fatal: out of memory\n");
        exit(2);
    }
    memcpy(result, path, base_len);
    strcpy(result + base_len, new_ext);
    return result;
}

int main(int argc, char **argv) {
    const char *output_pdf = NULL;
    const char *job_source = NULL;
    const char *resume_pdf = NULL;
    const char *extract_target = NULL;
    const char *analyze_target = NULL;
    bool output_json = false;
    bool keep_tex = false;
    bool tex_only = false;
    bool dump_ast = false;
    bool interactive = false;
    double cgpa_scale = 10.0;
    
    //to add a new option, add it to the long_options array and add a case for it in the switch statement
    static struct option long_options[] = {
        {"output", required_argument, 0, 'o'},
        {"interactive", no_argument, 0, 'i'},
        {"job-url", required_argument, 0, 5},
        {"job-desc", required_argument, 0, 6},
        {"resume-pdf", required_argument, 0, 7},
        {"existing-resume", required_argument, 0, 7},
        {"extract-json", required_argument, 0, 8},
        {"analyze-job", required_argument, 0, 9},
        {"json", no_argument, 0, 'j'},
        {"keep-tex", no_argument, 0, 1},
        {"tex-only", no_argument, 0, 2},
        {"cgpa-scale", required_argument, 0, 3},
        {"dump-ast", no_argument, 0, 4},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0},
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "o:hij", long_options, NULL)) != -1) {
        switch (opt) {
            case 'o':
                output_pdf = optarg;
                break;
            case 'i':
                interactive = true;
                break;
            case 'j':
                output_json = true;
                break;
            case 5:
            case 6:
                job_source = optarg;
                break;
            case 7:
                resume_pdf = optarg;
                break;
            case 8:
                extract_target = optarg;
                break;
            case 9:
                analyze_target = optarg;
                break;
            case 1:
                keep_tex = true;
                break;
            case 2:
                tex_only = true;
                break;
            case 3: {
                int scale = atoi(optarg);
                if (scale != 10 && scale != 4) {
                    fprintf(stderr, "error: --cgpa-scale must be 10 or 4\n");
                    return 1;
                }
                cgpa_scale = (double)scale;
                break;
            }
            case 4:
                dump_ast = true;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (analyze_target) {
        char *job_text = job_fetch_description(analyze_target);
        if (!job_text) {
            if (output_json) {
                printf("{\"success\":false,\"error\":\"could not fetch job description\"}\n");
            } else {
                fprintf(stderr, "error: could not fetch job description from '%s'\n", analyze_target);
            }
            return 1;
        }
        Resume *dummy = resume_new();
        JobAnalysis *analysis = job_analyze_resume(dummy, job_text, analyze_target);
        if (output_json) {
            resume_print_json(stdout, NULL, analysis);
        } else {
            job_print_report(analysis);
        }
        job_analysis_free(analysis);
        resume_free(dummy);
        free(job_text);
        return 0;
    }

    if (extract_target) {
        Resume *r = resume_load_from_any(extract_target);
        if (!r) {
            if (output_json) {
                printf("{\"success\":false,\"error\":\"failed to parse resume from '%s'\"}\n", extract_target);
            } else {
                fprintf(stderr, "error: failed to parse resume from '%s'\n", extract_target);
            }
            return 1;
        }
        JobAnalysis *analysis = NULL;
        char *job_text = NULL;
        if (job_source) {
            job_text = job_fetch_description(job_source);
            if (job_text) {
                analysis = job_analyze_resume(r, job_text, job_source);
            }
        }
        if (output_json) {
            resume_print_json(stdout, r, analysis);
        } else {
            if (analysis) job_print_report(analysis);
            resume_debug_print(r);
        }
        if (analysis) job_analysis_free(analysis);
        free(job_text);
        resume_free(r);
        return 0;
    }

    char *wizard_resume = NULL;
    char *wizard_pdf = NULL;
    const char *input_path = NULL;

    if (resume_pdf) {
        if (!run_tailor_existing_resume(resume_pdf, job_source, &wizard_resume, &wizard_pdf)) {
            return 1;
        }
        input_path = wizard_resume;
        if (!output_pdf) {
            output_pdf = wizard_pdf;
        }
    } else if (interactive) {
        if (!run_interactive_wizard(&wizard_resume, &wizard_pdf, job_source)) {
            return 1;
        }
        input_path = wizard_resume;
        if (!output_pdf) {
            output_pdf = wizard_pdf;
        }
    } else if (optind < argc) {
        input_path = argv[optind];
    } else {
        if (isatty(STDIN_FILENO)) {
            if (!run_interactive_wizard(&wizard_resume, &wizard_pdf, job_source)) {
                return 1;
            }
            input_path = wizard_resume;
            if (!output_pdf) {
                output_pdf = wizard_pdf;
            }
        } else {
            fprintf(stderr, "error: missing input file\n\n");
            print_usage(argv[0]);
            return 1;
        }
    }

    if (strcmp(input_path, "-") == 0) {
        yyin = stdin;
    } else {
        yyin = fopen(input_path, "r");
        if (!yyin) {
            fprintf(stderr, "error: could not open input file '%s'\n", input_path);
            free(wizard_resume);
            free(wizard_pdf);
            return 1;
        }
    }

    char *default_output = NULL;
    if (!output_pdf) {
        if (strcmp(input_path, "-") == 0) {
            default_output = strdup("resume.pdf");
        } else {
            default_output = replace_extension(input_path, ".pdf");
        }
        output_pdf = default_output;
    }

    g_resume = resume_new();
    yyparse();
    if (yyin != stdin) {
        fclose(yyin);
    }

    if (dump_ast) {
        if (output_json) {
            resume_print_json(stdout, g_resume, NULL);
        } else {
            if (diag_count() > 0) {
                diag_print_all();
            }
            resume_debug_print(g_resume);
        }
        int status = diag_has_errors() ? 1 : 0;
        resume_free(g_resume);
        diag_reset();
        free(default_output);
        free(wizard_resume);
        free(wizard_pdf);
        return status;
    }

    semantic_analyze(g_resume, cgpa_scale);

    JobAnalysis *analysis = NULL;
    char *job_text = NULL;
    if (job_source) {
        job_text = job_fetch_description(job_source);
        if (job_text) {
            analysis = job_analyze_resume(g_resume, job_text, job_source);
            if (!output_json) {
                job_print_report(analysis);
            }
        } else if (!output_json) {
            fprintf(stderr, "warning: could not fetch/load job description from '%s'\n", job_source);
        }
    }

    if (diag_has_errors()) {
        if (output_json) {
            printf("{\"success\":false,\"diagnostics\":[\n");
            for (int i = 0; i < diag_count(); i++) {
                printf("  {\"line\": %d, \"col\": %d, \"message\": \"%s\"}%s\n",
                       diag_get_line(i), diag_get_col(i), diag_get_message(i),
                       (i + 1 < diag_count()) ? "," : "");
            }
            printf("]}\n");
        } else if (diag_count() > 0) {
            diag_print_all();
        }
        if (analysis) job_analysis_free(analysis);
        free(job_text);
        resume_free(g_resume);
        diag_reset();
        free(default_output);
        free(wizard_resume);
        free(wizard_pdf);
        return 1;
    } else if (diag_count() > 0 && !output_json) {
        diag_print_all();
    }

    char *tex_path = replace_extension(output_pdf, ".tex");
    if (!codegen_write_tex(g_resume, tex_path)) {
        fprintf(stderr, "error: could not write '%s'\n", tex_path);
        if (analysis) job_analysis_free(analysis);
        free(job_text);
        resume_free(g_resume);
        free(tex_path);
        free(default_output);
        free(wizard_resume);
        free(wizard_pdf);
        return 1;
    }

    if (tex_only) {
        if (output_json) {
            printf("{\n  \"success\": true,\n  \"texPath\": \"%s\"\n}\n", tex_path);
        } else {
            printf("wrote %s\n", tex_path);
        }
        if (analysis) job_analysis_free(analysis);
        free(job_text);
        resume_free(g_resume);
        diag_reset();
        free(tex_path);
        free(default_output);
        free(wizard_resume);
        free(wizard_pdf);
        return 0;
    }

    BuildOptions build_opts = {
        .tex_path = tex_path,
        .output_pdf = output_pdf,
        .keep_tex = keep_tex,
    };
    bool ok = build_pdf(&build_opts);

    if (output_json) {
        printf("{\n  \"success\": %s,\n  \"outputPdf\": \"%s\",\n  \"texPath\": \"%s\"",
               ok ? "true" : "false", output_pdf, tex_path);
        if (analysis) {
            printf(",\n  \"matchScore\": %.1f,\n", analysis->match_percentage);
            printf("  \"matchedCount\": %d,\n", analysis->matched_count);
            printf("  \"missingCount\": %d,\n", analysis->missing_count);
            printf("  \"jobSkillCount\": %d", analysis->job_skill_count);
        }
        printf(",\n  \"diagnostics\": []\n}\n");
    } else if (ok) {
        printf("wrote %s\n", output_pdf);
    }

    if (analysis) job_analysis_free(analysis);
    free(job_text);
    resume_free(g_resume);
    diag_reset();
    free(tex_path);
    free(default_output);
    free(wizard_resume);
    free(wizard_pdf);
    return ok ? 0 : 1;
}
