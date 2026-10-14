#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdbool.h>

#include "ast.h"

bool codegen_write_tex(const Resume *r, const char *tex_path);

#endif
