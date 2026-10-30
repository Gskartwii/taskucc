#!/bin/sh
RUNNER="@runner@"
RUN_TMP=$(mktemp -d)
if ! "$RUNNER" > $RUN_TMP/out.log; then
    echo "$RUNNER > $RUN_TMP/out.log: fail"
    ok=false
else
    if ! diff -q "$RUN_TMP/out.log" "$RUNNER-golden.log"; then
        echo "$RUNNER-golden.log fail:"
        diff "$RUN_TMP/out.log" "$RUNNER-golden.log" || true
        ok=false
    fi
fi
if ! $ok; then
    echo "FAIL"
    exit 1
fi
echo "PASS"
