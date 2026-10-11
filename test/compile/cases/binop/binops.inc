#define FLOATOPS(output, left, right, suffix)          \
    BINOP_ACTION(output, left, right, +, add_##suffix) \
    BINOP_ACTION(output, left, right, -, sub_##suffix) \
    BINOP_ACTION(output, left, right, *, mul_##suffix) \
    BINOP_ACTION(output, left, right, /, div_##suffix) \
    BINOP_ACTION(_Bool, left, right, <, lt_##suffix)   \
    BINOP_ACTION(_Bool, left, right, >, gt_##suffix)   \
    BINOP_ACTION(_Bool, left, right, >=, ge_##suffix)  \
    BINOP_ACTION(_Bool, left, right, <=, le_##suffix)  \
    BINOP_ACTION(_Bool, left, right, ==, eq_##suffix)  \
    BINOP_ACTION(_Bool, left, right, !=, ne_##suffix)

#define INTOPS(output, left, right, suffix)             \
    FLOATOPS(output, left, right, suffix)               \
    BINOP_ACTION(output, left, right, %, mod_##suffix)  \
    BINOP_ACTION(output, left, right, |, bor_##suffix)  \
    BINOP_ACTION(output, left, right, &, band_##suffix) \
    BINOP_ACTION(output, left, right, ^, bxor_##suffix)

#define BINOP_LIST                                             \
    FLOATOPS(float, float, float, sf_sf)                       \
    FLOATOPS(double, double, double, df_df)                    \
    FLOATOPS(long double, long double, long double, tf_tf)     \
    FLOATOPS(float, float, long, sf_di)                        \
    FLOATOPS(long double, long, long double, di_tf)            \
    FLOATOPS(float, int, float, si_sf)                         \
    FLOATOPS(float, float, int, sf_si)                         \
    INTOPS(int, unsigned char, unsigned char, qi_qi)           \
    INTOPS(int, unsigned short, unsigned short, hi_hi)         \
    INTOPS(unsigned int, unsigned int, unsigned int, si_si)    \
    INTOPS(unsigned long, unsigned long, unsigned long, di_di) \
    INTOPS(unsigned long, unsigned long, unsigned int, di_si)  \
    INTOPS(unsigned long, unsigned int, unsigned long, si_di)  \
    INTOPS(unsigned int, unsigned char, unsigned int, qi_si)   \
    INTOPS(unsigned long, unsigned char, unsigned long, qi_di) \
    INTOPS(unsigned int, unsigned int, unsigned char, si_qi)   \
    INTOPS(unsigned long, unsigned long, unsigned char, di_qi)
