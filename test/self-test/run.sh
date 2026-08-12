#!/bin/sh
set -euo pipefail

RUNNER="$1"

exec "$RUNNER" run-tests

