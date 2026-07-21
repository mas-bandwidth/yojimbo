#!/usr/bin/env python3
"""Check yojimbo/STATE-MACHINE.md against the implementation.

Drives a real client and server through connect, steady state and disconnect,
records every client state transition, and checks the observed behaviour
against the documented machine. Nothing here consults yojimbo's source; the
legal transitions below are transcribed from STATE-MACHINE.md.

usage: python3 tools/conformance/verify_state_machine.py [--cxx CXX] [--build DIR]
exit:  0 = they agree, 1 = they do not, 2 = could not build/run
"""
import argparse, os, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

ERROR, DISCONNECTED, CONNECTING, CONNECTED = -1, 0, 1, 2
NAME = {ERROR: "ERROR", DISCONNECTED: "DISCONNECTED",
        CONNECTING: "CONNECTING", CONNECTED: "CONNECTED"}

# STATE-MACHINE.md, "Transitions". Every move the document permits.
LEGAL = {
    (DISCONNECTED, CONNECTING),    # Connect()
    (DISCONNECTED, ERROR),         # Connect() that cannot even build a token
    (CONNECTING,   CONNECTED),     # handshake completes
    (CONNECTING,   ERROR),         # handshake fails
    (CONNECTING,   DISCONNECTED),  # Disconnect() during handshake
    (CONNECTED,    DISCONNECTED),  # clean close, either side
    (CONNECTED,    ERROR),         # timeout / transport failure
    (CONNECTED,    CONNECTING),    # Connect() again while connected
    (ERROR,        CONNECTING),    # Connect() starts a fresh attempt
    (ERROR,        DISCONNECTED),  # Disconnect() clears the error
}


def build_and_run(cxx, build_dir):
    src = os.path.join(ROOT, "tools", "conformance", "drive_state_machine.cpp")
    libs = ["-lyojimbo", "-lnetcode", "-lreliable", "-lsodium-builtin", "-ltlsf"]
    with tempfile.TemporaryDirectory() as tmp:
        exe = os.path.join(tmp, "drive")
        cmd = [cxx, "-std=c++11", "-I" + os.path.join(ROOT, "include"), "-I" + ROOT,
               "-I" + os.path.join(ROOT, "..", "serialize"),
               "-I" + os.path.join(ROOT, "..", "netcode"),
               "-I" + os.path.join(ROOT, "..", "reliable"),
               "-I" + os.path.join(ROOT, "tlsf"),
               "-o", exe, src, "-L" + build_dir] + libs
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode != 0:
            print("build failed — is the library built in "
                  f"{build_dir}?\n{r.stderr[:1500]}", file=sys.stderr)
            sys.exit(2)
        r = subprocess.run([exe], capture_output=True, text=True, timeout=120)
        if r.returncode != 0:
            print("driver failed:\n" + (r.stdout + r.stderr)[:1500], file=sys.stderr)
            sys.exit(2)
        return r.stdout


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
    ap.add_argument("--build", default="/tmp/ybuild",
                    help="directory containing libyojimbo.a and friends")
    a = ap.parse_args()

    transitions, result = [], None
    for line in build_and_run(a.cxx, a.build).splitlines():
        f = line.split()
        if f[0] == "STATE":
            transitions.append((int(f[1]), int(f[2]), f[3]))
        elif f[0] == "RESULT":
            result = f[1:]

    checks, fails = 0, []
    def eq(name, got, exp):
        nonlocal checks; checks += 1
        if got != exp: fails.append(f"{name}: got {got!r}, STATE-MACHINE.md says {exp!r}")

    eq("driver completed the full lifecycle", result and result[0], "ok")

    # 1. every observed transition must be one the document permits
    for frm, to, phase in transitions:
        if frm == to: continue
        checks += 1
        if (frm, to) not in LEGAL:
            fails.append(f"ILLEGAL transition {NAME.get(frm,frm)} -> {NAME.get(to,to)} "
                         f"during '{phase}' is not in STATE-MACHINE.md")

    moves = [(f, t) for f, t, _ in transitions if f != t]

    # 2. the documented happy path, in order
    eq("happy path is DISCONNECTED->CONNECTING->CONNECTED->DISCONNECTED", moves,
       [(DISCONNECTED, CONNECTING), (CONNECTING, CONNECTED), (CONNECTED, DISCONNECTED)])

    # 3. "no transition into CONNECTED that does not pass through CONNECTING"
    for i, (frm, to) in enumerate(moves):
        if to == CONNECTED:
            checks += 1
            if frm != CONNECTING:
                fails.append(f"entered CONNECTED from {NAME.get(frm,frm)}, "
                             "but the document says only CONNECTING leads there")

    # 4. no spurious churn: the steady phase must produce no transitions at all
    eq("no state change while connected and idle",
       [p for f, t, p in transitions if f != t and p == "steady"], [])

    # 5. a clean disconnect lands in DISCONNECTED, never ERROR
    eq("clean disconnect ends in DISCONNECTED, not ERROR", moves[-1][1], DISCONNECTED)

    print(f"{checks} checks against STATE-MACHINE.md, {len(fails)} failures")
    print("  observed: " + " -> ".join(
        [NAME.get(moves[0][0], "?")] + [NAME.get(t, "?") for _, t in moves]))
    for f in fails: print("  FAIL " + f)
    if fails:
        print("\nSTATE-MACHINE.md and the implementation disagree. One of them is wrong.")
        return 1
    print("\nSTATE-MACHINE.md matches the implementation.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
