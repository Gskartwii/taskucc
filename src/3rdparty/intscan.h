#ifndef THIRDPARTY_INTSCAN_H
#define THIRDPARTY_INTSCAN_H

#include "../soft_u64.h"
#include "../tasku_pp.h"

void intscan(struct tacc_file_iter *f,
             unsigned base,
             struct tacc_u64 *limit,
             struct tacc_u64 *out);

#endif
