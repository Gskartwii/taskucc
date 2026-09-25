{
  localSystem ? "x86_64-unknown-linux-gnu",
  crossSystem ? "x86_64-unknown-linux-musl",
}: let
  pkgsImport = import (builtins.getFlake "github:NixOS/nixpkgs/pull/555663/head");
  pkgsBase = pkgsImport {
    inherit localSystem;
    crossSystem =
      ((pkgsImport {}).lib.systems.elaborate crossSystem)
      // {
        isStatic = true;
      };
  };
in
  (pkgsBase.extend (final: prev:
    with final; {
      tasku-m2 = callPackage ./nix/tasku-m2.nix {
        srcFiles = common.src;
      };
      tasku-gcc = callPackage ./nix/tasku-gcc.nix {
        srcFiles = common.src;
      };
      common = callPackage ./nix/common.nix {};
      tasku-both = symlinkJoin {
        name = "tasku";
        paths = [tasku-m2 tasku-gcc.debug];
      };
      tasku-test = targetPackages.callPackage ./test {};
    })).buildPackages
