# yojimbo client state machine

`STANDARD.md` specifies the bytes on the wire. This specifies **behaviour over
time**: the states a yojimbo client passes through, what moves it between them,
and what a caller may assume in each.

The two documents are deliberately separate. A conformance checker for a wire
format decodes bytes; a conformance checker for a state machine has to drive a
real client and watch it move. Different instrument, different failure modes.

## The four states

    CLIENT_STATE_ERROR         = -1
    CLIENT_STATE_DISCONNECTED  =  0
    CLIENT_STATE_CONNECTING
    CLIENT_STATE_CONNECTED

The numbering carries meaning and callers rely on it: **negative is failure,
zero is idle, positive is progress.** `IsConnecting()`, `IsConnected()`,
`IsDisconnected()` and `ConnectionFailed()` are thin predicates over this value.

## Transitions

    DISCONNECTED ──Connect()──> CONNECTING ──netcode handshake──> CONNECTED
         ^                          │                                │
         │                          │ handshake failure              │ netcode failure
         │                          v                                v
         │                        ERROR <────────────────────────────┘
         │                          │
         │  Disconnect()            │
         └──────────────────────────┘

    ERROR ──Connect()──> CONNECTING (a fresh attempt)

    CONNECTED ──Disconnect(), clean server close, or connection-level error──> DISCONNECTED

    DISCONNECTED ──ConnectLoopback()──> CONNECTED

* **`Connect()`** moves DISCONNECTED (or ERROR, or CONNECTED) to CONNECTING. If
  `InsecureConnect` cannot generate the connect token, or the client cannot
  allocate its state, the client goes straight to ERROR without ever reaching
  CONNECTING. If netcode refuses the connect token immediately, or the socket
  cannot be created, the client ends in DISCONNECTED instead.
* **CONNECTING → CONNECTED** happens when the underlying netcode client
  completes its handshake.
* **`ConnectLoopback()`** is the exception to the handshake path: it connects a
  loopback client directly to a server slot and moves DISCONNECTED (or ERROR,
  or CONNECTED) straight to CONNECTED, without passing through CONNECTING. It
  tears any previous connection down first.
* **CONNECTING → ERROR** on any handshake failure: token expired, token
  invalid, connection denied, or any of the three timeouts.
* **CONNECTED → DISCONNECTED** when the server closes the connection cleanly,
  when the caller calls `Disconnect()`, or when a connection-level error is
  detected (a channel error such as desync or a failed serialize, a read
  error, or an allocator/message-factory error). The disconnect reason records
  which one.
* **CONNECTED → ERROR** when netcode reports a failure state while connected,
  such as a connection timeout. A connection-level error takes the
  DISCONNECTED edge above instead, because `BaseClient::AdvanceTime` disconnects
  before netcode is updated again.
* **`Disconnect()`** from any state ends at DISCONNECTED.

Apart from `ConnectLoopback()`, there is no transition into CONNECTED that does
not pass through CONNECTING.

## Derivation from netcode

yojimbo does not run its own handshake. Its four states are a **projection of
netcode's ten**, computed on every `AdvanceTime`:

| netcode client state | value | yojimbo state |
|---|--:|---|
| `CONNECT_TOKEN_EXPIRED` | −6 | **ERROR** |
| `INVALID_CONNECT_TOKEN` | −5 | **ERROR** |
| `CONNECTION_TIMED_OUT` | −4 | **ERROR** |
| `CONNECTION_RESPONSE_TIMED_OUT` | −3 | **ERROR** |
| `CONNECTION_REQUEST_TIMED_OUT` | −2 | **ERROR** |
| `CONNECTION_DENIED` | −1 | **ERROR** |
| `DISCONNECTED` | 0 | **DISCONNECTED** |
| `SENDING_CONNECTION_REQUEST` | 1 | **CONNECTING** |
| `SENDING_CONNECTION_RESPONSE` | 2 | **CONNECTING** |
| `CONNECTED` | 3 | **CONNECTED** |

The rule is simply: **any negative netcode state is ERROR**, zero is
DISCONNECTED, states 1 and 2 are CONNECTING, and 3 is CONNECTED. New negative
netcode states would map to ERROR automatically without a yojimbo change, which
is why the comparison is `< 0` rather than an enumeration.

That projection is only consulted while a netcode client exists. A
connection-level error detected first (a channel, read, allocator or
message-factory failure) makes `AdvanceTime` call `Disconnect()`, which tears
the netcode client down and leaves the client in DISCONNECTED with the reason
recorded, so the netcode state is never projected.

The *reason* for a failure is not in the state — it is recorded separately as a
disconnect reason, derived from which negative netcode state was seen. The
state tells you that it failed; the reason tells you why.

## What a caller may assume

* **Only in CONNECTED** may messages be sent or received. Sending in any other
  state is a programming error, not a queued operation.
* **`AdvanceTime` is what moves the machine.** A client that is never updated
  never changes state, never times out, and never completes a handshake. There
  is no background thread.
* **ERROR does not retry by itself.** `Connect()` starts a fresh attempt;
  `Disconnect()` returns it to DISCONNECTED. Every ERROR (out of memory at
  connect, a connect token that could not be generated, or a failure netcode
  reported) tears the netcode client down first, so `AdvanceTime` has nothing
  to update and the client stays in ERROR until `Connect()` or `Disconnect()`.
* **A clean server-side close and a connection-level error both land in
  DISCONNECTED, not ERROR.** ERROR is reserved for failures that stop the
  connect attempt before netcode can carry it (allocation or token generation)
  and for failures netcode reports (a failed handshake, a connection timeout).
  Use the disconnect reason to tell "the server said goodbye" from "the
  connection broke" — the state alone does not.

## Server side

The server has no equivalent state enum. It is either running or not
(`IsRunning()`, set by `Start()` and cleared by `Stop()`), and it tracks each
client slot independently. A client slot is connected or free; there is no
intermediate per-slot state visible to the caller, because the handshake is
netcode's and completes before the slot is marked connected.

## Provenance

Written 2026-07-21 by Rowan, from `source/yojimbo_client.cpp` and
`include/yojimbo_client_interface.h`, and verified behaviourally: a real client
and server are driven through connect, message exchange and disconnect, every
state transition is recorded, and the observed sequence is checked against the
machine above — including that no illegal transition occurs. See
`tools/conformance/verify_state_machine.py`.

Where this document and the implementation disagree, the implementation is
authoritative and this document is a bug.
