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
        echo "== $config failed: rerunning the test binary on a pty"
        cat > /tmp/ptyrun.c << 'CEOF'
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#if defined(__linux__)
#include <pty.h>
#else
#include <util.h>
#endif
int main(int argc, char **argv) {
    int fd; pid_t pid = forkpty(&fd, NULL, NULL, NULL);
    if (pid == 0) { execv(argv[1], argv + 1); _exit(127); }
    char buf[4096]; ssize_t n;
    while ((n = read(fd, buf, sizeof buf)) > 0) fwrite(buf, 1, n, stdout);
    int st = 0; waitpid(pid, &st, 0);
    printf("\nexit status 0x%x (signal %d)\n", st, WIFSIGNALED(st) ? WTERMSIG(st) : 0);
    return 0;
}
CEOF
        cc -o /tmp/ptyrun /tmp/ptyrun.c -lutil && /tmp/ptyrun "$PWD/$dir/bin/test" | tail -n 30 || true
        exit 1
    fi
done
echo "== ok"
