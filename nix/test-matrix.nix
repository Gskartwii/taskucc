let
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
      name = "${localSystem}-${crossSystem}";
      value = testOn {
        inherit localSystem crossSystem;
      };
    })
    systems)
  systems)
