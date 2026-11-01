{
  stdenv,
  lib,
  srcFiles,
  buildPackages,
}: let
  toXFile = toExt: path: builtins.replaceStrings ["/" ".c"] ["_" toExt] path;
  mk-tasku-tasku = compiler:
    stdenv.mkDerivation {
      pname = "tasku-cc-tasku";
      version = "0.1.0";
      src = ../src;
      buildPhase = ''
        ${lib.strings.concatMapStringsSep "\n" (file: ''
            $TASKU ${file} -I${../tasku-libc/include} > \
              ${toXFile ".s" file} || (echo failed to produce: ${file}; exit 1)
            $CC -o ${toXFile ".o" file} -c ${toXFile ".s" file} \
               || (echo failed to produce .o: ${file}; exit 1)
          '')
          srcFiles}
        $CC -o tasku ${lib.strings.concatMapStringsSep " " (toXFile ".o") srcFiles}
      '';
      installPhase = ''
        mkdir -p $out/bin
        cp tasku $out/bin/tasku-tasku
      '';
      env.TASKU = lib.getExe compiler;
      dontStrip = true;
      meta.mainProgram = "tasku-tasku";
    };
in rec {
  tasku-tasku-gcc = mk-tasku-tasku buildPackages.tasku-gcc.debug;
  tasku-tasku-m2 = mk-tasku-tasku buildPackages.tasku-m2;
  tasku-tasku-tasku = mk-tasku-tasku tasku-tasku-gcc;
}
