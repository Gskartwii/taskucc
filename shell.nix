let
  pkgs = import (builtins.getFlake "nixpkgs") {};
in
  pkgs.mkShell {
    nativeBuildInputs = with pkgs; [
      clang-tools
      gdb
    ];
    env.KAK_EXTRA_CONFIG = pkgs.writeText "tasku-extra.kak" ''
      hook global WinSetOption filetype=(c|cpp) expandtab
    '';
  }
