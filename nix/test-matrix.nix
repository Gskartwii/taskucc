{lite ? false}: let
  systems = [
    "i686-linux"
    "x86_64-linux"
    "aarch64-linux"
    "riscv64-linux"
  ];
  taskuPkgSet = import ../.;
  testOn = config: let
    taskuPkgSet' = taskuPkgSet config;
  in
    taskuPkgSet'.tasku-test.target-unit-test;
in
  builtins.listToAttrs (builtins.concatMap (localSystem:
    map (crossSystem: {
      name = "unit-${localSystem}-${crossSystem}";
      value =
        if !lite || localSystem != "riscv64-linux"
        then
          testOn {
            inherit localSystem crossSystem;
          }
        else "skipped testing on riscv64";
    })
    systems)
  systems)
  // {
    recurseForDerivations = true;
  }
