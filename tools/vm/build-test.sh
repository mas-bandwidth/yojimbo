#!/bin/sh
# Build and test yojimbo, debug then release, exactly as the build-test job in
# .github/workflows/ci.yml does. Runs inside a platform VM (see platforms.yml); plain POSIX sh
# so it runs the same under Linux dash, FreeBSD sh, NetBSD sh and OpenBSD ksh.
set -eu
cd "$(dirname "$0")/../.."
PATH="$PATH:/usr/local/bin:/usr/pkg/bin"
export PATH

echo "== platform"
uname -a
cmake --version | head -1
c++ --version | head -1 || true

JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)
echo "== $JOBS build jobs"

for config in Debug Release; do
    dir="build-$(echo "$config" | tr 'A-Z' 'a-z')"
    echo "== $config"
    cmake -B "$dir" -DCMAKE_BUILD_TYPE="$config"
    cmake --build "$dir" -j "$JOBS"
    ctest --test-dir "$dir" --output-on-failure
done
echo "== ok"
