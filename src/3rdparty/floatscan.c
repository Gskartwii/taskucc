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

#include "floatscan.h"
#include "soft_float.h"
#include "soft_u64.h"
#include "tasku_pp.h"

#define DECIMAL_BUFFER_SIZE 2048
#define DECBUF_LIMIT (DECIMAL_BUFFER_SIZE - 1)
#define INTMAX_DIV_10 0x0CCCCCCC

static int shgetc(struct tacc_file_iter *file) {
    char ch;
    if (tacc_file_is_eof(file)) {
        return -1;
    }
    ch = tacc_file_iter_consume_ch(file);
    return (int) ch;
}

#define shunget(f)                                                      \
    tacc_assert(ASSERT_ICE, (f->src != f->orig), "unget at beginning"); \
    f->src = f->src - 1

static void scanexp(struct tacc_file_iter *f, struct tacc_u64 *out) {
    int c;
    int x;
    struct tacc_u64 y;
    struct tacc_u64 llong_max_div100;
    int neg = 0;

    tacc_u64_zero(&y);

    llong_max_div100.low = 0x7ae147ae;
    llong_max_div100.high = 0x0147ae14;

    if (tacc_file_iter_accept_ch(f, '-')) {
        neg = 1;
    } else {
        tacc_file_iter_accept_ch(f, '+');
    }
    c = shgetc(f);
    tacc_assert(
        ASSERT_DIAG, (unsigned) (c - '0') < 10, "invalid exponent %d", c);
    for (x = 0; (unsigned) (c - '0') < 10 && x < INTMAX_DIV_10; c = shgetc(f)) {
        x = 10 * x + (int) (c - '0');
    }
    for (y.low = (unsigned) x;
         (unsigned) (c - '0') < 10 && tacc_u64_slt(&y, &llong_max_div100);
         c = shgetc(f)) {
        tacc_u64_mul_u32(&y, &y, 10);
        tacc_u64_add_u32(&y, &y, (unsigned) (c - '0'));
    }
    /*
     * x = 0 is unused. We can't have an empty statement there on M2, so, just
     * do something useless
     */
    for (x = 0; (unsigned) (c - '0') < 10; c = shgetc(f)) {
    }
    shunget(f);
    if (neg) {
        tacc_u64_neg(out, &y);
    } else {
        tacc_u64_copy(out, &y);
    }
}

/*
 * Holds 2048 numbers in the range of [0, 10^9).
 * Most significant part of the number comes first.
 */
uint32_t decimal_buffer[2048];
uint32_t th_f128[4] = {10384593, 717069655, 257060992, 658440191};
int powers_of_10[8] = {
    10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000};

uint32_t decbuf_val(int index) {
    char *decbuf;
    uint32_t *decbuf_pos;

    decbuf = (char *) decimal_buffer;
    decbuf = decbuf + ((size_t) index) * (sizeof(uint32_t));
    decbuf_pos = (uint32_t *) decbuf;

    return *decbuf_pos;
}
void decbuf_set(int index, uint32_t val) {
    char *decbuf;
    uint32_t *decbuf_pos;

    decbuf = (char *) decimal_buffer;
    decbuf = decbuf + ((size_t) index) * (sizeof(uint32_t));
    decbuf_pos = (uint32_t *) decbuf;

    *decbuf_pos = val;
}
uint32_t th_val(int index) {
    char *th;
    uint32_t *th_pos;

    th = (char *) th_f128;
    th = th + ((size_t) index) * (sizeof(uint32_t));
    th_pos = (uint32_t *) th;

    return *th_pos;
}
int calc_p10(int index) {
    char *p10;
    int *p10_pos;

    p10 = (char *) powers_of_10;
    p10 = p10 + ((size_t) index) * (sizeof(int));
    p10_pos = (int *) p10;

    return *p10_pos;
}

