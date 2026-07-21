#!/usr/bin/env python3
"""Check yojimbo/STANDARD.md against the implementation.

Fully decodes the committed fuzz corpus — framing AND message bodies — using
ONLY what STANDARD.md says (plus serialize/STANDARD.md for the bit-level
primitives). Nothing here consults yojimbo's source. Every packet must decode
to exhaustion: leftover or missing bits mean the document is wrong.

The corpus was produced by the real library (tools/gen_seed_corpus_connection),
so agreement is a genuine differential result.

usage: python3 tools/conformance/verify_standard.py
exit:  0 = the document matches the implementation, 1 = it does not
"""
import glob, math, os, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CORPUS = os.path.join(ROOT, "fuzz", "corpus", "fuzz_connection")

# fuzz_config.h selector 0, and the ChannelConfig defaults it leaves alone
NUM_CHANNELS = 2
CHANNEL_TYPE = ["RELIABLE_ORDERED", "UNRELIABLE_UNORDERED"]
MAX_MESSAGES_PER_PACKET = 256
MAX_BLOCK_SIZE = 256 * 1024
BLOCK_FRAGMENT_SIZE = 1024
MAX_FRAGMENTS_PER_BLOCK = MAX_BLOCK_SIZE // BLOCK_FRAGMENT_SIZE
# fuzz_messages.h: PRIMITIVES, STRING, BYTES, BLOCK
NUM_MESSAGE_TYPES = 4
MAX_MESSAGE_TYPE = NUM_MESSAGE_TYPES - 1
FUZZ_MAX_STRING, FUZZ_MAX_BYTES = 64, 512


