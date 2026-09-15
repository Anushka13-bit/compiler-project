/*
 * CLI entry point. Wires the six phases together:
 *
 *   lex+parse (flex/bison) -> semantic analysis -> IR lowering
 *   -> codegen (.tex) -> [build.c: pdflatex -> .pdf]
 *
 * main.c owns g_resume's lifetime and is the only place that decides
 * whether accumulated diagnostics should halt the pipeline.
 */
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "build.h"
#include "codegen.h"
#include "diagnostics.h"
#include "ir.h"
#include "parser.tab.h"
#include "semantic.h"

extern FILE *yyin;
extern Resume *g_resume;

static void print_usage(const char *prog) {
    fprintf(stderr,
            "usage: %s <input.resume> [-o <output.pdf>] [options]\n\n"
            "options:\n"
            "  -o, --output <path>   output PDF path (default: <input basename>.pdf)\n"
            "      --keep-tex        keep the generated .tex file\n"
            "      --tex-only        stop after code generation; skip pdflatex\n"
            "      --cgpa-scale <N>  valid CGPA scale, 10 or 4 (default: 10)\n"
            "      --dump-ast        print the parsed AST and exit (skips semantic "
            "analysis)\n"
            "  -h, --help            show this help\n",
            prog);
}

/* Swaps the extension of `path` for `new_ext` (which must include the
 * leading '.'). Caller owns the returned buffer. */
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
    bool keep_tex = false;
    bool tex_only = false;
    bool dump_ast = false;
    double cgpa_scale = 10.0;

    static struct option long_options[] = {
        {"output", required_argument, 0, 'o'}, {"keep-tex", no_argument, 0, 1},
        {"tex-only", no_argument, 0, 2},       {"cgpa-scale", required_argument, 0, 3},
        {"dump-ast", no_argument, 0, 4},       {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0},
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "o:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'o':
                output_pdf = optarg;
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

    if (optind >= argc) {
        fprintf(stderr, "error: missing input file\n\n");
        print_usage(argv[0]);
        return 1;
    }
    const char *input_path = argv[optind];

    yyin = fopen(input_path, "r");
    if (!yyin) {
        fprintf(stderr, "error: could not open input file '%s'\n", input_path);
        return 1;
    }

    char *default_output = NULL;
    if (!output_pdf) {
        default_output = replace_extension(input_path, ".pdf");
        output_pdf = default_output;
    }

    /* -- phase 1+2: lex + parse -------------------------------------- */
    g_resume = resume_new();
    yyparse();
    fclose(yyin);

    if (dump_ast) {
        if (diag_count() > 0) {
            diag_print_all();
        }
        resume_debug_print(g_resume);
        int status = diag_has_errors() ? 1 : 0;
        resume_free(g_resume);
        diag_reset();
        free(default_output);
        return status;
    }

    /* -- phase 3: semantic analysis ------------------------------------ */
    semantic_analyze(g_resume, cgpa_scale);

    if (diag_count() > 0) {
        diag_print_all();
    }
    if (diag_has_errors()) {
        resume_free(g_resume);
        diag_reset();
        free(default_output);
        return 1;
    }

    /* -- phase 4: IR generation -------------------------------------- */
    ResumeIR *ir = ir_lower(g_resume);
    resume_free(g_resume);
    diag_reset();

    /* -- phase 5: code generation -------------------------------------- */
    char *tex_path = replace_extension(output_pdf, ".tex");
    if (!codegen_write_tex(ir, tex_path)) {
        fprintf(stderr, "error: could not write '%s'\n", tex_path);
        ir_free(ir);
        free(tex_path);
        free(default_output);
        return 1;
    }
    ir_free(ir);

    if (tex_only) {
        printf("wrote %s\n", tex_path);
        free(tex_path);
        free(default_output);
        return 0;
    }

    /* -- phase 6 (outside the compiler core): pdflatex ------------------ */
    BuildOptions build_opts = {
        .tex_path = tex_path,
        .output_pdf = output_pdf,
        .keep_tex = keep_tex,
    };
    bool ok = build_pdf(&build_opts);
    free(tex_path);

    if (ok) {
        printf("wrote %s\n", output_pdf);
    }
    free(default_output);
    return ok ? 0 : 1;
}
