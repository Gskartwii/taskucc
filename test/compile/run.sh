#!/bin/sh
set -euo pipefail

SCRIPT_DIR=$(dirname $(realpath "$0"))
RUNNER="$1"
cd "$SCRIPT_DIR"
RUN_TMP=$(mktemp -d)
ok=true

for case_dir in cases/*; do
    if [ -x "$case_dir/run.sh" ]; then
        if ! "$case_dir/run.sh" "$RUNNER"; then
            echo "$case_dir: fail: $?"
            ok=false
        fi
        continue
    fi
    if ! timeout 1 "$RUNNER" "$case_dir/test.c" > "$RUN_TMP/test.S"; then
        echo "COMPILE $case_dir/test.c: fail"
        ok=false
        continue
    fi
    if ! $CC "$RUN_TMP/test.S" "$case_dir/driver.c" -o "$RUN_TMP/driver"; then
        echo "LINK $case_dir/test.c: fail"
        ok=false
        continue
    fi
    if ! timeout 1 "$RUN_TMP/driver"; then
        echo "RUN $case_dir/test.c: fail"
        ok=false
        continue
    fi
    echo "$case_dir: ok"
done
if ! $ok; then
    echo "FAIL"
    exit 1
fi
echo "PASS"

