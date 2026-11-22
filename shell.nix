{system ? builtins.currentSystem}: let
  inputs = import ./nix/tamal/default.nix {
    inherit system;
  };
  nixpkgs = import inputs.nixpkgs;
  pkgs = nixpkgs {};
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
      rr
    ];
    env.KAK_EXTRA_CONFIG = pkgs.writeText "tasku-extra.kak" ''
      hook global WinSetOption filetype=(c|cpp) %{
        expandtab
        set window formatcmd "clang-format --assume-filename=%val{bufname}"
      }
    '';
  }
