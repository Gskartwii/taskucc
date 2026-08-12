{
  stdenv,
  qemu-user,
  pkgsBuildTarget,
  stdenvNoCC,
  tasku-m2,
  tasku-gcc,
  fetchgit,
  lib,
  newScope,
  pv,
}: let
  tinycc-src = fetchgit {
    url = "https://repo.or.cz/tinycc.git";
    rev = "a338258d309c888bde96b2d1f206299231a54ddf";
    hash = "sha256-R1Kyycihw5rKu+vv/GzEMPAdaApW0lrIESjrbnEa2Dg=";
  };
in
  lib.makeScope newScope (self:
    with self; {
      # We want cc and binutils to target targetPlatform so we can link with
      # the output of our taskucc.
      unit-test = pkgsBuildTarget.stdenv.mkDerivation {
        pname = "taskucc-unit-test";
        version = "0.1.0";
        dontUnpack = true;

        nativeBuildInputs = [qemu-user];
        strictDeps = true;

        env.QEMU_TARGET = stdenv.targetPlatform.qemuArch;

        buildPhase = ''
          cd ${./.}
          ok=true
          echo "=== TASKU-M2 ==="
          if ! ./run.sh "${lib.getExe tasku-m2}"; then
            ok=false
          fi
          echo "=== TASKU-GCC ==="
          if ! ./run.sh "${lib.getExe tasku-gcc.debug}"; then
            ok=false
          fi
          if $ok; then
            touch "$out"
          fi
        '';
      };
      compare-m2-gcc = stdenvNoCC.mkDerivation {
        pname = "compare-taskucc-across-m2-gcc";
        version = "0.1.0";
        dontUnpack = true;
        nativeBuildInputs = [pv];

        buildPhase = ''
          ok=true

          file=${tinycc-src}/tcc.c
          flags="\
            -E \
            $file \
            -I ${../tasku-libc/include} \
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

          if ! timeout 5 ${lib.getExe tasku-gcc.debug} $flags | pv -r  > tasku-gcc-test; then
            ok=false
            echo "tasku-gcc failed on $file"
          fi
          if ! timeout 90 ${lib.getExe tasku-m2} $flags | pv -r > tasku-m2-test; then
            ok=false
            echo "tasku-m2 failed on $file; timeout"
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
    })
