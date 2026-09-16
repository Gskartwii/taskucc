{stdenvNoCC}: let
  system = stdenvNoCC.targetPlatform.system;
in {
  src = [
    "util.c"
    "3rdparty/intscan.c"
    "3rdparty/floatscan.c"
    "call_itf.c"
    "codegen.c"
    "compile.c"
    "dynarray.c"
    "dynhash.c"
    "dynstring.c"
    "string_list.c"
    "soft_u64.c"
    "soft_u128.c"
    "soft_float.c"
    "machine.c"
    "type.c"
    "expr.c"
    "attribute.c"
    "decl.c"
    "statement.c"
    "parse.c"
    "format.c"
    "tasku_file.c"
    "tasku_pp.c"
    "test.c"
    "tasku.c"

    "target/${system}.c"
    "target/${system}/codegen.c"
    "target/${system}/call_itf.c"
  ];
}
