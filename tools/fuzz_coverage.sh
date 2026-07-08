#!/usr/bin/env bash
#
# Measure source coverage of a fuzz harness over its committed corpus plus a batch of
# pseudo-random inputs (the standalone driver's two modes). This approximates what the
# coverage-guided fuzzer reaches and — used the same way before and after a change — gives a
# comparable before/after number for the deserialization layer.
#
# Usage: tools/fuzz_coverage.sh <connection|structured>   (default: connection)
#
# Needs clang + llvm-profdata/llvm-cov (on macOS: xcrun). Not run in CI; a local dev tool.
set -euo pipefail

cd "$(dirname "$0")/.."

target="${1:-connection}"
iters="${FUZZ_COV_ITERS:-200000}"
out="$(mktemp -d)"
bin="$out/harness"

PROFDATA() { if command -v xcrun >/dev/null 2>&1; then xcrun llvm-profdata "$@"; else llvm-profdata "$@"; fi; }
COV() { if command -v xcrun >/dev/null 2>&1; then xcrun llvm-cov "$@"; else llvm-cov "$@"; fi; }

# The deserialization layer we care about.
FILES=(
  source/yojimbo_channel.cpp
  source/yojimbo_connection.cpp
  source/yojimbo_reliable_ordered_channel.cpp
  source/yojimbo_unreliable_unordered_channel.cpp
)

COVFLAGS="-fprofile-instr-generate -fcoverage-mapping"
DEFS="-DYOJIMBO_DEBUG -DNETCODE_DEBUG -DRELIABLE_DEBUG -DSERIALIZE_DEBUG"
INCS="-I. -Iinclude -Isodium -Itlsf -Inetcode -Ireliable -Iserialize -Ifuzz"

case "$target" in
  connection)  src=fuzz/fuzz_connection.cpp; corpus=fuzz/corpus/fuzz_connection ;;
  structured)  src=fuzz/fuzz_connection_structured.cpp; corpus=fuzz/corpus/fuzz_connection_structured ;;
  *) echo "unknown target: $target" >&2; exit 1 ;;
esac

echo "building $src with coverage instrumentation..."
clang++ -std=c++11 -DFUZZ_STANDALONE $DEFS $INCS $COVFLAGS -g \
  "$src" source/*.cpp netcode/netcode.c reliable/reliable.c tlsf/tlsf.c sodium/sodium.c \
  -o "$bin" 2>/dev/null

echo "running over corpus ($corpus) + $iters random inputs..."
LLVM_PROFILE_FILE="$out/corpus.profraw" "$bin" "$corpus"/* >/dev/null 2>&1 || true
LLVM_PROFILE_FILE="$out/random.profraw" FUZZ_ITERS="$iters" "$bin" >/dev/null 2>&1 || true

PROFDATA merge -sparse "$out"/*.profraw -o "$out/cov.profdata"

echo
echo "=== coverage: deserialization layer ($target) ==="
COV report "$bin" -instr-profile="$out/cov.profdata" "${FILES[@]}" 2>/dev/null

# Optional: emit an annotated report to a file for line-level inspection.
if [ -n "${FUZZ_COV_ANNOTATE:-}" ]; then
  COV show "$bin" -instr-profile="$out/cov.profdata" "${FILES[@]}" \
    -format=text > "${FUZZ_COV_ANNOTATE}" 2>/dev/null
  echo "annotated report written to ${FUZZ_COV_ANNOTATE}"
fi

echo
echo "(profile data in $out)"
