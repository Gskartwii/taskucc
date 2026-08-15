let
  pkgs = import (builtins.getFlake "nixpkgs") {};
  lib = pkgs.lib;
in
  pkgs.mkShell {
    nativeBuildInputs = with pkgs; [
      llvmPackages_23.clang-tools
      llvmPackages_23.llvm
      gdb
    ];
    env.KAK_EXTRA_CONFIG = pkgs.writeText "tasku-extra.kak" ''
      hook global WinSetOption filetype=(c|cpp) %{
        expandtab
        set window formatcmd "clang-format --assume-filename=%val{bufname}"
      }
    '';
  }
