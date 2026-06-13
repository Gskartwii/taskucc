let
  pkgs = import (builtins.getFlake "nixpkgs") {};
  lib = pkgs.lib;
  m2libc = pkgs.minimal-bootstrap.m2libc;
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
    "tasku_file.h"
    "tasku_pp.h"
  ];
  src = [
    "util.c"
    "tasku_file.c"
    "tasku_pp.c"
    "tasku.c"
  ];

  m2-all = includes ++ local_hdrs ++ src;
in
  {
    tasku-gcc = pkgs.stdenv.mkDerivation {
      pname = "tasku-cc-gcc";
      version = "0.1.0";
      src = ./.;
      buildPhase = ''
        ${lib.strings.concatMapStringsSep "\n" (file: "$CC -std=c90 -g -Wall -Wextra -Wpedantic -Werror -c ${file}") src}
        $CC -o tasku ${lib.strings.concatMapStringsSep " " (builtins.replaceStrings [".c"] [".o"]) src}
      '';
      installPhase = ''
        mkdir -p $out/bin
        cp tasku $out/bin/tasku
      '';
      dontStrip = true;
    };
    tasku-m2 = pkgs.stdenvNoCC.mkDerivation {
      pname = "tasku-cc";
      version = "0.1.0";
      src = ./.;
      nativeBuildInputs = [
        pkgs.minimal-bootstrap.stage0-posix.mescc-tools
      ];
      buildPhase = ''
        M2-Planet --architecture amd64 \
          -f ${lib.strings.concatStringsSep " -f " m2-all} \
          -f m2_shim.h \
          -f util.c \
          -f tasku.c \
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

        cat tasku.hex2

        hex2 --architecture amd64 \
          --little-endian \
          --base-address 0x00600000 \
          -f ${m2libc}/amd64/ELF-amd64-debug.hex2 \
          -f tasku.hex2 \
          -o tasku
      '';

      installPhase = ''
        mkdir -p $out/bin/
        cp tasku $out/bin/tasku
      '';
    };
  }
