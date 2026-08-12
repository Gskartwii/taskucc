let
  pkgs = import (builtins.getFlake "nixpkgs") {};
  lib = pkgs.lib;
  tasku-m2 = pkgs.callPackage ./nix/tasku-m2.nix {};
  tasku-gcc = pkgs.callPackage ./nix/tasku-gcc.nix {};
in
  lib.makeScope pkgs.newScope (self:
    with self; {
      inherit tasku-m2 tasku-gcc;
      tasku-both = pkgs.symlinkJoin {
        name = "tasku";
        paths = [tasku-m2 tasku-gcc.debug];
      };
      test = callPackage ./test {};
    })