static void decfloat(struct tacc_file_iter *f,
                     int bits,
                     int emin,
                     int sign,
                     struct tacc_f128 *out) {
    int c;
    int i;
    int decbuf_curr_pow10;
    int decbuf_i;
    int a;
    int decbuf_end;
    int exponent_of_10;
    uint32_t num_significant_digits;
    struct tacc_u64 exp_as_written;
    struct tacc_u64 aux;
    struct tacc_u64 aux_2;
    int seen_digits;
    int seen_decimal_point;
    int offset_first_sd_from_decpoint;
    int exp_adjustment;
    int emax;
    int denormal;
    struct tacc_f128 y;
    struct tacc_f128 frac;
    struct tacc_f128 bias;
    int rpm9;
    int power_of_10;
    uint32_t carry;
    uint32_t tmp;
    int ld_b1b_dig;
    int ldbl_mant_dig;
    int shift;
    uint32_t tail_begin;
    struct tacc_f128 aux_f;
    struct tacc_f128 aux_f_2;

    exponent_of_10 = 0;
    num_significant_digits = 0;
    tacc_u64_zero(&exp_as_written);
    seen_digits = 0;
    seen_decimal_point = 0;
    emax = -emin - bits + 3;
    denormal = 0;
    tacc_f128_zero(&frac);
    tacc_f128_zero(&bias);

    decbuf_curr_pow10 = 0;
    decbuf_i = 0;

    /* Don't let leading zeros consume buffer space */

    /* Skip zeroes in whole part */
    while (tacc_file_iter_accept_ch(f, '0')) {
        seen_digits = 1;
    }
    if (tacc_file_iter_accept_ch(f, '.')) {
        seen_decimal_point = 1;
        /*
         * If whole part is 0, skip zeroes in fractional part. Reduce implicit
         * exponent
         */
        while (tacc_file_iter_accept_ch(f, '0')) {
            seen_digits = 1;
            exponent_of_10 = exponent_of_10 - 1;
        }
    }

    decbuf_set(0, 0);
    for (c = shgetc(f); (unsigned) (c - '0') < 10 || c == '.'; c = shgetc(f)) {
        if (c == '.') {
            if (seen_decimal_point) {
                break;
            }
            seen_decimal_point = 1;
            exponent_of_10 = (int) num_significant_digits;
        } else if (decbuf_i < DECIMAL_BUFFER_SIZE - 3) {
            num_significant_digits = num_significant_digits + 1;
            if (decbuf_curr_pow10) {
                decbuf_set(decbuf_i,
                           decbuf_val(decbuf_i) * 10 + (uint32_t) (c - '0'));
            } else {
                decbuf_set(decbuf_i, (uint32_t) (c - '0'));
            }
            decbuf_curr_pow10 = decbuf_curr_pow10 + 1;
            if (decbuf_curr_pow10 == 9) {
                decbuf_i = decbuf_i + 1;
                decbuf_curr_pow10 = 0;
            }
            seen_digits = 1;
        } else {
            num_significant_digits = num_significant_digits + 1;
            if (c != '0') {
                decbuf_set(DECIMAL_BUFFER_SIZE - 4,
                           decbuf_val(DECIMAL_BUFFER_SIZE - 4) | 1);
            }
        }
    }
    if (!seen_decimal_point) {
        exponent_of_10 = (int) num_significant_digits;
    }

    if (seen_digits && (c | 32) == 'e') {
        scanexp(f, &exp_as_written);
        tacc_u64_from_i32(&aux, emin);
        tacc_u64_from_u32(&aux_2, 0xFFFFFFFF);
        tacc_assert(ASSERT_DIAG,
                    tacc_u64_sle(&exp_as_written, &aux_2),
                    "exponent too high: %x:%x > %x:%x",
                    exp_as_written.high,
                    exp_as_written.low,
                    aux_2.high,
                    aux_2.low);
        tacc_assert(ASSERT_DIAG,
                    tacc_u64_sge(&exp_as_written, &aux),
                    "exponent too low");
        exponent_of_10 = exponent_of_10 + (int) (exp_as_written.low);
    } else if (c >= 0) {
        shunget(f);
    }
    tacc_assert(ASSERT_DIAG, seen_digits, "invalid floating literal");

    /* Handle zero specially to avoid nasty special cases later */
    if (decbuf_val(0) == 0) {
        tacc_f128_zero(out);
        if (sign < 0) {
            out->sign = 1;
        }
        return;
    }

    /* Optimize small integers (w/no exponent) and over/under-flow */
    if (exponent_of_10 == (int) num_significant_digits &&
        num_significant_digits < 10 &&
        (bits > 30 || decbuf_val(0) >> ((unsigned) bits) == 0)) {
        tacc_f128_from_u32(out, decbuf_val(0));
        out->sign = 1;
        return;
    }

    /* Align incomplete final B1B digit */
    if (decbuf_curr_pow10) {
        /* out->sign nop operation to avoid M2 issues */
        for (out->sign = 0; decbuf_curr_pow10 < 9;
             decbuf_curr_pow10 = decbuf_curr_pow10 + 1) {
            decbuf_set(decbuf_i, decbuf_val(decbuf_i) * 10);
        }
        decbuf_i = decbuf_i + 1;
        decbuf_curr_pow10 = 0;
    }

    a = 0;
    decbuf_end = decbuf_i;
    exp_adjustment = 0;
    offset_first_sd_from_decpoint = exponent_of_10;

    /* Drop trailing zeros */
    /* exp_adjustment = 0 here is a no-op for M2 compat */
    for (exp_adjustment = 0; !decbuf_val(decbuf_end - 1);
         decbuf_end = decbuf_end - 1) {
    }

    /* Align radix point to B1B digit boundary */
    if (offset_first_sd_from_decpoint % 9) {
        if (offset_first_sd_from_decpoint >= 0) {
            rpm9 = offset_first_sd_from_decpoint % 9;
        } else {
            rpm9 = (offset_first_sd_from_decpoint % 9) + 9;
        }
        power_of_10 = calc_p10(8 - rpm9);
        carry = 0;
        /* Why decbuf_i=a? Why not decbuf_i=0 ??? */
        for (decbuf_i = a; decbuf_i != decbuf_end; decbuf_i = decbuf_i + 1) {
            tmp = decbuf_val(decbuf_i) % (uint32_t) power_of_10;
            decbuf_set(decbuf_i,
                       decbuf_val(decbuf_i) / (uint32_t) power_of_10 + carry);
            carry = (uint32_t) (1000000000 / power_of_10) * tmp;
            if (decbuf_i == a && !decbuf_val(decbuf_i)) {
                a = (a + 1) & DECBUF_LIMIT;
                offset_first_sd_from_decpoint -= 9;
            }
        }
        if (carry) {
            decbuf_set(decbuf_end, carry);
            decbuf_end = decbuf_end + 1;
        }
        offset_first_sd_from_decpoint =
            offset_first_sd_from_decpoint + 9 - rpm9;
    }

    ld_b1b_dig = 4;
    ldbl_mant_dig = 113;

    /* Upscale until desired number of bits are left of radix point */
    while (offset_first_sd_from_decpoint < 9 * ld_b1b_dig ||
           (offset_first_sd_from_decpoint == 9 * ld_b1b_dig &&
            decbuf_val(a) < th_val(0))) {
        carry = 0;
        exp_adjustment -= 29;
        for (decbuf_i = (decbuf_end - 1) & DECBUF_LIMIT; 1;
             decbuf_i = (decbuf_i - 1) & DECBUF_LIMIT) {
            tacc_u64_from_u32(&aux, decbuf_val(decbuf_i));
            tacc_u64_lsh_n(&aux, &aux, 29);
            tacc_u64_add_u32(&aux, &aux, carry);
            tacc_u64_from_u32(&aux_2, 1000000000);
            if (tacc_u64_ugt(&aux, &aux_2)) {
                tacc_u64_from_u32(&aux_2, 1000000000);
                tacc_u64_udiv(&aux, &aux_2, &aux, &aux_2);
                carry = aux.low;
                decbuf_set(decbuf_i, aux_2.low);
            } else {
                carry = 0;
                decbuf_set(decbuf_i, aux.low);
            }
            if (decbuf_i == ((decbuf_end - 1) & DECBUF_LIMIT) &&
                decbuf_i != a && !decbuf_val(decbuf_i)) {
                decbuf_end = decbuf_i;
            }
            if (decbuf_i == a) {
                break;
            }
        }
        if (carry) {
            offset_first_sd_from_decpoint += 9;
            a = (a - 1) & DECBUF_LIMIT;
            if (a == decbuf_end) {
                decbuf_end = (decbuf_end - 1) & DECBUF_LIMIT;
                decbuf_set((decbuf_end - 1) & DECBUF_LIMIT,
                           decbuf_val((decbuf_end - 1) & DECBUF_LIMIT) |
                               decbuf_val(decbuf_end));
            }
            decbuf_set(a, carry);
        }
    }

    /* Downscale until exactly number of bits are left of radix point */
    while (1) {
        carry = 0;
        shift = 1;
        for (i = 0; i < ld_b1b_dig; i++) {
            decbuf_i = (a + i) & DECBUF_LIMIT;
            if (decbuf_i == decbuf_end || decbuf_val(decbuf_i) < th_val(i)) {
                i = ld_b1b_dig;
                break;
            }
            if (decbuf_val((a + i) & DECBUF_LIMIT) > th_val(i)) {
                break;
            }
        }
        if (i == ld_b1b_dig &&
            offset_first_sd_from_decpoint == 9 * ld_b1b_dig) {
            break;
        }
        /* FIXME: find a way to compute optimal shift */
        if (offset_first_sd_from_decpoint > 9 + 9 * ld_b1b_dig) {
            shift = 9;
        }
        exp_adjustment += shift;
        for (decbuf_i = a; decbuf_i != decbuf_end;
             decbuf_i = (decbuf_i + 1) & DECBUF_LIMIT) {
            tmp = decbuf_val(decbuf_i) & ((uint32_t) ((1 << shift) - 1));
            decbuf_set(decbuf_i,
                       (decbuf_val(decbuf_i) >> ((unsigned) shift)) + carry);
            carry = ((uint32_t) (1000000000 >> shift)) * tmp;
            if (decbuf_i == a && !decbuf_val(decbuf_i)) {
                a = (a + 1) & DECBUF_LIMIT;
                i = i - 1;
                offset_first_sd_from_decpoint -= 9;
            }
        }
        if (carry) {
            if (((decbuf_end + 1) & DECBUF_LIMIT) != a) {
                decbuf_set(decbuf_end, carry);
                decbuf_end = (decbuf_end + 1) & DECBUF_LIMIT;
            } else
                decbuf_set((decbuf_end - 1) & DECBUF_LIMIT,
                           decbuf_val((decbuf_end - 1) & DECBUF_LIMIT) | 1);
        }
    }

    /* Assemble desired bits into floating point variable */
    tacc_f128_zero(&y);
    for (i = 0; i < ld_b1b_dig; i++) {
        if (((a + i) & DECBUF_LIMIT) == decbuf_end) {
            decbuf_end = (decbuf_end + 1) & DECBUF_LIMIT;
            decbuf_set(decbuf_end - 1, 0);
        }
        /* y *= 10^9 */
        tacc_f128_mull_u32(&y, &y, 1000000000);
        tacc_f128_addl_u32(&y, &y, decbuf_val((a + i) & DECBUF_LIMIT));
    }

    if (sign < 0) {
        y.sign = 1;
    }

    /* Limit precision for denormal results */
    if (bits > ldbl_mant_dig + exp_adjustment - emin) {
        bits = ldbl_mant_dig + exp_adjustment - emin;
        if (bits < 0) {
            bits = 0;
        }
        denormal = 1;
    }

    /* Calculate bias term to force rounding, move out lower bits */
    if (bits < ldbl_mant_dig) {
        tacc_f128_from_u32(&bias, 1);
        tacc_f128_scalbn(&bias, &bias, 2 * ldbl_mant_dig - bits - 1);
        tacc_f128_copysignl(&bias, &bias, &y);

        tacc_f128_fmodl_p2(&frac, &y, ldbl_mant_dig - bits);
        tacc_f128_subl(&y, &y, &frac);
        tacc_f128_addl(&y, &y, &bias);
    }

    /* Process tail of decimal input so it can affect rounding */
    if (((a + i) & DECBUF_LIMIT) != decbuf_end) {
        tail_begin = decbuf_val((a + i) & DECBUF_LIMIT);
        /* tail_begin compared to 10^9 / 2 */
        if (tail_begin < 500000000 &&
            (tail_begin || ((a + i + 1) & DECBUF_LIMIT) != decbuf_end)) {
            /* Less than half, but not zero */
            tacc_f128_from_frac(&aux_f, sign, 4);
            tacc_f128_addl(&frac, &frac, &aux_f);
        } else if (tail_begin > 500000000) {
            /* More than half */
            tacc_f128_from_frac(&aux_f, sign * 3, 4);
            tacc_f128_addl(&frac, &frac, &aux_f);
        } else if (tail_begin == 500000000) {
            if (((a + i + 1) & DECBUF_LIMIT) == decbuf_end) {
                /* Exactly half */
                tacc_f128_from_frac(&aux_f, sign, 2);
                tacc_f128_addl(&frac, &frac, &aux_f);
            } else {
                /* A little bit more than half */
                tacc_f128_from_frac(&aux_f, sign * 3, 4);
                tacc_f128_addl(&frac, &frac, &aux_f);
            }
        }
        if (ldbl_mant_dig - bits >= 2) {
            tacc_f128_fmodl_p2(&aux_f, &frac, 0);
            if (!tacc_f128_is_zero(&aux_f)) {
                tacc_f128_addl_u32(&frac, &frac, 1);
            }
        }
    }

    tacc_f128_addl(&y, &y, &frac);
    tacc_f128_subl(&y, &y, &bias);

    if (((exp_adjustment + ldbl_mant_dig) & 0x7fffffff) > emax - 5) {
        tacc_f128_epsilonl128(&aux_f_2);
        tacc_f128_from_u32(&aux_f, 2);
        tacc_f128_divl(&aux_f_2, &aux_f, &aux_f_2);
        tacc_f128_absl(&aux_f, &y);
        if (tacc_f128_ge(&aux_f, &aux_f_2)) {
            if (denormal && bits == ldbl_mant_dig + exp_adjustment - emin) {
                denormal = 0;
            }
            tacc_f128_from_frac(&aux_f, 1, 2);
            tacc_f128_mull(&y, &y, &aux_f);
            exp_adjustment++;
        }
    }

    tacc_f128_scalbnl(out, &y, exp_adjustment);
}

