#!/usr/bin/env bash

set -euo pipefail

HOST="${HOST:-127.0.0.1}"
PORT="${PORT:-8080}"
REQS="${REQS:-200}"
CONCURRENCY="${CONCURRENCY:-8}"
TIMEOUT="${TIMEOUT:-5}"
URL="${1:-http://${HOST}:${PORT}/}"

echo "Hitting ${URL} with ${REQS} POST requests (concurrency=${CONCURRENCY}, timeout=${TIMEOUT}s)"

seq "${REQS}" | xargs -P "${CONCURRENCY}" -I{} bash -c '
  code=$(curl -s -o /dev/null -w "%{http_code}\n" --max-time "${TIMEOUT}" -X POST --data "load_test=${RANDOM}&i=$1" "$2")
  rc=$?
  if [ $rc -eq 28 ]; then
    echo "timeout"
    exit 124
  elif [ $rc -ne 0 ]; then
    echo "error"
    exit $rc
  fi
  echo "${code}"
' _ {} "${URL}" | awk '
  {codes[$1]++}
  END {
    print "Response code counts:"
    for (c in codes) printf("  %s: %d\n", c, codes[c]);
  }
'
