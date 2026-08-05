#ifndef TACC_COMPILE_H
#define TACC_COMPILE_H

#include "target/target.h"

struct tacc_compiler {
    struct tacc_target *target;
    struct tacc_type_list *basic_types;
};

#endif
