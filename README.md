# Tasku C Compiler

Tasku C Compiler is a bootstrappable compiler. It is intended to be built by
[M2-Planet](https://github.com/oriansj/M2-Planet) and to eventually build the
mob branch of [tinycc](https://repo.or.cz/tinycc.git) without any patches.

The Tasku C Compiler aims to implement most of C99. Features planned to be
included:
- Floating-point support, including long double
- long long, including software arithmetic on 32-bit platforms
- Variable-length arrays and alloca
- setjmp
- Large structures as arguments and return values
- Inline assembly, including integrated assembler
- Static linker with ELF support
- Cross-compiling
- Adherence to C99 standard for all implemented features

Features not planned:
- _Complex, _Atomic
- Quality diagnostics
- Optimality of generated code
- Diagnosing constness issues

Restrictions placed by M2-Planet:
- Can not take address of structure field or array member
- Can not use long long on 32-bit platforms
- No floating-point support
- Pointer arithmetic on non-char types requires `tacc_sizeadj`
