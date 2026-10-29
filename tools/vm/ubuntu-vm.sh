#!/bin/bash
# Ubuntu LTS cloud-image VM for platforms.yml, on a GitHub-hosted ubuntu-24.04 runner.
# usage: ubuntu-vm.sh <series> <version> <sums|boot|run|collect|teardown>
#   sums      fetch the release's SHA256SUMS, check its GPG signature against the pinned UEC image
#             signing key, and print "sum=<sha256 of the image>" (for the workflow's cache key)
#   boot      fetch the image if the cache missed, check its SHA256, boot it under qemu+KVM with
#             cloud-init and a throwaway qcow2 overlay, wait for ssh
#   run       record the guest release, copy the tree in, install packages, run build-test.sh
#   collect   copy the serial console log somewhere an artifact upload can find it
#   teardown  stop the guest and remove everything (always safe to call)
set -euo pipefail
SERIES=$1 VERSION=$2 CMD=$3
WORK="${RUNNER_TEMP:-/tmp}/ubuntu-vm-${SERIES}"
CACHE="$HOME/cloud-images/${SERIES}"
BASE="https://cloud-images.ubuntu.com/releases/${SERIES}/release"
IMAGE="ubuntu-${VERSION}-server-cloudimg-amd64.img"
# Fingerprint of the UEC Image Automatic Signing Key. Any other key fails verification.
PIN=D2EB44626FDDC30B513D5BB71A5D6C4C7DB87C81
KEYRING=/usr/share/keyrings/ubuntu-cloudimage-keyring.gpg
SRC=$(cd "$(dirname "$0")/../.." && pwd)

g()   { "$WORK/g" "$@"; }

case "$CMD" in
sums)
    mkdir -p "$WORK"; cd "$WORK"
    curl -fsSL --retry 3 -o SHA256SUMS "$BASE/SHA256SUMS"
    curl -fsSL --retry 3 -o SHA256SUMS.gpg "$BASE/SHA256SUMS.gpg"
    gpg --batch --no-default-keyring --keyring "$KEYRING" --status-fd 1 --verify SHA256SUMS.gpg SHA256SUMS | tee gpg-status.txt >&2
    grep -q '^\[GNUPG:\] GOODSIG ' gpg-status.txt
    grep -q "^\[GNUPG:\] VALIDSIG ${PIN} " gpg-status.txt
    SUM=$(awk -v f="*${IMAGE}" '$2 == f {print $1}' SHA256SUMS)
    test -n "$SUM"
    echo "sum=${SUM}"
    ;;
boot)
    mkdir -p "$WORK" "$CACHE"; cd "$WORK"
    SUM=$(awk -v f="*${IMAGE}" '$2 == f {print $1}' SHA256SUMS)
    CACHED="$CACHE/${SUM}.img"
    if [ -f "$CACHED" ] && echo "${SUM}  ${CACHED}" | sha256sum -c --status -; then
        echo "image cache hit ${CACHED}"
    else
        echo "image cache miss: downloading ${BASE}/${IMAGE}"
        curl -fL --retry 3 -o "$CACHE/download.tmp" "$BASE/$IMAGE"
        echo "${SUM}  $CACHE/download.tmp" | sha256sum -c -
        mv "$CACHE/download.tmp" "$CACHED"
    fi
    # The cached image is never written: the guest boots from an overlay.
    qemu-img create -f qcow2 -F qcow2 -b "$CACHED" "$WORK/overlay.qcow2" 20G
    ssh-keygen -t ed25519 -f "$WORK/guest_key" -N "" -q
    PUB=$(cat "$WORK/guest_key.pub")
    cat > user-data << EOT
#cloud-config
hostname: lts-guest
ssh_pwauth: false
package_update: false
package_upgrade: false
users:
  - name: runner
    sudo: ALL=(ALL) NOPASSWD:ALL
    groups: sudo
    shell: /bin/bash
    lock_passwd: true
    ssh_authorized_keys:
      - ${PUB}
