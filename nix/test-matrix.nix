{lite ? false}: let
  hostSystems = [
    "i686-linux"
    "x86_64-linux"
    "aarch64-linux"
    "riscv64-linux"
  ];
  targetSystems = [
    "i686-unknown-linux-musl"
    "x86_64-unknown-linux-musl"
    "aarch64-unknown-linux-musl"
    "riscv64-unknown-linux-musl"
  ];
  taskuPkgSet = import ../.;
  testOn = config: let
    taskuPkgSet' = taskuPkgSet config;
  in {
    unit-test = taskuPkgSet'.tasku-test.target-unit-test;
    compare-m2-gcc = taskuPkgSet'.tasku-test.compare-m2-gcc;
    recurseForDerivations = true;
  };
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
    targetSystems)
  hostSystems)
  // {
    recurseForDerivations = true;
  }
