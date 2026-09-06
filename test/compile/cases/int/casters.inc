#define CASTER_LIST                    \
    CASTER_ACTION(char, char, qi_qi)   \
    CASTER_ACTION(short, short, hi_hi) \
    CASTER_ACTION(int, int, si_si)     \
    CASTER_ACTION(long, long, di_di)   \
    CASTER_ACTION(long, int, di_si)    \
    CASTER_ACTION(int, long, si_di)    \
    CASTER_ACTION(char, int, qi_si)    \
    CASTER_ACTION(char, long, qi_di)   \
    CASTER_ACTION(int, char, si_qi)    \
    CASTER_ACTION(long, char, di_qi)