EOT
    printf 'instance-id: yj-lts-%s-%s\nlocal-hostname: lts-guest\n' "$SERIES" "${GITHUB_RUN_ID:-0}" > meta-data
    cloud-localds seed.img user-data meta-data
    PORT=$(python3 -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1",0)); print(s.getsockname()[1]); s.close()')
    echo "guest ssh port ${PORT}, $(nproc) vcpus, 6 GiB"
    : > serial.log
    qemu-system-x86_64 \
        -machine q35,accel=kvm -cpu host -m 6144 -smp "$(nproc)" \
        -display none -daemonize -pidfile "$WORK/qemu.pid" -serial file:"$WORK/serial.log" \
        -drive "file=$WORK/overlay.qcow2,if=virtio,format=qcow2,cache=writeback" \
        -drive "file=$WORK/seed.img,if=virtio,format=raw" \
        -netdev "user,id=net0,hostfwd=tcp:127.0.0.1:${PORT}-:22" -device virtio-net-pci,netdev=net0 \
        >qemu.log 2>&1
    cat > g << EOT
#!/bin/sh
exec ssh -i "$WORK/guest_key" -p "$PORT" -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o IdentitiesOnly=yes -o ConnectTimeout=5 -o ServerAliveInterval=30 -o LogLevel=ERROR runner@127.0.0.1 "\$@"
EOT
    cat > gcp << EOT
#!/bin/sh
exec scp -i "$WORK/guest_key" -P "$PORT" -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o IdentitiesOnly=yes -o LogLevel=ERROR "\$@"
EOT
    chmod +x g gcp
    for i in $(seq 1 60); do
        if ! kill -0 "$(cat qemu.pid)" 2>/dev/null; then echo "qemu exited before ssh"; tail -n 60 serial.log; exit 1; fi
        if g true 2>/dev/null; then echo "ssh up at try ${i}"; break; fi
        if [ "$i" = 60 ]; then echo "ssh did not come up"; tail -n 60 serial.log; exit 1; fi
        sleep 5
    done
    g 'cloud-init status --wait >/dev/null || true; cloud-init status'
    ;;
run)
    cd "$WORK"
    g 'uname -a; lsb_release -a 2>/dev/null; nproc; free -m'
    test "$(g 'lsb_release -cs')" = "$SERIES"
    {
        echo "### Ubuntu ${VERSION} (${SERIES}) guest"
        echo "- uname -r: \`$(g 'uname -r')\`"
        echo "- lsb_release: \`$(g 'lsb_release -ds')\`"
    } >> "${GITHUB_STEP_SUMMARY:-/dev/null}"
    tar -C "$SRC" --exclude=./.git -cf src.tar .
    "$WORK/gcp" src.tar runner@127.0.0.1:src.tar
    g 'mkdir -p ~/src && tar -C ~/src -xf ~/src.tar && rm ~/src.tar'
    g 'sudo DEBIAN_FRONTEND=noninteractive apt-get update -y && sudo DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends cmake build-essential python3 lsb-release'
    g 'gcc --version | head -1; cmake --version | head -1'
    g 'cd ~/src && sh tools/vm/build-test.sh'
    ;;
collect)
    mkdir -p "$WORK/logs"
    cp "$WORK/serial.log" "$WORK/qemu.log" "$WORK/logs/" 2>/dev/null || true
    echo "logs=$WORK/logs"
    ;;
teardown)
    if [ -f "$WORK/qemu.pid" ]; then
        pid=$(cat "$WORK/qemu.pid")
        if kill -0 "$pid" 2>/dev/null; then
            g 'sudo poweroff' >/dev/null 2>&1 || true
            for i in $(seq 1 20); do kill -0 "$pid" 2>/dev/null || break; sleep 1; done
            kill -9 "$pid" 2>/dev/null || true
        fi
        echo "guest stopped (qemu pid ${pid})"
    fi
    rm -rf "$WORK"
    ;;
*) echo "unknown command $CMD" >&2; exit 2 ;;
esac
