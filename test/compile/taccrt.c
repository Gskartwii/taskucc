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

long double __taccrt_addtf3(long double x, long double y) { return x + y; }
long double __taccrt_subtf3(long double x, long double y) { return x - y; }
long double __taccrt_multf3(long double x, long double y) { return x * y; }
long double __taccrt_divtf3(long double x, long double y) { return x / y; }
int __taccrt_eqtf2(long double x, long double y) { return !(x == y); }
int __taccrt_netf2(long double x, long double y) { return x != y; }
int __taccrt_lttf2(long double x, long double y) { return (x < y) ? -1 : 0; }
int __taccrt_letf2(long double x, long double y) { return (x <= y) ? -1 : 1; }
