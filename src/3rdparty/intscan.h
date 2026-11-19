#ifndef THIRDPARTY_INTSCAN_H
#define THIRDPARTY_INTSCAN_H

#include "../file_iter.h"
#include "../soft_u64.h"

void intscan(struct tacc_file_iter *f,
             unsigned base,
             struct tacc_u64 *limit,
             struct tacc_u64 *out);

#endif
