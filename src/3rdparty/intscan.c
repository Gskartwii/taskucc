/*
 * Imported from musl 1.2.6.
 * Modified for M2-Planet, and to get rid of FILE* API.
 * musl 1.2.6 is licensed under the following license:
 *
 * Copyright © 2005-2020 Rich Felker, et al.

 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * ----------------------------------------------------------------------
 *
 * Authors/contributors include:
 *
 * A. Wilcox
 * Ada Worcester
 * Alex Dowad
 * Alex Suykov
 * Alexander Monakov
 * Andre McCurdy
 * Andrew Kelley
 * Anthony G. Basile
 * Aric Belsito
 * Arvid Picciani
 * Bartosz Brachaczek
 * Benjamin Peterson
 * Bobby Bingham
 * Boris Brezillon
 * Brent Cook
 * Chris Spiegel
 * Clément Vasseur
 * Daniel Micay
 * Daniel Sabogal
 * Daurnimator
 * David Carlier
 * David Edelsohn
 * Denys Vlasenko
 * Dmitry Ivanov
 * Dmitry V. Levin
 * Drew DeVault
 * Emil Renner Berthing
 * Fangrui Song
 * Felix Fietkau
 * Felix Janda
 * Gianluca Anzolin
 * Hauke Mehrtens
 * He X
 * Hiltjo Posthuma
 * Isaac Dunham
 * Jaydeep Patil
 * Jens Gustedt
 * Jeremy Huntwork
 * Jo-Philipp Wich
 * Joakim Sindholt
 * John Spencer
 * Julien Ramseier
 * Justin Cormack
 * Kaarle Ritvanen
 * Khem Raj
 * Kylie McClain
 * Leah Neukirchen
 * Luca Barbato
 * Luka Perkov
 * Lynn Ochs
 * M Farkas-Dyck (Strake)
 * Mahesh Bodapati
 * Markus Wichmann
 * Masanori Ogino
 * Michael Clark
 * Michael Forney
 * Mikhail Kremnyov
 * Natanael Copa
 * Nicholas J. Kain
 * orc
 * Pascal Cuoq
 * Patrick Oppenlander
 * Petr Hosek
 * Petr Skocik
 * Pierre Carrier
 * Reini Urban
 * Rich Felker
 * Richard Pennington
 * Ryan Fairfax
 * Samuel Holland
 * Segev Finer
 * Shiz
 * sin
 * Solar Designer
 * Stefan Kristiansson
 * Stefan O'Rear
 * Szabolcs Nagy
 * Timo Teräs
 * Trutz Behn
 * Will Dietz
 * William Haddon
 * William Pitcock
 *
 * Portions of this software are derived from third-party works licensed
 * under terms compatible with the above MIT license:
 *
 * The TRE regular expression implementation (src/regex/reg* and
 * src/regex/tre*) is Copyright © 2001-2008 Ville Laurikari and licensed
 * under a 2-clause BSD license (license text in the source files). The
 * included version has been heavily modified by Rich Felker in 2012, in
 * the interests of size, simplicity, and namespace cleanliness.
 *
 * Much of the math library code (src/math/ * and src/complex/ *) is
 * Copyright © 1993,2004 Sun Microsystems or
 * Copyright © 2003-2011 David Schultz or
 * Copyright © 2003-2009 Steven G. Kargl or
 * Copyright © 2003-2009 Bruce D. Evans or
 * Copyright © 2008 Stephen L. Moshier or
 * Copyright © 2017-2018 Arm Limited
 * and labelled as such in comments in the individual source files. All
 * have been licensed under extremely permissive terms.
 *
 * The ARM memcpy code (src/string/arm/memcpy.S) is Copyright © 2008
 * The Android Open Source Project and is licensed under a two-clause BSD
 * license. It was taken from Bionic libc, used on Android.
 *
 * The AArch64 memcpy and memset code (src/string/aarch64/ *) are
 * Copyright © 1999-2019, Arm Limited.
 *
 * The implementation of DES for crypt (src/crypt/crypt_des.c) is
 * Copyright © 1994 David Burren. It is licensed under a BSD license.
 *
 * The implementation of blowfish crypt (src/crypt/crypt_blowfish.c) was
 * originally written by Solar Designer and placed into the public
 * domain. The code also comes with a fallback permissive license for use
 * in jurisdictions that may not recognize the public domain.
 *
 * The smoothsort implementation (src/stdlib/qsort.c) is Copyright © 2011
 * Lynn Ochs and is licensed under an MIT-style license.
 *
 * The x86_64 port was written by Nicholas J. Kain and is licensed under
 * the standard MIT terms.
 *
 * The mips and microblaze ports were originally written by Richard
 * Pennington for use in the ellcc project. The original code was adapted
 * by Rich Felker for build system and code conventions during upstream
 * integration. It is licensed under the standard MIT terms.
 *
 * The mips64 port was contributed by Imagination Technologies and is
 * licensed under the standard MIT terms.
 *
 * The powerpc port was also originally written by Richard Pennington,
 * and later supplemented and integrated by John Spencer. It is licensed
 * under the standard MIT terms.
 *
 * All other files which have no copyright comments are original works
 * produced specifically for use as part of this library, written either
 * by Rich Felker, the main author of the library, or by one or more
 * contibutors listed above. Details on authorship of individual files
 * can be found in the git version control history of the project. The
 * omission of copyright and license comments in each file is in the
 * interest of source tree size.
 *
 * In addition, permission is hereby granted for all public header files
 * (include/ * and arch/ * /bits/ *) and crt files intended to be linked into
 * applications (crt/ *, ldso/dlstart.c, and arch/ * /crt_arch.h) to omit
 * the copyright notice and permission notice otherwise required by the
 * license, and to use these files without any requirement of
 * attribution. These files include substantial contributions from:
 *
 * Bobby Bingham
 * John Spencer
 * Nicholas J. Kain
 * Rich Felker
 * Richard Pennington
 * Stefan Kristiansson
 * Szabolcs Nagy
 *
 * all of whom have explicitly granted such permission.
 *
 * This file previously contained text expressing a belief that most of
 * the files covered by the above exception were sufficiently trivial not
 * to be subject to copyright, resulting in confusion over whether it
 * negated the permissions granted in the license. In the spirit of
 * permissive licensing, and of not having licensing issues being an
 * obstacle to adoption, that text has been removed.
 */