static void hexfloat(struct tacc_file_iter *f,
                     int bits,
                     int emin,
                     int sign,
                     struct tacc_f128 *out) {
    uint32_t x = 0;
    struct tacc_f128 y;
    struct tacc_f128 scale;
    struct tacc_f128 bias;
    struct tacc_f128 aux_f;
    int gottail;
    int gotrad;
    int gotdig;
    int rp;
    int dc;
    struct tacc_u64 e2;
    int d;
    int c;

    tacc_u64_zero(&e2);
    tacc_f128_zero(&y);
    tacc_f128_from_u32(&scale, 1);
    tacc_f128_zero(&bias);
    rp = 0;
    dc = 0;
    gottail = 0;
    gotrad = 0;
    gotdig = 0;

    c = shgetc(f);

    /* Skip leading zeros */
    for (; c == '0'; c = shgetc(f)) {
        gotdig = 1;
    }

    if (c == '.') {
        gotrad = 1;
        c = shgetc(f);
        /* Count zeros after the radix point before significand */
        for (rp = 0; c == '0'; c = shgetc(f)) {
            gotdig = 1;
            rp = rp - 1;
        }
    }

    for (gottail = 0; ((unsigned) c - '0') < 10 ||
                      ((unsigned) (c | 32) - 'a') < 6 || c == '.';
         c = shgetc(f)) {
        if (c == '.') {
            if (gotrad) {
                break;
            }
            rp = dc;
            gotrad = 1;
        } else {
            gotdig = 1;
            if (c > '9') {
                d = (c | 32) + 10 - 'a';
            } else {
                d = c - '0';
            }
            if (dc < 8) {
                x = x * 16 + (uint32_t) d;
            } else if (dc < 113 /* ldbl_mant_dig */ / 4 + 1) {
                tacc_f128_divl_u32(&scale, &scale, 16);
                tacc_f128_mull_u32(&aux_f, &scale, (uint32_t) d);
                tacc_f128_addl(&y, &y, &aux_f);
            } else if (d && !gottail) {
                tacc_f128_divl_u32(&aux_f, &scale, 2);
                tacc_f128_addl(&y, &y, &aux_f);
                gottail = 1;
            }
            dc = dc + 1;
        }
    }
    tacc_assert(ASSERT_DIAG, gotdig, "invalid hex floating point literal");
    if (!gotrad) {
        rp = dc;
    }
    while (dc < 8) {
        x = x * 16;
        dc = dc + 1;
    }
    if ((c | 32) == 'p') {
        scanexp(f, &e2);
    } else {
        shunget(f);
    }
    tacc_u64_add_s32(&e2, &e2, 4 * rp - 32);

    if (!x) {
        tacc_f128_zero(out);
        if (sign < 0) {
            out->sign = 1;
        }
        return;
    }
    tacc_assert(
        ASSERT_DIAG, !(tacc_u64_sgt_s32(&e2, -emin)), "exponent out of range");
    tacc_assert(ASSERT_DIAG,
                !(tacc_u64_slt_s32(&e2, emin - 2 * 113 /* LDBL_MANT_DIG */)),
                "exponent out of range");

    while ((x >> ((unsigned) 31)) == 0) {
        tacc_f128_mull_u32(&aux_f, &y, 2);
        if (tacc_f128_ge_s32(&aux_f, 1)) {
            x = x + x + 1;
            tacc_f128_subl_u32(&y, &y, 1);
        } else {
            x = x + x;
            tacc_f128_addl(&y, &y, &y);
        }
        tacc_u64_add_s32(&e2, &e2, -1);
    }

    if (bits > 32 + ((int) e2.low) - emin) {
        bits = 32 + ((int) e2.low) - emin;
        if (bits < 0) {
            bits = 0;
        }
    }

    if (bits < 113) {
        tacc_f128_from_u32(&aux_f, 1);
        tacc_f128_scalbn(&bias, &aux_f, 32 + 113 - bits - 1);
        if (sign < 0) {
            bias.sign = 1;
        }
    }

    if (bits < 32 && !tacc_f128_is_zero(&y) && !(x & 1)) {
        x = x + 1;
        tacc_f128_zero(&y);
    }

    tacc_f128_from_u32(&aux_f, x);
    if (sign < 0) {
        y.sign = 1;
        aux_f.sign = 1;
    }
    tacc_f128_addl(&aux_f, &bias, &aux_f);
    tacc_f128_addl(&y, &aux_f, &y);
    tacc_f128_subl(&y, &y, &bias);

    tacc_assert(ASSERT_DIAG,
                !tacc_f128_is_zero(&y),
                "subnormal hexadecimal floating-point number");

    tacc_f128_scalbnl(out, &y, (int) e2.low);
}

