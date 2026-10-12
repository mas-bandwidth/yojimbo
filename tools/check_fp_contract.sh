#!/usr/bin/env bash
# check_fp_contract.sh
#
# Enforces the repository's FP-contraction policy on the CI workflows: every direct compiler
# invocation in .github/workflows/ci.yml and fuzz-nightly.yml must pass -ffp-contract=off
# (the hand-written compile lines are outside the CMake target flag system). Compiler
# invocations that go through upstream cmake (netcode's own build in ci.yml) are left alone.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
files=("$root/.github/workflows/ci.yml" "$root/.github/workflows/fuzz-nightly.yml")

fail=0
report=""

for f in "${files[@]}"; do
    i=0
    while IFS= read -r line || [ -n "$line" ]; do
        i=$((i + 1))
        trimmed="${line#"${line%%[![:space:]]*}"}"
        case "$trimmed" in
            clang++*|clang*|cc*|gcc*|g++*)
                cmd="$line"
                line_no=$i
                while : ; do
                    if [[ "$cmd" != *\\ ]]; then
                        break
                    fi
                    if ! IFS= read -r nxt; then
                        break
                    fi
                    i=$((i + 1))
                    cmd="$cmd"$'\n'"$nxt"
                done
                if [[ "$cmd" != *"-ffp-contract=off"* ]]; then
                    flattened="${cmd//$'\n'/ }"
                    printf -v entry "%s:%d: missing -ffp-contract=off: %s\n" "$f" "$line_no" "$flattened"
                    report+="$entry"
                    fail=1
                fi
                ;;
        esac
    done < "$f"
done

if [ "$fail" -ne 0 ]; then
    echo "FP contraction (-ffp-contract=off) is NOT set on these compilations:"
    printf "%b" "$report"
    exit 1
fi
echo "OK: every compiler invocation in the workflows passes -ffp-contract=off"
