#ifndef THIRDPARTY_FLOATSCAN_H
#define THIRDPARTY_FLOATSCAN_H

#include "../soft_float.h"
#include "../tasku_pp.h"

void floatscan(struct tacc_file_iter *f, int prec, struct tacc_f128 *out);

#endif
