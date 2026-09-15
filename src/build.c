#include "build.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* Absolute fallbacks to try if plain "pdflatex" isn't on $PATH -- covers
 * the common macOS LaTeX distributions (MacTeX/BasicTeX install here,
 * keg-only Homebrew LaTeX formulae symlink here too). */
static const char *PDFLATEX_FALLBACKS[] = {
    "/Library/TeX/texbin/pdflatex",
    "/usr/local/texlive/2025/bin/universal-darwin/pdflatex",
    "/usr/local/texlive/2024/bin/universal-darwin/pdflatex",
    "/opt/homebrew/bin/pdflatex",
    "/usr/local/bin/pdflatex",
};
#define PDFLATEX_FALLBACK_COUNT (int)(sizeof(PDFLATEX_FALLBACKS) / sizeof(PDFLATEX_FALLBACKS[0]))

static void split_dir_and_jobname(const char *tex_path, char *dir, size_t dir_size, char *jobname,
                                   size_t jobname_size) {
    const char *slash = strrchr(tex_path, '/');
    const char *base = slash ? slash + 1 : tex_path;

    if (slash) {
        size_t len = (size_t)(slash - tex_path);
        if (len >= dir_size) len = dir_size - 1;
        memcpy(dir, tex_path, len);
        dir[len] = '\0';
    } else {
        strncpy(dir, ".", dir_size);
    }

    strncpy(jobname, base, jobname_size - 1);
    jobname[jobname_size - 1] = '\0';
    char *dot = strrchr(jobname, '.');
    if (dot && strcmp(dot, ".tex") == 0) {
        *dot = '\0';
    }
}

/* Runs pdflatex once. Returns the child's exit status via *exit_code.
 * Returns true if pdflatex was found and executed (regardless of
 * whether the LaTeX compilation itself succeeded), false if no
 * pdflatex binary could be found at all. */
static bool run_pdflatex(const char *tex_path, const char *dir) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return false;
    }

    if (pid == 0) {
        /* Child: pdflatex writes its own <jobname>.log in -output-directory,
         * which is what we actually parse for errors -- so it's enough to
         * throw the noisy stdout/stderr chatter away here. */
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }

        char *argv[] = {(char *)"pdflatex", (char *)"-interaction=nonstopmode",
                         (char *)"-halt-on-error", (char *)"-output-directory", (char *)dir,
                         (char *)tex_path, NULL};

        execvp("pdflatex", argv);
        for (int i = 0; i < PDFLATEX_FALLBACK_COUNT; i++) {
            argv[0] = (char *)PDFLATEX_FALLBACKS[i];
            execv(PDFLATEX_FALLBACKS[i], argv);
        }
        _exit(127); /* sentinel: pdflatex could not be found anywhere */
    }

    int status = 0;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) == 127) {
        fprintf(stderr,
                "error: pdflatex not found. Install a LaTeX distribution (e.g. `brew install "
                "--cask basictex`) and make sure pdflatex is on your PATH.\n");
        return false;
    }
    return true;
}

/* Scans a pdflatex .log file for the first line starting with "! " and
 * prints it, instead of dumping the whole (often huge) log. */
static void report_first_latex_error(const char *log_path) {
    FILE *fp = fopen(log_path, "r");
    if (!fp) {
        fprintf(stderr, "error: LaTeX compilation failed (no log file was produced)\n");
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "! ", 2) == 0) {
            size_t len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') {
                line[len - 1] = '\0';
            }
            fprintf(stderr, "error: LaTeX compilation failed: %s\n", line);
            fclose(fp);
            return;
        }
    }
    fclose(fp);
    fprintf(stderr, "error: LaTeX compilation failed (see pdflatex output for details)\n");
}

static void remove_if_exists(const char *path) {
    /* Best-effort cleanup; a missing aux file (e.g. build failed before
     * pdflatex wrote one) is not itself an error. */
    unlink(path);
}

static bool move_file(const char *from, const char *to) {
    if (strcmp(from, to) == 0) {
        return true;
    }
    if (rename(from, to) == 0) {
        return true;
    }
    /* rename() fails across filesystems -- fall back to copy+delete. */
    FILE *src = fopen(from, "rb");
    if (!src) return false;
    FILE *dst = fopen(to, "wb");
    if (!dst) {
        fclose(src);
        return false;
    }
    char buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
        fwrite(buf, 1, n, dst);
    }
    fclose(src);
    fclose(dst);
    unlink(from);
    return true;
}

bool build_pdf(const BuildOptions *opts) {
    char dir[1024];
    char jobname[512];
    split_dir_and_jobname(opts->tex_path, dir, sizeof(dir), jobname, sizeof(jobname));

    char aux_path[1152], log_path[1152], out_path[1152], pdf_path[1152];
    snprintf(aux_path, sizeof(aux_path), "%s/%s.aux", dir, jobname);
    snprintf(log_path, sizeof(log_path), "%s/%s.log", dir, jobname);
    snprintf(out_path, sizeof(out_path), "%s/%s.out", dir, jobname);
    snprintf(pdf_path, sizeof(pdf_path), "%s/%s.pdf", dir, jobname);

    bool found = run_pdflatex(opts->tex_path, dir);
    if (!found) {
        return false;
    }

    FILE *pdf_check = fopen(pdf_path, "rb");
    bool pdf_exists = pdf_check != NULL;
    if (pdf_check) fclose(pdf_check);

    if (!pdf_exists) {
        report_first_latex_error(log_path);
        remove_if_exists(aux_path);
        remove_if_exists(log_path);
        remove_if_exists(out_path);
        return false;
    }

    bool moved = move_file(pdf_path, opts->output_pdf);
    remove_if_exists(aux_path);
    remove_if_exists(log_path);
    remove_if_exists(out_path);
    if (!opts->keep_tex) {
        remove_if_exists(opts->tex_path);
    }

    if (!moved) {
        fprintf(stderr, "error: could not move generated PDF to '%s'\n", opts->output_pdf);
        return false;
    }
    return true;
}
