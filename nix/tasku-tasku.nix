{
  stdenv,
  lib,
  srcFiles,
  buildPackages,
}: let
  mk-tasku-tasku = compiler:
    stdenv.mkDerivation {
      pname = "tasku-cc-tasku";
      version = "0.1.0";
      src = ../src;
      buildPhase = ''
        ${lib.strings.concatMapStringsSep "\n" (file: ''
            $TASKU ${file} -I${../tasku-libc/include} > \
              ${builtins.replaceStrings ["/" ".c"] ["_" ".s"] file} || (echo failed to produce: ${file}; exit 1)
          '')
          srcFiles}
        $CC -o tasku ${lib.strings.concatMapStringsSep " " (builtins.replaceStrings ["/" ".c"] ["_" ".s"]) srcFiles}
      '';
      installPhase = ''
        mkdir -p $out/bin
        cp tasku $out/bin/tasku-tasku
      '';
      env.TASKU = lib.getExe compiler;
      dontStrip = true;
      meta.mainProgram = "tasku-tasku";
    };
in {
  tasku-tasku-gcc = mk-tasku-tasku buildPackages.tasku-gcc.debug;
}
