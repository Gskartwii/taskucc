{
  lib,
  stdenvNoCC,
  fetchFromGitHub,
  minimal-bootstrap,
  srcFiles,
}: let
  inherit (import ./common.nix) src;
  system = stdenvNoCC.hostPlatform.system;
  m2libc = fetchFromGitHub {
    owner = "oriansj";
    repo = "M2libc";
    rev = "ca023d8dc855171fd0618951add5817e0e568fca";
    hash = "sha256-7xjH/Dti62/s4cRVLE5NMDYEsiKIfuaDsF9a8+glW5o=";
  };
  m2arch =
    {
      x86_64-linux = "amd64";
      i686-linux = "x86";
      aarch64-linux = "aarch64";
      armv7l-linux = "armv7l";
      riscv64-linux = "riscv64";
      riscv32-linux = "riscv32";
    }.${
      system
    };
  flag64 = lib.optionalString stdenvNoCC.hostPlatform.is64bit "--64";
  baseaddr =
    {
      x86_64-linux = "0x00600000";
      i686-linux = "0x08048000";
      aarch64-linux = "0x00600000";
      armv7l-linux = "0x00010000";
      riscv64-linux = "0x00600000";
      riscv32-linux = "0x00600000";
    }.${
      system
    };
  includes-m2 = [
    "sys/types.h"
    "stddef.h"
    "${m2arch}/linux/fcntl.c"
    "fcntl.c"
    "sys/utsname.h"
    "${m2arch}/linux/unistd.c"
    "${m2arch}/linux/sys/stat.c"
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
    "target/${stdenvNoCC.targetPlatform.system}/registers.h"
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
  m2-all = includes ++ local_hdrs ++ srcFiles;
in
  stdenvNoCC.mkDerivation {
    pname = "tasku-cc";
    version = "0.1.0";
    src = ../src;
    nativeBuildInputs = [
      minimal-bootstrap.stage0-posix.mescc-tools
    ];
    buildPhase = ''
      M2-Planet --architecture ${m2arch} \
        -f ${lib.strings.concatStringsSep " -f " m2-all} \
        --debug \
        -o ./tasku.M1

      blood-elf --little-endian ${flag64} -f tasku.M1 -o tasku-footer.M1

      M1 --architecture ${m2arch} \
        --little-endian \
        -f ${m2libc}/${m2arch}/${m2arch}_defs.M1 \
        -f ${m2libc}/${m2arch}/libc-full.M1 \
        -f tasku.M1 \
        -f tasku-footer.M1 \
        -o tasku.hex2

      hex2 --architecture ${m2arch} \
        --little-endian \
        --base-address ${baseaddr} \
        -f ${m2libc}/${m2arch}/ELF-${m2arch}-debug.hex2 \
        -f tasku.hex2 \
        -o tasku
    '';

    installPhase = ''
      mkdir -p $out/bin/
      cp tasku.M1 $out/bin/tasku.M1
      cp tasku $out/bin/tasku-m2
    '';
    meta.mainProgram = "tasku-m2";
  }
