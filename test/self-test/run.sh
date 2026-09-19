#!/bin/sh
set -euo pipefail

RUNNER="$1"

exec timeout 10 "$RUNNER" run-tests
