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
    if ! timeout 1 "$RUNNER" "$case_dir/test.c" > "$RUN_TMP/test.log"; then
        echo "$case_dir/test.c: fail"
        ok=false
        continue
    fi
    if ! diff -q "$RUN_TMP/test.log" "$case_dir/out.log"; then
        echo "$case_dir/out.log: fail:"
        diff "$RUN_TMP/test.log" "$case_dir/out.log"
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
