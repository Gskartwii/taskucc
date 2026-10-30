#!/bin/sh
set -euo pipefail

RUN_TMP=$(mktemp -d)
SCRIPT_DIR=$(dirname $(realpath "$0"))
ok=true

for case_bin in $SCRIPT_DIR/compile-test/*; do
    case "$case_bin" in
        *.log)
            continue
            ;;
        *)
            ;;
    esac
    if [ -f "$case_bin-golden.log" ]; then
        if ! timeout 1 "$case_bin" > $RUN_TMP/out.log; then
            echo "$case_bin > $RUN_TMP/out.log: fail"
            ok=false
            continue
        fi
        if ! diff -q "$RUN_TMP/out.log" "$case_bin-golden.log"; then
            echo "$case_bin-golden.log fail:"
            diff "$RUN_TMP/out.log" "$case_bin-golden.log" || true
            ok=false
            continue
        fi
        echo "$case_bin: diff-ok"
        continue
    fi
    if ! timeout 1 "$case_bin"; then
        echo "$case_bin: fail: $?"
        ok=false
        continue
    fi
    echo "$case_bin: ok"
done

if ! $ok; then
    echo "FAIL"
    exit 1
fi
echo "PASS"
touch "$out"
