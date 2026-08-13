#!/bin/sh
set -euo pipefail

SCRIPT_DIR=$(dirname $(realpath "$0"))
ok=true

for case_bin in $SCRIPT_DIR/compile-test/*; do
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
