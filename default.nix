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
    "string_list.h"
    "tasku_file.h"
    "soft_u64.h"
    "type.h"
    "machine.h"
    "target/codegen.h"
    "codegen.h"
    "target/target.h"
    "expr.h"
    "attribute.h"
    "statement.h"
    "decl.h"
    "parse.h"
    "format.h"
    "tasku_pp.h"
    "compile.h"
    "test.h"
  ];
  src = [
    "util.c"
    "3rdparty/intscan.c"
    "codegen.c"
    "compile.c"
    "dynarray.c"
    "dynhash.c"
    "dynstring.c"
    "string_list.c"
    "soft_u64.c"
    "machine.c"
    "type.c"
    "expr.c"
    "attribute.c"
    "decl.c"
    "statement.c"
    "parse.c"
    "format.c"
    "tasku_file.c"
    "tasku_pp.c"
    "test.c"
    "tasku.c"

    "target/x86_64-linux.c"
    "target/x86_64-linux/codegen.c"
  ];

  m2-all = includes ++ local_hdrs ++ src;

  cflags = [
    "-std=c99"
    "-Wlong-long"
    "-Wall"
    "-Wextra"
    "-Wpedantic"
    "-Werror"
    "-Wconversion"
    "-Warith-conversion"
    "-Wno-variadic-macros"
    "-I${./src}"
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
        ${lib.strings.concatMapStringsSep "\n" (file: "$CC ${builtins.concatStringsSep " " cf} -o ${builtins.replaceStrings ["/" ".c"] ["_" ".o"] file} -c ${file}") src}
        $CC ${builtins.concatStringsSep " " cf} -o tasku ${lib.strings.concatMapStringsSep " " (builtins.replaceStrings ["/" ".c"] ["_" ".o"]) src}
      '';
      installPhase = ''
        mkdir -p $out/bin
        cp tasku $out/bin/tasku-gcc
      '';
      dontStrip = true;
      meta.mainProgram = "tasku-gcc";
    };
in
  lib.makeScope pkgs.newScope (self:
    with self; {
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
      test = callPackage ./test {};
    })
