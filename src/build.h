#ifndef BUILD_H
#define BUILD_H

#include <stdbool.h>

typedef struct {
    const char *tex_path;
    const char *output_pdf;
    bool keep_tex;
} BuildOptions;

bool build_pdf(const BuildOptions *opts);

#endif
