let
  pkgs = import (builtins.getFlake "nixpkgs") {};
  lib = pkgs.lib;
in
  # Need clang-format 23 for PackArguments: OnePerLine...
  assert lib.versionOlder pkgs.llvmPackages_git.clang-tools.version "24";
  assert lib.versionAtLeast pkgs.llvmPackages_git.clang-tools.version "23";
  assert lib.versionOlder pkgs.llvmPackages.clang-tools.version "23";
    pkgs.mkShell {
      nativeBuildInputs = with pkgs; [
        llvmPackages_git.clang-tools
        gdb
      ];
      env.KAK_EXTRA_CONFIG = pkgs.writeText "tasku-extra.kak" ''
        hook global WinSetOption filetype=(c|cpp) %{
          expandtab
          set window formatcmd clang-format
        }
      '';
    }