#include "intscan.h"
#include "../util.h"

#ifdef __M2__
#define M1 127
#else
#define M1 0x7f
#endif

/* Lookup table for digit values. 127>=36 -> invalid */
static const unsigned char table[] = {
    M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1,
    M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1,
    M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, 0,  1,  2,  3,  4,  5,  6,  7,
    8,  9,  M1, M1, M1, M1, M1, M1, M1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, M1, M1, M1,
    M1, M1, M1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
    26, 27, 28, 29, 30, 31, 32, 33, 34, 35, M1, M1, M1, M1, M1, M1, M1, M1, M1,
    M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1,
    M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1,
    M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1,
    M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1,
    M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1,
    M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1, M1,
    M1, M1, M1, M1, M1, M1, M1, M1, M1, M1,
};

#define UINT_MAX_DIV10 0x19999999
#define UINT_MAX_DIV32 0x07FFFFFF
#define UINT_MAX_DIV36 0x071C71C7

static int shgetc(struct tacc_file_iter *file) {
    if (tacc_file_is_eof(file)) {
        return -1;
    }
    return tacc_file_iter_consume_ch(file);
}

#define shunget(f)                                          \
    tacc_assert((f->src != f->orig), "unget at beginning"); \
    f->src = f->src - 1

void intscan(struct tacc_file_iter *f,
             unsigned base,
             struct tacc_u64 *lim,
             struct tacc_u64 *out) {
    const unsigned char *val;
    int c;
    int bs;
    int neg;
    unsigned x;
    struct tacc_u64 y;
    struct tacc_u64 ullong_max_div_base;
    struct tacc_u64 aux;
    struct tacc_u64 aux1;

    val = table + 1;
    neg = 0;
    tacc_u64_zero(&y);

    tacc_assert(!(base > 36 || base < 2), "bad base %d", base);

    if (base == 10) {
        ullong_max_div_base.high = 0x19999999;
        ullong_max_div_base.low = 0x99999999;

        x = 0;
        c = shgetc(f);
        for (x = 0; (unsigned) (c - '0') < 10U && x <= UINT_MAX_DIV10 - 1;
             c = shgetc(f))
            x = x * 10 + (unsigned) (c - '0');
        for (y.low = x; (unsigned) (c - '0') < 10U &&
                        tacc_u64_ule(&y, &ullong_max_div_base);
             c = shgetc(f)) {
            aux.high = 0xFFFFFFFF;
            aux.low = 0xFFFFFFFF;
            tacc_u64_sub_u32(&aux, &aux, (uint32_t) (c - '0'));
            tacc_u64_mul_u32(&aux1, &y, 10);
            if (!(tacc_u64_ule(&aux1, &aux))) {
                break;
            }
            tacc_u64_add_u32(&y, &aux1, (uint32_t) (c - '0'));
        }
        if ((unsigned) (c - '0') >= 10U) {
            goto done;
        }
    } else if (!(base & (base - 1))) {
        switch ((0x17 * base) >> 5 & 7) {
        case 0:
        case 1:
        case 2:
            bs = (0x17 * base) >> 5 & 7;
            break;
        case 3:
            bs = 4;
            break;
        case 4:
            bs = 7;
            break;
        case 5:
            bs = 3;
            break;
        case 6:
            bs = 6;
            break;
        case 7:
            bs = 5;
            break;
        default:
            tacc_assert(0, "invalid base: %d", base);
            bs = 0;
            break;
        }
        ullong_max_div_base.low = 0xFFFFFFFF;
        ullong_max_div_base.high = 0xFFFFFFFF;
        tacc_u64_rsh_n(&ullong_max_div_base, &ullong_max_div_base, bs);

        c = shgetc(f);

        for (x = 0; val[c] < base && x <= UINT_MAX_DIV32; c = shgetc(f))
            x = x << bs | val[c];
        for (y.low = x; val[c] < base && tacc_u64_ule(&y, &ullong_max_div_base);
             c = shgetc(f)) {
            tacc_u64_lsh_n(&y, &y, bs);
            y.low |= val[c];
        }
    } else {
        ullong_max_div_base.low = 0xFFFFFFFF;
        ullong_max_div_base.high = 0xFFFFFFFF;
        tacc_u64_zero(&aux1);
        tacc_u64_add_u32(&aux1, &aux1, base);
        tacc_u64_udiv(&ullong_max_div_base, &aux, &ullong_max_div_base, &aux1);

        c = shgetc(f);

        for (x = 0; val[c] < base && x <= UINT_MAX_DIV36 - 1; c = shgetc(f))
            x = x * base + val[c];
        for (y.low = x; val[c] < base && tacc_u64_ule(&y, &ullong_max_div_base);
             c = shgetc(f)) {
            aux.high = 0xFFFFFFFF;
            aux.low = 0xFFFFFFFF;
            tacc_u64_sub_u32(&aux, &aux, (uint32_t) val[c]);
            tacc_u64_mul_u32(&aux1, &y, base);
            if (!(tacc_u64_ule(&aux1, &aux))) {
                break;
            }
            tacc_u64_add_u32(&y, &aux1, val[c]);
        }
    }
    tacc_assert(val[c] >= base, "constant number overflow, too many digits");
    tacc_assert(tacc_u64_ule(&y, lim),
                "constant number overflow, out of requested range");
done:
    shunget(f);
    if (neg) {
        tacc_u64_neg(out, &y);
    } else {
        tacc_u64_copy(out, &y);
    }
}
