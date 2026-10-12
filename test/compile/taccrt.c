long double __taccrt_floatditf(long long x) { return x; }
long double __taccrt_floatunsditf(unsigned long long x) { return x; }

float __taccrt_trunctfsf2(long double x) { return (float) x; }
double __taccrt_trunctfdf2(long double x) { return (double) x; }

long long __taccrt_muldi3(long long x, long long y) { return x * y; }

long long __taccrt_divdi3(long long x, long long y) { return x / y; }
unsigned long long __taccrt_udivdi3(unsigned long long x,
                                    unsigned long long y) {
    return x / y;
}
long long __taccrt_moddi3(long long x, long long y) { return x % y; }
unsigned long long __taccrt_umoddi3(unsigned long long x,
                                    unsigned long long y) {
    return x % y;
}
