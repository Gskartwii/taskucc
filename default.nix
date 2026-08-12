let
  pkgs = import (builtins.getFlake "nixpkgs") {};
  tasku-m2 = pkgs.callPackage ./nix/tasku-m2.nix {
    srcFiles = common.src;
  };
  tasku-gcc = pkgs.callPackage ./nix/tasku-gcc.nix {
    srcFiles = common.src;
  };
  common = pkgs.callPackage ./nix/common.nix {};
in
  pkgs.lib.makeScope pkgs.newScope (self:
    with self; {
      inherit tasku-m2 tasku-gcc;
      tasku-both = pkgs.symlinkJoin {
        name = "tasku";
        paths = [tasku-m2 tasku-gcc.debug];
      };
      test = callPackage ./test {};
    })