class R:
    """serialize/STANDARD.md: LSB-first bit packing."""
    def __init__(s, b): s.b = b; s.i = 0; s.n = len(b) * 8
    def bits(s, k):
        if s.i + k > s.n: raise ValueError("read past end of packet")
        v = 0
        for j in range(k):
            v |= ((s.b[s.i // 8] >> (s.i % 8)) & 1) << j; s.i += 1
        return v
    def align(s):
        pad = (8 - (s.i % 8)) % 8
        if pad and s.bits(pad) != 0: raise ValueError("align padding not zero")
    def read_bytes(s, n):
        s.align()
        out = s.b[s.i // 8: s.i // 8 + n]
        if len(out) != n: raise ValueError("short byte read")
        s.i += n * 8
        return out


def br(lo, hi): return 0 if lo == hi else (hi - lo).bit_length()
def sint(r, lo, hi):
    n = br(lo, hi)
    v = (r.bits(n) if n else 0) + lo
    if not (lo <= v <= hi): raise ValueError(f"value {v} outside [{lo},{hi}]")
    return v

def relative(r, prev):
    if r.bits(1): return prev + 1
    if r.bits(1): return prev + sint(r, 2, 6)
    if r.bits(1): return prev + sint(r, 7, 23)
    if r.bits(1): return prev + sint(r, 24, 280)
    if r.bits(1): return prev + sint(r, 281, 4377)
    return prev + r.bits(32)

def sequence_relative(r, prev):
    """STANDARD.md, 'serialize_sequence_relative'."""
    b = relative(r, prev)
    if b >= 65536: b -= 65536
    return b & 0xFFFF


def message_body(r, mtype):
    """fuzz_messages.h bodies, expressed purely in serialize primitives."""
    if mtype == 0:      # PRIMITIVES
        sint(r, -100000, 100000); r.bits(17); r.bits(1)
        r.bits(32); r.bits(32); r.bits(32)               # float, double (two words)
        mx = math.ceil(1.0 / 0.01); r.bits(br(0, mx))    # compressed_float
        r.align()
        base = sint(r, 0, 1000); relative(r, base)
    elif mtype == 1:    # STRING
        n = sint(r, 0, FUZZ_MAX_STRING - 1); r.read_bytes(n)
    elif mtype == 2:    # BYTES
        n = sint(r, 0, FUZZ_MAX_BYTES); r.read_bytes(n)
    elif mtype == 3:    # BLOCK
        r.bits(16)
    else:
        raise ValueError(f"unknown message type {mtype}")


def decode_packet(raw):
    """STANDARD.md, 'Connection Packet'. Returns a description; raises on mismatch."""
    r = R(raw)
    desc = []
    n_entries = sint(r, 0, NUM_CHANNELS)
    for _ in range(n_entries):
        ci = sint(r, 0, NUM_CHANNELS - 1) if NUM_CHANNELS > 1 else 0
        is_block = r.bits(1)
        if is_block:
            msg_id = r.bits(16)
            n_frag = sint(r, 1, MAX_FRAGMENTS_PER_BLOCK) if MAX_FRAGMENTS_PER_BLOCK > 1 else 1
            frag_id = sint(r, 0, n_frag - 1) if n_frag > 1 else 0
            frag_size = sint(r, 1, BLOCK_FRAGMENT_SIZE)
            r.read_bytes(frag_size)
            if frag_id == 0:
                mtype = sint(r, 0, MAX_MESSAGE_TYPE) if MAX_MESSAGE_TYPE > 0 else 0
                if mtype != 3: raise ValueError("fragment 0 message type is not a block message")
                message_body(r, mtype)
            desc.append(f"ch{ci} block id={msg_id} frag={frag_id}/{n_frag} size={frag_size}")
            continue
        if not r.bits(1):
            desc.append(f"ch{ci} empty"); continue
        n_msg = sint(r, 1, MAX_MESSAGES_PER_PACKET)
        if CHANNEL_TYPE[ci] == "RELIABLE_ORDERED":
            ids = [r.bits(16)]
            for _ in range(n_msg - 1):
                ids.append(sequence_relative(r, ids[-1]))
            types = []
            for _ in range(n_msg):
                types.append(sint(r, 0, MAX_MESSAGE_TYPE) if MAX_MESSAGE_TYPE > 0 else 0)
                message_body(r, types[-1])
            desc.append(f"ch{ci} reliable n={n_msg} ids={ids[0]}..{ids[-1]} types={types}")
        else:
            types = []
            for _ in range(n_msg):
                t = sint(r, 0, MAX_MESSAGE_TYPE) if MAX_MESSAGE_TYPE > 0 else 0
                types.append(t)
                message_body(r, t)
                if t == 3:                                   # block message attached
                    sz = sint(r, 1, MAX_BLOCK_SIZE); r.read_bytes(sz)
            desc.append(f"ch{ci} unreliable n={n_msg} types={types}")
    return desc, r.i, len(raw) * 8


def main():
    files = sorted(glob.glob(os.path.join(CORPUS, "*")))
    if not files:
        print(f"no corpus at {CORPUS}", file=sys.stderr); return 2
    n, fails = 0, []
    for path in files:
        d = open(path, "rb").read()
        off = 1                                   # byte 0 selects the config
        idx = 0
        while off + 2 <= len(d):
            ln = d[off] | (d[off + 1] << 8); off += 2
            if ln == 0 or off + ln > len(d): break
            raw = d[off:off + ln]; off += ln
            n += 1
            try:
                desc, used, total = decode_packet(raw)
                # the packet must decode to exhaustion: only flush padding may remain
                if total - used >= 8:
                    fails.append(f"{os.path.basename(path)} pkt{idx}: "
                                 f"{total-used} bits left over after decoding ({'; '.join(desc)})")
            except Exception as e:
                fails.append(f"{os.path.basename(path)} pkt{idx}: {e}")
            idx += 1
    print(f"{n} corpus packets fully decoded against STANDARD.md, {len(fails)} failures")
    for f in fails[:15]: print("  FAIL " + f)
    if fails:
        print("\nSTANDARD.md and the implementation disagree. One of them is wrong.")
        return 1
    print("\nSTANDARD.md matches the implementation.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
