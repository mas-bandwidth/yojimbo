# yojimbo 1.0 — connection packet format

This document specifies the **wire format of yojimbo's connection packet**: the
payload that carries messages between client and server, precisely enough to
write an independent implementation that interoperates byte-for-byte.

## Scope, and what this document is not

yojimbo is three layers, and only the middle one is specified here.

| layer | responsibility | specified where |
|---|---|---|
| **netcode** | connection, encryption, connect tokens, transport | `netcode/STANDARD.md` |
| **yojimbo connection** | channels, messages, reliability, block transfer | **this document** |
| **serialize** | bit packing of every primitive below | `serialize/STANDARD.md` |

Everything here is written using serialize's primitives — `serialize_int`,
`serialize_bool`, `serialize_bits`, `serialize_bytes`,
`serialize_int_relative`. Their bit-level encodings are **not** repeated here;
read serialize's standard for those. When this document says
`serialize_int( value, 0, 63 )` it means exactly what that document says it
means, including that the width is `bits_required(0,63)` = 6 bits.

A connection packet is carried as the payload of a netcode *connection payload
packet*. It is encrypted and authenticated by netcode. Nothing in this document
concerns confidentiality or integrity — by the time you are parsing this
format, netcode has already established both.

## Configuration is part of the format

This format is **not self-describing**, and it is even less self-describing
than serialize's. Reader and writer must agree in advance on:

* `numChannels`, and each channel's `type`
* per channel: `maxMessagesPerPacket`, `maxBlockSize`, `blockFragmentSize`,
  `disableBlocks`
* the **number of message types** registered in the message factory

Every one of these changes the number of bits on the wire. A client and server
built against different channel configurations, or different message-type
counts, will not interoperate — they will desynchronize mid-packet and the
failure will look like corruption rather than misconfiguration.

This is a deliberate trade: nothing is spent transmitting what both ends
already know.

## Connection Packet

    serialize_int( numChannelEntries, 0, numChannels )
    for each of numChannelEntries:
        <channel packet data>

A packet with `numChannelEntries == 0` is legal and carries no messages. It is
still useful: netcode's own header carries the acknowledgements, so an empty
connection packet is how a peer acks without having anything to say.

Channel entries appear in ascending channel index order.

## Channel Packet Data

    if numChannels > 1:
        serialize_int( channelIndex, 0, numChannels - 1 )
    else:
        channelIndex = 0            // nothing on the wire

    serialize_bool( blockMessage )

    if blockMessage == false:
        <messages, per the channel's type>
    else:
        <block fragment>

Note the elision: **with a single channel configured, the channel index is not
transmitted at all.** Zero bits, not a zero value.

`blockMessage` selects between the two things a channel entry can carry:
ordinary messages, or one fragment of a large block transfer.

If `blockMessage` is true and the channel has `disableBlocks` set, the read
**fails**.

## Messages — reliable-ordered channels

    serialize_bool( hasMessages )
    if !hasMessages: done

    serialize_int( numMessages, 1, maxMessagesPerPacket )

    // message ids
    serialize_bits( messageId[0], 16 )
    for i in 1 .. numMessages-1:
        serialize_sequence_relative( messageId[i-1], messageId[i] )

    // message types and bodies
    for i in 0 .. numMessages-1:
        if maxMessageType > 0:
            serialize_int( messageType[i], 0, maxMessageType )
        <message body — application defined>

Where `maxMessageType = (number of registered message types) - 1`.

Three elisions worth naming, because each is a place an implementation goes
subtly wrong:

* `hasMessages` is a single bit, and when false **nothing else follows**.
* The **first** message id is a full 16 bits; every subsequent id is encoded
  **relative to its predecessor**. Message ids in a reliable-ordered channel
  are near-consecutive, so this usually costs one bit each.
* If the factory registers exactly **one** message type, `maxMessageType` is 0
  and the type field is **omitted entirely** — again zero bits, not a zero
  value.

All ids come first, then all types and bodies. They are **not** interleaved
per message. An implementation that writes `(id, type, body)` triples will
produce a stream this library cannot read.

### serialize_sequence_relative

