# Conformance: STANDARD.md vs the implementation

`STANDARD.md` specifies yojimbo's connection packet format. This fully decodes
the committed fuzz corpus — framing **and** message bodies — using only what
that document says, plus `serialize/STANDARD.md` for the bit-level primitives.
Nothing here consults yojimbo's source.

    python3 tools/conformance/verify_standard.py

No compiler needed. The corpus in `fuzz/corpus/fuzz_connection` was produced by
the real library via `tools/gen_seed_corpus_connection`, so agreement is a
genuine differential result rather than a self-consistency check.

## The load-bearing assertion

**Every packet must decode to exhaustion.** Bits left over, or a read past the
end, means the document is wrong somewhere. This is much stronger than checking
that fields look plausible: a wrong elision rule produces field values that
still look reasonable while the bit cursor drifts, and only the total tells you.

## What is covered

Connection packet framing; channel entries and the block discriminator;
reliable-ordered framing (16-bit first id, relative ids after, all ids before
any types); unreliable-unordered framing (no ids, type and body interleaved);
block fragments including that only fragment 0 carries the type and body; and
every message body in `fuzz/fuzz_messages.h`, which between them exercise
ranged ints, odd bit widths, bool, float, double, compressed float, alignment,
the relative-integer ladder, strings, and byte blocks.

## What is NOT covered

Channel configurations other than fuzz selector 0, and the reliability
machinery above the wire (retransmission, reassembly state, ack logic). Those
are semantics rather than format.
