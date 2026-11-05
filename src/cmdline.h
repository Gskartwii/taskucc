#ifndef TACC_CMDLINE_H
#define TACC_CMDLINE_H

#include "dynstring.h"
#include "string_list.h"
#include "util.h"
#include <string.h>

#define ARG_SHORT_APPEND(ch, to_list)                    \
    do {                                                 \
        if (*arg == ch) {                                \
            arg = arg + 1;                               \
            if (!*arg) {                                 \
                i = i + 1;                               \
                tacc_assert(ASSERT_DIAG,                 \
                            i < count,                   \
                            "incomplete argument -%s\n", \
                            arg - 1);                    \
                arg = argv[i];                           \
            }                                            \
            str = tacc_dynstring_new();                  \
            tacc_dynstring_concat(str, arg);             \
            tacc_string_list_push(to_list, str);         \
            goto next;                                   \
        }                                                \
    } while (0)
#define ARG_SHORT_SET(ch, to_place)                      \
    do {                                                 \
        if (*arg == ch) {                                \
            arg = arg + 1;                               \
            if (!*arg) {                                 \
                i = i + 1;                               \
                tacc_assert(ASSERT_DIAG,                 \
                            i < count,                   \
                            "incomplete argument -%s\n", \
                            arg - 1);                    \
                arg = argv[i];                           \
            }                                            \
            str = tacc_dynstring_new();                  \
            tacc_dynstring_concat(str, arg);             \
            if (to_place != NULL) {                      \
                tacc_dynstring_free(to_place);           \
            }                                            \
            to_place = str;                              \
            goto next;                                   \
        }                                                \
    } while (0)
#define ARG_SHORT_SET_FLAG(ch, flag)                           \
    do {                                                       \
        if (*arg == ch) {                                      \
            arg = arg + 1;                                     \
            tacc_assert(ASSERT_DIAG,                           \
                        (*arg) == 0,                           \
                        "cannot take content for flag: -%s\n", \
                        arg - 1);                              \
            flag = 1;                                          \
            goto next;                                         \
        }                                                      \
    } while (0)
#define ARG_LONG_SET_FLAG(name, flag) \
    do {                              \
        if (!strcmp(arg, name)) {     \
            flag = 1;                 \
            goto next;                \
        }                             \
    } while (0)

#endif
