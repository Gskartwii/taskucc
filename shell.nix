let
  pkgs = import (builtins.getFlake "nixpkgs") {};
in
  pkgs.mkShell {
    nativeBuildInputs = [pkgs.clang-tools];
  }
