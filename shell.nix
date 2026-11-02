let
  nixpkgs = import (builtins.getFlake "nixpkgs");
  nixpkgsUnstable = import (builtins.getFlake "nixpkgs/nixos-unstable");
  pkgs = nixpkgs {};
  pkgsUnstable = nixpkgsUnstable {};
  pkgsi686Cross = nixpkgs {crossSystem = "i686-unknown-linux-musl";};
  lib = pkgs.lib;
in
  pkgs.mkShell {
    nativeBuildInputs = with pkgs; [
      llvmPackages_23.clang-tools
      llvmPackages_23.llvm
      gdb
      perf
      pkgsCross.riscv64-musl.buildPackages.gdb
      pkgsCross.aarch64-multiplatform.buildPackages.gdb
      pkgsi686Cross.buildPackages.gdb
      pkgsUnstable.rr
    ];
    env.KAK_EXTRA_CONFIG = pkgs.writeText "tasku-extra.kak" ''
      hook global WinSetOption filetype=(c|cpp) %{
        expandtab
        set window formatcmd "clang-format --assume-filename=%val{bufname}"
      }
    '';
  }
