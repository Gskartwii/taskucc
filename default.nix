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
    "dynstring.h"
    "tasku_file.h"
    "tasku_pp.h"
  ];
  src = [
    "util.c"
    "dynstring.c"
    "tasku_file.c"
    "tasku_pp.c"
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
    "-g3"
    "-Og"
    "-Wall"
    "-Wextra"
    "-Wpedantic"
    "-Werror"
    "-Wconversion"
    "-Warith-conversion"
    "-fsanitize=undefined"
  ];
in
  rec {
    tasku-gcc = pkgs.stdenv.mkDerivation {
      pname = "tasku-cc-gcc";
      version = "0.1.0";
      src = ./src;
      buildPhase = ''
        ${lib.strings.concatMapStringsSep "\n" (file: "$CC ${builtins.concatStringsSep " " cflags} -c ${file}") src}
        $CC -fsanitize=undefined -o tasku ${lib.strings.concatMapStringsSep " " (builtins.replaceStrings [".c"] [".o"]) src}
      '';
      installPhase = ''
        mkdir -p $out/bin
        cp tasku $out/bin/tasku-gcc
      '';
      dontStrip = true;
      meta.mainProgram = "tasku-gcc";
    };
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
        for file in ${tinycc-src}/*.{h,c}; do
          case "$file" in
            */coff.h)
              continue
              ;;
          esac

          bn=$(basename "$file")
          if ! timeout 1 ${lib.getExe tasku-m2} "$file" > tasku-m2-test; then
            ok=false
            echo "tasku-m2 failed on $file"
            continue
          fi
          if ! timeout 1 ${lib.getExe tasku-gcc} "$file" > tasku-gcc-test; then
            ok=false
            echo "tasku-gcc failed on $file"
            continue
          fi
          if ! diff -q tasku-gcc-test tasku-m2-test; then
            ok=false
            echo "mismatch found on $file"
            continue
          fi
          echo "$bn: ok"
        done
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
