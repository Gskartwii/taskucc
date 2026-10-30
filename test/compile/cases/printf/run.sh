#!/bin/sh
set -euo pipefail

SCRIPT_DIR=$(dirname $(realpath "$0"))/../..
RUNNER="$1"
cd "$SCRIPT_DIR"
RUN_TMP=$(mktemp -d)
ok=true

case_dir="cases/printf"

if ! timeout 10 "$RUNNER" "$case_dir/test.c" > "$RUN_TMP/test.S"; then
    echo "COMPILE cases/printf/test.c: fail"
    ok=false
else
    if ! $CC "$RUN_TMP/test.S" "$SCRIPT_DIR/taccrt.c" -o "$RUN_TMP/driver"; then
        echo "LINK $case_dir/test.c: fail"
        ok=false
    else
        mv "$RUN_TMP/driver" "$out/bin/compile-test/$(basename $case_dir)-$RUNNER-unwrapped"
        cp "$case_dir/golden.log" "$out/bin/compile-test/$(basename $case_dir)-$RUNNER-unwrapped-golden.log"
        sed "s^@runner@^$out/bin/compile-test/$(basename $case_dir)-$RUNNER-unwrapped^" "$case_dir/tester.sh" > "$out/bin/compile-test/$(basename $case_dir)-$RUNNER"
        chmod +x "$out/bin/compile-test/$(basename $case_dir)-$RUNNER"
        echo "$case_dir: ok"
    fi
fi
if ! $ok; then
    echo "FAIL"
    exit 1
fi
echo "PASS"
