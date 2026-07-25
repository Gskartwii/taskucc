#ifndef TASKU_FORMAT_H
#define TASKU_FORMAT_H

#include "parse.h"

struct tacc_formatter {
    size_t indent;
};

void tacc_format_ast(struct tacc_formatter *fmt, struct tacc_ast *ast);

#endif
