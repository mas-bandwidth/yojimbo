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
    if ! ctest --test-dir "$dir" --output-on-failure; then
        # A test's stdout is lost when it dies on a trap (it is block-buffered on a pipe), so
        # run the failing binaries again on a pty, where it is line-buffered, to see how far
        # they got. The BSD and util-linux script(1) take different arguments; try both.
        echo "== $config failed: rerunning the test binaries on a pty"
        for t in "$dir"/bin/test; do
            echo "== $t"
            # a watchdog, because script(1) can outlive a child that trapped
            rm -f pty.log
            script -q pty.log "$t" </dev/null >/dev/null 2>&1 &
            pid=$!
            sleep 40
            kill "$pid" 2>/dev/null || true
            [ -s pty.log ] || { script -qec "$t" pty.log </dev/null >/dev/null 2>&1 & sleep 40; kill $! 2>/dev/null || true; }
            tail -n 25 pty.log || true
        done
        exit 1
    fi
done
echo "== ok"
