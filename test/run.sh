#!/bin/sh
set -euo pipefail

SCRIPT_DIR=$(dirname $(realpath "$0"))
ok=true

for case_dir in $SCRIPT_DIR/*/; do
    if [ -x "$case_dir/run.sh" ]; then
        if ! "$case_dir/run.sh" "$@"; then
            echo "$case_dir: fail: $?"
            ok=false
        fi
        continue
        echo "$case_dir: ok"
    else
        echo "skip: $case_dir"
    fi
done
if ! $ok; then
    echo "FAIL"
    exit 1
fi
echo "PASS"