Encodes a 16-bit sequence number relative to a previous one, handling wrap:

    writing:  a = previous
              b = current + ( previous > current ? 65536 : 0 )
              serialize_int_relative( a, b )

    reading:  serialize_int_relative( a, b )
              if b >= 65536: b -= 65536
              current = uint16( b )

The wrap is handled by lifting the smaller value into a second 65536-wide
window before taking the difference, so the difference is always positive and
usually tiny. The underlying `serialize_int_relative` ladder is specified in
serialize's standard; a difference of 1 costs a single bit.

## Messages — unreliable-unordered channels

    serialize_bool( hasMessages )
    if !hasMessages: done

    serialize_int( numMessages, 1, maxMessagesPerPacket )

    for i in 0 .. numMessages-1:
        if maxMessageType > 0:
            serialize_int( messageType[i], 0, maxMessageType )
        <message body — application defined>
        if this message type is a block message:
            <message block>

**No message ids are transmitted.** That is the entire difference in framing
between the two channel types: an unreliable channel has nothing to reorder or
retransmit, so ids would be dead weight.

Unlike a reliable channel, type and body **are** interleaved per message here.

### Message block

Attached to an individual unreliable message whose type is a block message:

    serialize_int( blockSize, 1, maxBlockSize )
    serialize_bytes( blockData, blockSize )

Remember that `serialize_bytes` **aligns to a byte boundary first** — see
serialize's standard. That alignment is part of this format.

## Block Fragments

Used when `blockMessage` is true. Large blocks on a reliable-ordered channel
are split across packets, one fragment per packet per channel.

    serialize_bits( messageId, 16 )

    if maxFragmentsPerBlock > 1:
        serialize_int( numFragments, 1, maxFragmentsPerBlock )
    else:
        numFragments = 1            // nothing on the wire

    if numFragments > 1:
        serialize_int( fragmentId, 0, numFragments - 1 )
    else:
        fragmentId = 0              // nothing on the wire

    serialize_int( fragmentSize, 1, blockFragmentSize )
    serialize_bytes( fragmentData, fragmentSize )

    if fragmentId == 0:
        if maxMessageType > 0:
            serialize_int( messageType, 0, maxMessageType )
        <the block message's own body — application defined>

`maxFragmentsPerBlock` is derived from the channel config as
`maxBlockSize / blockFragmentSize`.

**Only fragment 0 carries the message type and the message body.** Later
fragments carry raw payload only. This mirrors reliable's design, where only
fragment 0 carries the packet header — a receiver learns what a block *is*
from its first fragment, and everything after is bytes.

If the message named by fragment 0 is not a block message, the read **fails**.

## Receiver Obligations

* Reject `numChannelEntries > numChannels`.
* Reject a `channelIndex` outside `[0, numChannels-1]`.
* Reject `numMessages` outside `[1, maxMessagesPerPacket]`.
* Reject a block fragment on a channel configured with `disableBlocks`.
* Reject `fragmentId >= numFragments`, or `numFragments` above
  `maxFragmentsPerBlock`.
* Reject a fragment-0 message type that is not a block message.
* Fail the packet — do not partially apply it — if any message body fails to
  deserialize.

These are not advisory. Every one of them is a place where a malformed or
hostile packet would otherwise be accepted, and yojimbo's parser runs on
attacker-influenced bytes by construction.

## What This Format Does Not Do

* **No encryption or authentication.** netcode has already provided both by the
  time these bytes exist. Do not layer this format over an unprotected
  transport and expect yojimbo's guarantees.
* **No acknowledgement.** Acks live in netcode's header, not here.
* **No message-type registry on the wire.** Types are indices into a factory
  both ends compiled in. There is no negotiation and no version field.
* **No ordering across channels.** Ordering is per reliable-ordered channel
  only.

## Provenance

Written 2026-07-21 by Rowan, by reading the reference implementation
(`source/yojimbo_connection.cpp`, `source/yojimbo_channel.cpp`,
`include/yojimbo_message.h`, `include/yojimbo_serialize.h`).

It documents the format as it stands; where this document and the
implementation disagree, the implementation is authoritative and this document
is a bug.

Companion standards: `netcode/STANDARD.md` for the transport beneath,
`serialize/STANDARD.md` for the bit-level encoding of every primitive used
here.
