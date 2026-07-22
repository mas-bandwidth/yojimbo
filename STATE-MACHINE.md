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

    DISCONNECTED ──Connect()──> CONNECTING ──netcode connects──> CONNECTED
         ^                          │                                │
         │                          │ failure                        │ Disconnect()
         │                          v                                │ or server closes
         └──────────────────────  ERROR <───────────failure──────────┘
                Connect()             │
                                      └── Connect() starts a fresh attempt

* **`Connect()`** moves DISCONNECTED (or ERROR, or CONNECTED) to CONNECTING. If
  the connect token cannot be generated the client goes straight to ERROR
  without ever reaching CONNECTING.
* **CONNECTING → CONNECTED** happens when the underlying netcode client
  completes its handshake.
* **CONNECTING → ERROR** on any handshake failure: token expired, token
  invalid, connection denied, or any of the three timeouts.
* **CONNECTED → DISCONNECTED** when the server closes the connection cleanly.
* **CONNECTED → ERROR** on timeout or transport failure.
* **`Disconnect()`** from any state ends at DISCONNECTED.

There is no transition into CONNECTED that does not pass through CONNECTING,
and no path from DISCONNECTED to CONNECTED that skips it.

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

The *reason* for a failure is not in the state — it is recorded separately as a
disconnect reason, derived from which negative netcode state was seen. The
state tells you that it failed; the reason tells you why.

## What a caller may assume

* **Only in CONNECTED** may messages be sent or received. Sending in any other
  state is a programming error, not a queued operation.
* **`AdvanceTime` is what moves the machine.** A client that is never updated
  never changes state, never times out, and never completes a handshake. There
  is no background thread.
* **ERROR is terminal until acted on.** The client does not retry by itself.
  `Connect()` starts a fresh attempt; `Disconnect()` returns it to
  DISCONNECTED.
* **A clean server-side close lands in DISCONNECTED, not ERROR.** Distinguish
  "the server said goodbye" from "something broke" — they are different states
  on purpose.

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
