let
  pkgs = import (builtins.getFlake "nixpkgs") {};
  lib = pkgs.lib;
  m2libc = pkgs.fetchFromGitHub {
    owner = "oriansj";
    repo = "M2libc";
    rev = "ca023d8dc855171fd0618951add5817e0e568fca";
    hash = "sha256-7xjH/Dti62/s4cRVLE5NMDYEsiKIfuaDsF9a8+glW5o=";
  };
  includes-m2 = [
    "sys/types.h"
    "stddef.h"
    "amd64/linux/fcntl.c"
    "fcntl.c"
    "sys/utsname.h"
    "amd64/linux/unistd.c"
    "amd64/linux/sys/stat.c"
    "ctype.c"
    "stdlib.c"
    "stdarg.h"
    "stdio.h"
    "stdio.c"
    "string.c"
    "bootstrappable.c"
  ];
  includes = map (file: "${m2libc}/${file}") includes-m2;
  local_hdrs = [
    "m2_shim.h"
    "util.h"
    "dynarray.h"
    "dynhash.h"
    "dynstring.h"
    "tasku_file.h"
    "soft_u64.h"
    "type.h"
    "machine.h"
    "target_defs.h"
    "expr.h"
    "tasku_pp.h"
    "test.h"
  ];
  src = [
    "3rdparty/intscan.c"
    "util.c"
    "dynarray.c"
    "dynhash.c"
    "dynstring.c"
    "soft_u64.c"
    "machine.c"
    "type.c"
    "expr.c"
    "target_defs.c"
    "tasku_file.c"
    "tasku_pp.c"
    "test.c"
    "tasku.c"
  ];

  m2-all = includes ++ local_hdrs ++ src;

  tinycc-src = pkgs.fetchgit {
    url = "https://repo.or.cz/tinycc.git";
    rev = "a338258d309c888bde96b2d1f206299231a54ddf";
    hash = "sha256-R1Kyycihw5rKu+vv/GzEMPAdaApW0lrIESjrbnEa2Dg=";
  };

  cflags = [
    "-std=c90"
    "-Wall"
    "-Wextra"
    "-Wpedantic"
    "-Werror"
    "-Wconversion"
    "-Warith-conversion"
    "-Wno-variadic-macros"
  ];
  cflags-debug =
    cflags
    ++ [
      "-g3"
      "-Og"
      "-fsanitize=address,undefined"
    ];
  cflags-opt =
    cflags
    ++ [
      "-O3"
      "-pg"
    ];
  mk-tasku-gcc = cf:
    pkgs.stdenv.mkDerivation {
      pname = "tasku-cc-gcc";
      version = "0.1.0";
      src = ./src;
      buildPhase = ''
        ${lib.strings.concatMapStringsSep "\n" (file: "$CC ${builtins.concatStringsSep " " cf} -c ${file}") src}
        $CC ${builtins.concatStringsSep " " cf} -o tasku $(basename --multiple ${lib.strings.concatMapStringsSep " " (builtins.replaceStrings [".c"] [".o"]) src})
      '';
      installPhase = ''
        mkdir -p $out/bin
        cp tasku $out/bin/tasku-gcc
      '';
      dontStrip = true;
      meta.mainProgram = "tasku-gcc";
    };
in rec {
  tasku-gcc = mk-tasku-gcc cflags-debug;
  tasku-gcc-opt = mk-tasku-gcc cflags-opt;
  tasku-m2 = pkgs.stdenvNoCC.mkDerivation {
    pname = "tasku-cc";
    version = "0.1.0";
    src = ./src;
    nativeBuildInputs = [
      pkgs.minimal-bootstrap.stage0-posix.mescc-tools
    ];
    buildPhase = ''
      M2-Planet --architecture amd64 \
        -f ${lib.strings.concatStringsSep " -f " m2-all} \
        --debug \
        -o ./tasku.M1

      blood-elf --little-endian --64 -f tasku.M1 -o tasku-footer.M1

      M1 --architecture amd64 \
        --little-endian \
        -f ${m2libc}/amd64/amd64_defs.M1 \
        -f ${m2libc}/amd64/libc-full.M1 \
        -f tasku.M1 \
        -f tasku-footer.M1 \
        -o tasku.hex2

      hex2 --architecture amd64 \
        --little-endian \
        --base-address 0x00600000 \
        -f ${m2libc}/amd64/ELF-amd64-debug.hex2 \
        -f tasku.hex2 \
        -o tasku
    '';

    installPhase = ''
      mkdir -p $out/bin/
      cp tasku.M1 $out/bin/tasku.M1
      cp tasku $out/bin/tasku-m2
    '';
    meta.mainProgram = "tasku-m2";
  };
  tasku-both = pkgs.symlinkJoin {
    name = "tasku";
    paths = [tasku-m2 tasku-gcc];
  };

  compare-m2-gcc = pkgs.stdenvNoCC.mkDerivation {
    pname = "compare-taskucc-across-m2-gcc";
    version = "0.1.0";
    dontUnpack = true;

    buildPhase = ''
      ok=true
      touch assert.h \
        config.h \
        direct.h \
        dlfcn.h \
        errno.h \
        fcntl.h \
        inttypes.h \
        io.h \
        malloc.h \
        math.h \
        setjmp.h \
        signal.h \
        stdarg.h \
        stdint.h \
        stdio.h \
        stdlib.h \
        string.h \
        test.h \
        time.h \
        unistd.h \
        windows.h
      mkdir sys
      touch sys/{time.h,mman.h,ucontext.h}

      flags="\
        ${tinycc-src}/tcc.c \
        -DONE_SOURCE \
        -DTCC_TARGET_X86_64=1 \
        -DBOOTSTRAP=1 \
        -DCONFIG_TCCDIR=\"\" \
        -DCONFIG_SYSROOT=\"\" \
        -DCONFIG_TCC_CRTPREFIX=\"{B}\" \
        -DCONFIG_TCC_ELFINTERP=\"\" \
        -DCONFIG_TCC_LIBPATHS=\"{B}\" \
        -DCONFIG_TCC_SYSINCLUDEPATHS=\"\" \
        -DTCC_LIBGCC=\"libc.a\" \
        -DTCC_LIBTCC1=\"libtcc1.a\" \
        -DCONFIG_TCCBOOT=1 \
        -DCONFIG_TCC_STATIC=1 \
        -DCONFIG_USE_LIBGCC=1 \
        -DTCC_VERSION=\"0.9.28\" \
        -DCONFIG_TCC_SEMLOCK=0"

      if ! timeout 30 ${lib.getExe tasku-m2} $flags > tasku-m2-test; then
        ok=false
        echo "tasku-m2 failed on $file"
      fi
      if ! timeout 30 ${lib.getExe tasku-gcc} $flags > tasku-gcc-test; then
        ok=false
        echo "tasku-gcc failed on $file"
      fi
      if ! diff -q tasku-gcc-test tasku-m2-test; then
        ok=false
        echo "mismatch found"
      fi
      echo "$bn: ok"
      if $ok; then
        touch "$out"
      fi
    '';
  };

  unit-test = pkgs.stdenvNoCC.mkDerivation {
    pname = "taskucc-unit-test";
    version = "0.1.0";
    dontUnpack = true;

    buildPhase = ''
      cd ${./test}
      ok=true
      echo "=== TASKU-M2 ==="
      if ! ./run.sh "${lib.getExe tasku-m2}"; then
        ok=false
      fi
      echo "=== TASKU-GCC ==="
      if ! ./run.sh "${lib.getExe tasku-gcc}"; then
        ok=false
      fi
      if $ok; then
        touch "$out"
      fi
    '';
  };
}
