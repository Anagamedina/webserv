#!/usr/bin/env bash
set -euo pipefail

# Stress test for CGI POST endpoint
# Default scenario matches the tester-style load: 20 workers x 5 iterations

URL="${URL:-http://127.0.0.1:8080/directory/youpi.bla}"
WORKERS="${WORKERS:-20}"
REPEATS="${REPEATS:-5}"
PAYLOAD="${PAYLOAD:-tests/100mb.test}"
CONTENT_TYPE="${CONTENT_TYPE:-test/file}"
TIMEOUT="${TIMEOUT:-180}"
TMP_DIR="${TMP_DIR:-/tmp/webserv_stress_$$}"
SAVE_BODIES="${SAVE_BODIES:-0}"

usage() {
  cat <<EOF
Usage: $0 [options]

Options:
  -u URL            Target URL (default: $URL)
  -w WORKERS        Number of parallel workers (default: $WORKERS)
  -r REPEATS        Requests per worker (default: $REPEATS)
  -p PAYLOAD        Payload file path (default: $PAYLOAD)
  -t TIMEOUT        Curl max-time seconds (default: $TIMEOUT)
  -c CONTENT_TYPE   Content-Type header (default: $CONTENT_TYPE)
  -s SAVE_BODIES    Save full response bodies (0/1, default: $SAVE_BODIES)
  -h                Show this help

Examples:
  $0
  $0 -u http://127.0.0.1:8080/directory/youpi.bla -w 20 -r 5 -p tests/100mb.test
EOF
}

while getopts ":u:w:r:p:t:c:s:h" opt; do
  case "$opt" in
    u) URL="$OPTARG" ;;
    w) WORKERS="$OPTARG" ;;
    r) REPEATS="$OPTARG" ;;
    p) PAYLOAD="$OPTARG" ;;
    t) TIMEOUT="$OPTARG" ;;
    c) CONTENT_TYPE="$OPTARG" ;;
    s) SAVE_BODIES="$OPTARG" ;;
    h)
      usage
      exit 0
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      usage
      exit 1
      ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
      usage
      exit 1
      ;;
  esac
done

if [[ ! -f "$PAYLOAD" ]]; then
  echo "[ERROR] Payload file not found: $PAYLOAD" >&2
  echo "Create one with: dd if=/dev/zero of=tests/100mb.test bs=1M count=100" >&2
  exit 1
fi

mkdir -p "$TMP_DIR"
RESULTS_FILE="$TMP_DIR/results.tsv"
: > "$RESULTS_FILE"

echo "[INFO] URL=$URL"
echo "[INFO] WORKERS=$WORKERS REPEATS=$REPEATS TOTAL=$((WORKERS * REPEATS))"
echo "[INFO] PAYLOAD=$PAYLOAD"
echo "[INFO] TMP_DIR=$TMP_DIR"
echo "[INFO] SAVE_BODIES=$SAVE_BODIES"

run_worker() {
  local worker_id="$1"
  local i code curl_status time_total body_bytes out_file err_file sample out_target

  for ((i=1; i<=REPEATS; i++)); do
    out_file="$TMP_DIR/resp_w${worker_id}_i${i}.bin"
    err_file="$TMP_DIR/resp_w${worker_id}_i${i}.err"

    if [[ "$SAVE_BODIES" == "1" ]]; then
      out_target="$out_file"
    else
      out_target="/dev/null"
    fi

    set +e
    read -r code time_total body_bytes < <(
      curl -sS \
        --max-time "$TIMEOUT" \
        -X POST \
        -H "Content-Type: $CONTENT_TYPE" \
        --data-binary "@$PAYLOAD" \
        -o "$out_target" \
        -w "%{http_code} %{time_total} %{size_download}" \
        "$URL" 2>"$err_file"
    )
    curl_status=$?
    set -e

    if [[ $curl_status -ne 0 ]]; then
      code="CURL_ERR"
      time_total="-1"
      body_bytes="0"
      sample="exit=$curl_status $(tr '\n' ' ' < "$err_file" | head -c 120)"
    else
      sample=""
      if [[ "$code" != "200" ]]; then
        if [[ "$SAVE_BODIES" == "1" && -f "$out_file" ]]; then
          sample="$(head -c 140 "$out_file" | tr '\n' ' ' | tr '\r' ' ')"
        else
          sample="$(tr '\n' ' ' < "$err_file" | head -c 140)"
        fi
      fi
    fi

    printf "%s\t%s\t%s\t%s\t%s\t%s\n" \
      "$worker_id" "$i" "$code" "$time_total" "$body_bytes" "$sample" >> "$RESULTS_FILE"

    echo "worker=$worker_id iter=$i code=$code time=${time_total}s body=$body_bytes"
  done
}

export URL WORKERS REPEATS PAYLOAD CONTENT_TYPE TIMEOUT TMP_DIR RESULTS_FILE
export -f run_worker

seq 1 "$WORKERS" | xargs -P "$WORKERS" -I{} bash -c 'run_worker "$@"' _ {}

echo
echo "=== SUMMARY (status counts) ==="
awk -F'\t' '{count[$3]++} END {for (k in count) printf "%s\t%d\n", k, count[k]}' "$RESULTS_FILE" | sort

echo
echo "=== NON-200 OR CURL_ERR (first 40) ==="
awk -F'\t' '$3 != "200" {printf "worker=%s iter=%s code=%s time=%s body=%s sample=%s\n", $1,$2,$3,$4,$5,$6}' "$RESULTS_FILE" | head -40 || true

echo
echo "[DONE] Detailed artifacts in: $TMP_DIR"
echo "[TIP] Tail server logs in parallel to correlate failures."