void floatscan(struct tacc_file_iter *f, int prec, struct tacc_f128 *out) {
    int sign;
    int bits;
    int emin;

    sign = 1;

    switch (prec) {
    case 0:
        bits = 24 /* FLT_MANT_DIG */;
        emin = (-125 /* FLT_MIN_EXP */) - bits;
        break;
    case 1:
        bits = 53 /* DBL_MANT_DIG */;
        emin = (-16381 /* DBL_MIN_EXP */) - bits;
        break;
    case 2:
        bits = 64 /* LDBL_MANT_DIG_80 */;
        emin = (-16381 /* LDBL_MIN_EXP_80 */) - bits;
        break;
    case 3:
        bits = 113 /* LDBL_MANT_DIG_128 */;
        emin = (-16381 /* LDBL_MIN_EXP_128 */) - bits;
        break;
    default:
        tacc_assert(ASSERT_ICE, 0, "invalid precision");
        return;
    }

    if (tacc_file_iter_accept_ch(f, '-')) {
        sign = -1;
    } else {
        tacc_file_iter_accept_ch(f, '+');
    }

    if (tacc_file_iter_accept_ch(f, '0')) {
        if (tacc_file_iter_accept_ch(f, 'x') ||
            tacc_file_iter_accept_ch(f, 'X')) {
            hexfloat(f, bits, emin, sign, out);
            return;
        }
    }

    decfloat(f, bits, emin, sign, out);
    return;
}
