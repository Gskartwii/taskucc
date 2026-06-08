let
  pkgs = import (builtins.getFlake "nixpkgs") {};
  lib = pkgs.lib;
  m2libc = pkgs.minimal-bootstrap.m2libc;
  includes = [
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
in
  pkgs.stdenvNoCC.mkDerivation {
    pname = "tasku-cc";
    version = "0.1.0";
    src = ./.;
    nativeBuildInputs = [
      pkgs.m2-planet
      pkgs.mescc-tools
    ];
    buildPhase = ''
      M2-Planet --architecture amd64 \
        -f ${lib.strings.concatMapStringsSep " -f " (file: "${m2libc}/${file}") includes} \
        tasku.c \
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
      cp tasku $out/bin/tasku
    '';
  }
