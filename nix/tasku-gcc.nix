{
  stdenv,
  lib,
  srcFiles,
}: let
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
    "-I${../src}"
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
    stdenv.mkDerivation {
      pname = "tasku-cc-gcc";
      version = "0.1.0";
      src = ../src;
      buildPhase = ''
        ${lib.strings.concatMapStringsSep "\n" (file: "$CC ${builtins.concatStringsSep " " cf} -o ${builtins.replaceStrings ["/" ".c"] ["_" ".o"] file} -c ${file}") srcFiles}
        $CC ${builtins.concatStringsSep " " cf} -o tasku ${lib.strings.concatMapStringsSep " " (builtins.replaceStrings ["/" ".c"] ["_" ".o"]) srcFiles}
      '';
      installPhase = ''
        mkdir -p $out/bin
        cp tasku $out/bin/tasku-gcc
      '';
      dontStrip = true;
      meta.mainProgram = "tasku-gcc";
    };
in {
  debug = mk-tasku-gcc cflags-debug;
  opt = mk-tasku-gcc cflags-opt;
}
