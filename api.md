# namespace `yojimbo` 

The library namespace.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`class `[`yojimbo::Address`](#classyojimbo_1_1_address)    | An IP address and port number.
`class `[`yojimbo::Allocator`](#classyojimbo_1_1_allocator)    | Functionality common to all allocators.
`class `[`yojimbo::BaseStream`](#classyojimbo_1_1_base_stream)    | Functionality common to all stream classes.
`class `[`yojimbo::BaseTransport`](#classyojimbo_1_1_base_transport)    | Common functionality shared between multiple transport implementations.
`class `[`yojimbo::BitArray`](#classyojimbo_1_1_bit_array)    | A simple bit array class.
`class `[`yojimbo::BitReader`](#classyojimbo_1_1_bit_reader)    | Reads bit packed integer values from a buffer.
`class `[`yojimbo::BitWriter`](#classyojimbo_1_1_bit_writer)    | Bitpacks unsigned integer values to a buffer.
`class `[`yojimbo::BlockMessage`](#classyojimbo_1_1_block_message)    | A message that can have a block of data attached to it.
`class `[`yojimbo::Channel`](#classyojimbo_1_1_channel)    | Common functionality shared across all channel types.
`class `[`yojimbo::ChannelListener`](#classyojimbo_1_1_channel_listener)    | Implement this interface to receive callbacks for channel events.
`class `[`yojimbo::Client`](#classyojimbo_1_1_client)    | A client that connects to a server.
`class `[`yojimbo::Connection`](#classyojimbo_1_1_connection)    | Implements packet level acks and transmits messages.
`class `[`yojimbo::ConnectionListener`](#classyojimbo_1_1_connection_listener)    | Implement this interface to get callbacks when connection events occur.
`class `[`yojimbo::DefaultAllocator`](#classyojimbo_1_1_default_allocator)    | [Allocator](#classyojimbo_1_1_allocator) implementation based on malloc and free.
`class `[`yojimbo::EncryptionManager`](#classyojimbo_1_1_encryption_manager)    | Associates addresses with encryption keys so each client gets their own keys for packet encryption.
`class `[`yojimbo::LocalTransport`](#classyojimbo_1_1_local_transport)    | Implements a local transport built on top of a network simulator.
`class `[`yojimbo::Matcher`](#classyojimbo_1_1_matcher)    | Communicates with the matcher web service over HTTPS.
`class `[`yojimbo::MeasureStream`](#classyojimbo_1_1_measure_stream)    | Stream class for estimating how many bits it would take to serialize something.
`class `[`yojimbo::Message`](#classyojimbo_1_1_message)    | A reference counted object that can be serialized to a bitstream.
`class `[`yojimbo::MessageFactory`](#classyojimbo_1_1_message_factory)    | Defines the set of message types that can be created.
`class `[`yojimbo::NetworkSimulator`](#classyojimbo_1_1_network_simulator)    | Simulates packet loss, latency, jitter and duplicate packets.
`class `[`yojimbo::NetworkTransport`](#classyojimbo_1_1_network_transport)    | Implements a network transport built on top of non-blocking sendto and recvfrom socket APIs.
`class `[`yojimbo::Packet`](#classyojimbo_1_1_packet)    | A packet that can be serialized to a bit stream.
`class `[`yojimbo::PacketFactory`](#classyojimbo_1_1_packet_factory)    | Defines the set of packet types and a function to create packets.
`class `[`yojimbo::PacketProcessor`](#classyojimbo_1_1_packet_processor)    | Adds packet encryption and decryption on top of low-level read and write packet functions.
`class `[`yojimbo::Queue`](#classyojimbo_1_1_queue)    | A simple templated queue.
`class `[`yojimbo::ReadStream`](#classyojimbo_1_1_read_stream)    | Stream class for reading bitpacked data.
`class `[`yojimbo::ReliableOrderedChannel`](#classyojimbo_1_1_reliable_ordered_channel)    | Messages sent across this channel are guaranteed to arrive in the order they were sent.
`class `[`yojimbo::ReplayProtection`](#classyojimbo_1_1_replay_protection)    | Provides protection against packets being sniffed and replayed.
`class `[`yojimbo::SequenceBuffer`](#classyojimbo_1_1_sequence_buffer)    | Data structure that stores data indexed by sequence number.
`class `[`yojimbo::Serializable`](#classyojimbo_1_1_serializable)    | Interface for an object that knows how to read, write and measure how many bits it would take up in a bit stream.
`class `[`yojimbo::Server`](#classyojimbo_1_1_server)    | A server with n slots for clients to connect to.
`class `[`yojimbo::Socket`](#classyojimbo_1_1_socket)    | Simple wrapper around a non-blocking UDP socket.
`class `[`yojimbo::TLSF_Allocator`](#classyojimbo_1_1_t_l_s_f___allocator)    | [Allocator](#classyojimbo_1_1_allocator) built on the TLSF allocator implementation by Matt Conte.
`class `[`yojimbo::Transport`](#classyojimbo_1_1_transport)    | Interface for sending and receiving packets.
`class `[`yojimbo::TransportContextManager`](#classyojimbo_1_1_transport_context_manager)    | Maps addresses to transport contexts so each client on the server can have its own set of resources.
`class `[`yojimbo::UnreliableUnorderedChannel`](#classyojimbo_1_1_unreliable_unordered_channel)    | Messages sent across this channel are not guaranteed to arrive, and may be received in a different order than they were sent.
`class `[`yojimbo::WriteStream`](#classyojimbo_1_1_write_stream)    | Stream class for writing bitpacked data.
`struct `[`yojimbo::BitsRequired`](#structyojimbo_1_1_bits_required)    | Calculates the number of bits required to serialize an integer value in [min,max] at compile time.
`struct `[`yojimbo::ChallengePacket`](#structyojimbo_1_1_challenge_packet)    | Sent from server to client in response to valid connection request packets, provided that the server is not full.
`struct `[`yojimbo::ChallengeResponsePacket`](#structyojimbo_1_1_challenge_response_packet)    | Sent from client to server in response to a challenge packet.
`struct `[`yojimbo::ChallengeToken`](#structyojimbo_1_1_challenge_token)    | Data stored inside the encrypted challenge token.
`struct `[`yojimbo::ChannelConfig`](#structyojimbo_1_1_channel_config)    | Configuration properties for a message channel.
`struct `[`yojimbo::ChannelPacketData`](#structyojimbo_1_1_channel_packet_data)    | Per-channel data inside a connection packet.
`struct `[`yojimbo::ClientServerConfig`](#structyojimbo_1_1_client_server_config)    | Configuration shared between client and server.
`struct `[`yojimbo::ConnectionConfig`](#structyojimbo_1_1_connection_config)    | Configures connection properties and the set of channels for sending and receiving messages.
`struct `[`yojimbo::ConnectionContext`](#structyojimbo_1_1_connection_context)    | Provides information required to read and write connection packets.
`struct `[`yojimbo::ConnectionDeniedPacket`](#structyojimbo_1_1_connection_denied_packet)    | Sent from server to client to deny a client connection.
`struct `[`yojimbo::ConnectionPacket`](#structyojimbo_1_1_connection_packet)    | Implements packet level acks and carries messages across a connection.
`struct `[`yojimbo::ConnectionReceivedPacketData`](#structyojimbo_1_1_connection_received_packet_data)    | 
`struct `[`yojimbo::ConnectionRequestPacket`](#structyojimbo_1_1_connection_request_packet)    | Sent from client to server when a client is first requesting a connection.
`struct `[`yojimbo::ConnectionSentPacketData`](#structyojimbo_1_1_connection_sent_packet_data)    | 
`struct `[`yojimbo::ConnectToken`](#structyojimbo_1_1_connect_token)    | Connect token passed from the client to server when it requests a secure connection.
`struct `[`yojimbo::ConnectTokenEntry`](#structyojimbo_1_1_connect_token_entry)    | Used by the server to remember and reject connect tokens that have already been used.
`struct `[`yojimbo::DisconnectPacket`](#structyojimbo_1_1_disconnect_packet)    | Sent between client and server after connection is established when either side disconnects cleanly.
`struct `[`yojimbo::InsecureConnectPacket`](#structyojimbo_1_1_insecure_connect_packet)    | Sent from client to server requesting an insecure connect.
`struct `[`yojimbo::KeepAlivePacket`](#structyojimbo_1_1_keep_alive_packet)    | Sent once client/server connection is established, but only if necessary to avoid time out.
`struct `[`yojimbo::Log2`](#structyojimbo_1_1_log2)    | Calculates the log 2 of an unsigned 32 bit integer at compile time.
`struct `[`yojimbo::MatcherInternal`](#structyojimbo_1_1_matcher_internal)    | 
`struct `[`yojimbo::MatchResponse`](#structyojimbo_1_1_match_response)    | Response sent back from the matcher web service when the client requests a match.
`struct `[`yojimbo::PacketReadWriteInfo`](#structyojimbo_1_1_packet_read_write_info)    | Information passed into low-level functions to read and write packets.
`struct `[`yojimbo::PopCount`](#structyojimbo_1_1_pop_count)    | Calculates the population count of an unsigned 32 bit integer at compile time.
`struct `[`yojimbo::ServerClientData`](#structyojimbo_1_1_server_client_data)    | Per-client data stored on the server.
`struct `[`yojimbo::TransportContext`](#structyojimbo_1_1_transport_context)    | Gives the transport access to resources it needs to read and write packets.
# class `yojimbo::Address` 


An IP address and port number.

Supports both IPv4 and IPv6 addresses.

Identifies where a packet came from, and where a packet should be sent.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public uint32_t ipv4` | IPv4 address data. Valid if type is ADDRESS_IPV4.
`public uint16_t ipv6` | IPv6 address data. Valid if type is ADDRESS_IPV6.
`public  Address()` | [Address](#classyojimbo_1_1_address) default constructor.
`public  Address(uint8_t a,uint8_t b,uint8_t c,uint8_t d,uint16_t port)` | Create an IPv4 address.
`public  explicit Address(uint32_t address,int16_t port)` | Create an IPv4 address.
`public  explicit Address(uint16_t a,uint16_t b,uint16_t c,uint16_t d,uint16_t e,uint16_t f,uint16_t g,uint16_t h,uint16_t port)` | Create an IPv6 address.
`public  explicit Address(const uint16_t address,uint16_t port)` | Create an IPv6 address.
`public  explicit Address(const sockaddr_storage * addr)` | Create an address from a sockaddr_storage.
`public  explicit Address(const char * address)` | Parse a string to an address.
`public  explicit Address(const char * address,uint16_t port)` | Parse a string to an address.
`public void Clear()` | Clear the address.
`public uint32_t GetAddress4() const` | Get the IPv4 address data.
`public const uint16_t * GetAddress6() const` | Get the IPv6 address data.
`public void SetPort(uint16_t port)` | Set the port.
`public uint16_t GetPort() const` | Get the port number.
`public `[`AddressType`](#namespaceyojimbo_1a4ff5bf2db808019c5e6cf4a10f8fa654)` GetType() const` | Get the address type.
`public const char * ToString(char buffer,int bufferSize) const` | Convert the address to a string.
`public bool IsValid() const` | True if the address is valid.
`public bool IsLoopback() const` | Is this a loopback address?
`public bool IsLinkLocal() const` | Is this an IPv6 link local address?
`public bool IsSiteLocal() const` | Is this an IPv6 site local address?
`public bool IsMulticast() const` | Is this an IPv6 multicast address?
`public bool IsGlobalUnicast() const` | Is this in IPv6 global unicast address?
`public bool operator==(const `[`Address`](#classyojimbo_1_1_address)` & other) const` | 
`public bool operator!=(const `[`Address`](#classyojimbo_1_1_address)` & other) const` | 
`protected void Parse(const char * address)` | Helper function to parse an address string.

## Members

#### `public uint32_t ipv4` 

IPv4 address data. Valid if type is ADDRESS_IPV4.



#### `public uint16_t ipv6` 

IPv6 address data. Valid if type is ADDRESS_IPV6.



#### `public  Address()` 

[Address](#classyojimbo_1_1_address) default constructor.

Designed for convenience so you can have address members of classes and initialize them via assignment.

An address created by the default constructor will have address type set to ADDRESS_NONE. [Address::IsValid](#classyojimbo_1_1_address_1a52f799d1784e0351fee3e067904323fa) will return false.


**See also**: [IsValid](#classyojimbo_1_1_address_1a52f799d1784e0351fee3e067904323fa)

#### `public  Address(uint8_t a,uint8_t b,uint8_t c,uint8_t d,uint16_t port)` 

Create an IPv4 address.

IMPORTANT: Pass in port in local byte order. The address class handles the conversion to network order for you.


#### Parameters
* `a` The first field of the IPv4 address. 


* `b` The second field of the IPv4 address. 


* `c` The third field of the IPv4 address. 


* `d` The fourth field of the IPv4 address. 


* `port` The IPv4 port (local byte order).

#### `public  explicit Address(uint32_t address,int16_t port)` 

Create an IPv4 address.

IMPORTANT: Pass in address and port in local byte order. The address class handles the conversion to network order for you.


#### Parameters
* `address` The IPv4 address (local byte order). 


* `port` The IPv4 port (local byte order).

#### `public  explicit Address(uint16_t a,uint16_t b,uint16_t c,uint16_t d,uint16_t e,uint16_t f,uint16_t g,uint16_t h,uint16_t port)` 

Create an IPv6 address.

IMPORTANT: Pass in address fields and the port in local byte order. The address class handles the conversion to network order for you.


#### Parameters
* `a` First field of the IPv6 address (local byte order). 


* `b` Second field of the IPv6 address (local byte order). 


* `c` Third field of the IPv6 address (local byte order). 


* `d` Fourth field of the IPv6 address (local byte order). 


* `e` Fifth field of the IPv6 address (local byte order). 


* `f` Sixth field of the IPv6 address (local byte order). 


* `g` Seventh field of the IPv6 address (local byte order). 


* `h` Eigth field of the IPv6 address (local byte order). 


* `port` The IPv6 port (local byte order).

#### `public  explicit Address(const uint16_t address,uint16_t port)` 

Create an IPv6 address.

IMPORTANT: Pass in address fields and the port in local byte order. The address class handles the conversion to network order for you.


#### Parameters
* `address` Array containing 8 16bit address fields for the IPv6 address (local byte order). 


* `port` The IPv6 port (local byte order).

#### `public  explicit Address(const sockaddr_storage * addr)` 

Create an address from a sockaddr_storage.

This is a convenience function for working with BSD socket APIs.

Depending on the information in sockaddr_storage ss_family, the address will become ADDRESS_TYPE_IPV4 or ADDRESS_TYPE_IPV6.

If something goes wrong in the conversion the address type is set to ADDRESS_TYPE_NONE and [Address::IsValid](#classyojimbo_1_1_address_1a52f799d1784e0351fee3e067904323fa) returns false.


#### Parameters
* `addr` The sockaddr_storage data to convert to an address.





**See also**: [Address::IsValid](#classyojimbo_1_1_address_1a52f799d1784e0351fee3e067904323fa)


**See also**: [Address::GetType](#classyojimbo_1_1_address_1a2014451fd65bdc783dbda321540d508f)

#### `public  explicit Address(const char * address)` 

Parse a string to an address.

This versions supports parsing a port included in the address string. For example, "127.0.0.1:4000" and "[::1]:40000".

Parsing is performed via inet_pton once the port # has been extracted from the string, so you may specify any IPv4 or IPv6 address formatted in any valid way, and it should work as you expect.

Depending on the type of data in the string the address will become ADDRESS_TYPE_IPV4 or ADDRESS_TYPE_IPV6.

If the string is not recognized as a valid address, the address type is set to ADDRESS_TYPE_NONE, causing [Address::IsValid](#classyojimbo_1_1_address_1a52f799d1784e0351fee3e067904323fa) to return false. Please check that after creating an address from a string.


#### Parameters
* `address` The string to parse to create the address.





**See also**: [Address::IsValid](#classyojimbo_1_1_address_1a52f799d1784e0351fee3e067904323fa)


**See also**: [Address::GetType](#classyojimbo_1_1_address_1a2014451fd65bdc783dbda321540d508f)

#### `public  explicit Address(const char * address,uint16_t port)` 

Parse a string to an address.

This versions overrides any port read in the address with the port parameter. This lets you parse "127.0.0.1" and "[::1]" and pass in the port you want programatically.

Parsing is performed via inet_pton once the port # has been extracted from the string, so you may specify any IPv4 or IPv6 address formatted in any valid way, and it should work as you expect.

Depending on the type of data in the string the address will become ADDRESS_TYPE_IPV4 or ADDRESS_TYPE_IPV6.

If the string is not recognized as a valid address, the address type is set to ADDRESS_TYPE_NONE, causing [Address::IsValid](#classyojimbo_1_1_address_1a52f799d1784e0351fee3e067904323fa) to return false. Please check that after creating an address from a string.


#### Parameters
* `address` The string to parse to create the address. 


* `port` Overrides the port number read from the string (if any).





**See also**: [Address::IsValid](#classyojimbo_1_1_address_1a52f799d1784e0351fee3e067904323fa)


**See also**: [Address::GetType](#classyojimbo_1_1_address_1a2014451fd65bdc783dbda321540d508f)

#### `public void Clear()` 

Clear the address.

The address type is set to ADDRESS_TYPE_NONE.

After this function is called [Address::IsValid](#classyojimbo_1_1_address_1a52f799d1784e0351fee3e067904323fa) will return false.

#### `public uint32_t GetAddress4() const` 

Get the IPv4 address data.

#### Returns
The IPv4 address (local byte order).

#### `public const uint16_t * GetAddress6() const` 

Get the IPv6 address data.

#### Returns
the IPv6 address data (local byte order).

#### `public void SetPort(uint16_t port)` 

Set the port.

This is useful when you want to programatically set a server port, eg. try to open a server on ports 40000, 40001, etc...


#### Parameters
* `port` The port number (local byte order). Works for both IPv4 and IPv6 addresses.

#### `public uint16_t GetPort() const` 

Get the port number.

#### Returns
The port number (local byte order).

#### `public `[`AddressType`](#namespaceyojimbo_1a4ff5bf2db808019c5e6cf4a10f8fa654)` GetType() const` 

Get the address type.

#### Returns
The address type: ADDRESS_NONE, ADDRESS_IPV4 or ADDRESS_IPV6.

#### `public const char * ToString(char buffer,int bufferSize) const` 

Convert the address to a string.

#### Parameters
* `buffer` The buffer the address will be written to. 


* `bufferSize` The size of the buffer in bytes. Must be at least MaxAddressLength.

#### `public bool IsValid() const` 

True if the address is valid.

A valid address is any address with a type other than ADDRESS_TYPE_NONE.


#### Returns
True if the address is valid, false otherwise.

#### `public bool IsLoopback() const` 

Is this a loopback address?

Corresponds to an IPv4 address of "127.0.0.1", or an IPv6 address of "::1".


#### Returns
True if this is the loopback address.

#### `public bool IsLinkLocal() const` 

Is this an IPv6 link local address?

Corresponds to the first field of the address being 0xfe80


#### Returns
True if this address is a link local IPv6 address.

#### `public bool IsSiteLocal() const` 

Is this an IPv6 site local address?

Corresponds to the first field of the address being 0xfec0


#### Returns
True if this address is a site local IPv6 address.

#### `public bool IsMulticast() const` 

Is this an IPv6 multicast address?

Corresponds to the first field of the IPv6 address being 0xff00


#### Returns
True if this address is a multicast IPv6 address.

#### `public bool IsGlobalUnicast() const` 

Is this in IPv6 global unicast address?

Corresponds to any IPv6 address that is not any of the following: 1. Link Local 
2. Site Local
3. Multicast
4. Loopback



#### Returns
True if this is a global unicast IPv6 address.

#### `public bool operator==(const `[`Address`](#classyojimbo_1_1_address)` & other) const` 





#### `public bool operator!=(const `[`Address`](#classyojimbo_1_1_address)` & other) const` 





#### `protected void Parse(const char * address)` 

Helper function to parse an address string.

Used by the constructors that take a string parameter.


#### Parameters
* `address` The string to parse.

# class `yojimbo::Allocator` 


Functionality common to all allocators.

Extend this class to hook up your own allocator to yojimbo.

IMPORTANT: This allocator is not yet threadsafe. Only call it from one thread!

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  Allocator()` | [Allocator](#classyojimbo_1_1_allocator) constructor.
`public virtual  ~Allocator()` | [Allocator](#classyojimbo_1_1_allocator) destructor.
`public void * Allocate(size_t size,const char * file,int line)` | Allocate a block of memory.
`public void Free(void * p,const char * file,int line)` | Free a block of memory.
`public inline `[`AllocatorError`](#namespaceyojimbo_1a3cd9a1fe14eb463b62a96996f85d1f04)` GetError() const` | Get the allocator error level.
`public inline void ClearError()` | Clear the allocator error level back to default.
`protected `[`AllocatorError`](#namespaceyojimbo_1a3cd9a1fe14eb463b62a96996f85d1f04)` m_error` | The allocator error level.
`protected inline void SetError(`[`AllocatorError`](#namespaceyojimbo_1a3cd9a1fe14eb463b62a96996f85d1f04)` error)` | Set the error level.
`protected void TrackAlloc(void * p,size_t size,const char * file,int line)` | Call this function to track an allocation made by your derived allocator class.
`protected void TrackFree(void * p,const char * file,int line)` | Call this function to track a free made by your derived allocator class.

## Members

#### `public  Allocator()` 

[Allocator](#classyojimbo_1_1_allocator) constructor.

Sets the error level to ALLOCATOR_ERROR_NONE.

#### `public virtual  ~Allocator()` 

[Allocator](#classyojimbo_1_1_allocator) destructor.

Make sure all allocations made from this allocator are freed before you destroy this allocator.

In debug build, validates this is true walks the map of allocator entries. Any outstanding entries are considered memory leaks and printed to stdout.

#### `public void * Allocate(size_t size,const char * file,int line)` 

Allocate a block of memory.

IMPORTANT: Don't call this directly. Use the YOJIMBO_NEW or YOJIMBO_ALLOCATE macros instead, because they automatically pass in the source filename and line number for you.


#### Parameters
* `size` The size of the block of memory to allocate (bytes). 


* `file` The source code filename that is performing the allocation. Used for tracking allocations and reporting on memory leaks. 


* `line` The line number in the source code file that is performing the allocation.





#### Returns
A block of memory of the requested size, or NULL if the allocation could not be performed. If NULL is returned, the error level is set to ALLOCATION_ERROR_FAILED_TO_ALLOCATE.


**See also**: [Allocator::Free](#classyojimbo_1_1_allocator_1ac819dbc9add941b7a491a6ab7fb5fe3a)


**See also**: [Allocator::GetError](#classyojimbo_1_1_allocator_1a6418ce2bf0cf777755b2d06cca6ff522)

#### `public void Free(void * p,const char * file,int line)` 

Free a block of memory.

IMPORTANT: Don't call this directly. Use the YOJIMBO_DELETE or YOJIMBO_FREE macros instead, because they automatically pass in the source filename and line number for you.


#### Parameters
* `p` Pointer to the block of memory to free. Must be non-NULL block of memory that was allocated with this allocator. Will assert otherwise. 


* `file` The source code filename that is performing the free. Used for tracking allocations and reporting on memory leaks. 


* `line` The line number in the source code file that is performing the free.





**See also**: [Allocator::Allocate](#classyojimbo_1_1_allocator_1ab33a56067c7b815481e3dd091b701f9c)


**See also**: [Allocator::GetError](#classyojimbo_1_1_allocator_1a6418ce2bf0cf777755b2d06cca6ff522)

#### `public inline `[`AllocatorError`](#namespaceyojimbo_1a3cd9a1fe14eb463b62a96996f85d1f04)` GetError() const` 

Get the allocator error level.

Use this function to check if an allocation has failed. This is used in the client/server to disconnect a client with a failed allocation.


#### Returns
The allocator error level.

#### `public inline void ClearError()` 

Clear the allocator error level back to default.



#### `protected `[`AllocatorError`](#namespaceyojimbo_1a3cd9a1fe14eb463b62a96996f85d1f04)` m_error` 

The allocator error level.



#### `protected inline void SetError(`[`AllocatorError`](#namespaceyojimbo_1a3cd9a1fe14eb463b62a96996f85d1f04)` error)` 

Set the error level.

For correct client/server behavior when an allocation fails, please make sure you call this method to set the error level to ALLOCATOR_ERROR_FAILED_TO_ALLOCATE.


#### Parameters
* `error` The allocator error level to set.

#### `protected void TrackAlloc(void * p,size_t size,const char * file,int line)` 

Call this function to track an allocation made by your derived allocator class.

In debug build, tracked allocations are automatically checked for leaks when the allocator is destroyed.


#### Parameters
* `p` Pointer to the memory that was allocated. 


* `size` The size of the allocation in bytes. 


* `file` The source code file that performed the allocation. 


* `line` The line number in the source file where the allocation was performed.

#### `protected void TrackFree(void * p,const char * file,int line)` 

Call this function to track a free made by your derived allocator class.

In debug build, any allocation tracked without a corresponding free is considered a memory leak when the allocator is destroyed.


#### Parameters
* `p` Pointer to the memory that was allocated. 


* `file` The source code file that is calling in to free the memory. 


* `line` The line number in the source file where the free is being called from.

# class `yojimbo::BaseStream` 


Functionality common to all stream classes.



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  explicit BaseStream(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator)` | Base stream constructor.
`public inline void SetContext(void * context)` | Set a context on the stream.
`public inline void * GetContext() const` | Get the context pointer set on the stream.
`public inline void SetUserContext(void * context)` | Set a user context on the stream.
`public inline void * GetUserContext() const` | Get the user context pointer set on the stream.
`public inline `[`Allocator`](#classyojimbo_1_1_allocator)` & GetAllocator()` | Get the allocator set on the stream.

## Members

#### `public inline  explicit BaseStream(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator)` 

Base stream constructor.

#### Parameters
* `allocator` The allocator to use for stream allocations. This lets you dynamically allocate memory as you read and write packets.

#### `public inline void SetContext(void * context)` 

Set a context on the stream.

Contexts are used by the library supply data that is needed to read and write packets.

Specifically, this context is used by the connection to supply data needed to read and write connection packets.

If you are using the yojimbo client/server or connection classes you should NOT set this manually. It's already taken!

However, if you are using only the low-level parts of yojimbo, feel free to take this over and use it for whatever you want.


**See also**: [ConnectionContext](#structyojimbo_1_1_connection_context)


**See also**: [ConnectionPacket](#structyojimbo_1_1_connection_packet)

#### `public inline void * GetContext() const` 

Get the context pointer set on the stream.

#### Returns
The context pointer. May be NULL.

#### `public inline void SetUserContext(void * context)` 

Set a user context on the stream.

This is designed for users of the library to be able to set their own context on the stream, without interfering with the context used for connection packets.


**See also**: [Client::SetUserContext](#classyojimbo_1_1_client_1a2422ec124327768e01f061b472872c1a)


**See also**: [Server::SetUserContext](#classyojimbo_1_1_server_1abf7e8affdffa835a872dd48788956f9e)

#### `public inline void * GetUserContext() const` 

Get the user context pointer set on the stream.

#### Returns
The user context pointer. May be NULL.

#### `public inline `[`Allocator`](#classyojimbo_1_1_allocator)` & GetAllocator()` 

Get the allocator set on the stream.

You can use this allocator to dynamically allocate memory while reading and writing packets.


#### Returns
The stream allocator.

# class `yojimbo::BaseTransport` 

```
class yojimbo::BaseTransport
  : public yojimbo::Transport
```  

Common functionality shared between multiple transport implementations.



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  BaseTransport(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,const `[`Address`](#classyojimbo_1_1_address)` & address,uint64_t protocolId,double time,int maxPacketSize,int sendQueueSize,int receiveQueueSize,bool allocateNetworkSimulator)` | Base transport constructor.
`public  ~BaseTransport()` | 
`public virtual void SetContext(const `[`TransportContext`](#structyojimbo_1_1_transport_context)` & context)` | Set the transport context.
`public virtual void ClearContext()` | Clear context.
`public virtual void Reset()` | Reset the transport.
`public virtual void SendPacket(const `[`Address`](#classyojimbo_1_1_address)` & address,`[`Packet`](#classyojimbo_1_1_packet)` * packet,uint64_t sequence,bool immediate)` | [Queue](#classyojimbo_1_1_queue) a packet to be sent.
`public virtual `[`Packet`](#classyojimbo_1_1_packet)` * ReceivePacket(`[`Address`](#classyojimbo_1_1_address)` & from,uint64_t * sequence)` | Receive a packet.
`public virtual void WritePackets()` | Iterates across all packets in the send queue and writes them to the network.
`public virtual void ReadPackets()` | Reads packets from the network and adds them to the packet receive queue.
`public virtual int GetMaxPacketSize() const` | Returns the maximum packet size supported by this transport.
`public virtual void SetNetworkConditions(float latency,float jitter,float packetLoss,float duplicate)` | Add simulated network conditions on top of this transport.
`public virtual void ClearNetworkConditions()` | Clear network conditions back to defaults.
`public virtual void EnablePacketEncryption()` | Turns on packet encryption and enables it for all packet types.
`public virtual void DisablePacketEncryption()` | Disables encryption for all packet types.
`public virtual void DisableEncryptionForPacketType(int type)` | Disables encryption for a specific packet type.
`public virtual bool IsEncryptedPacketType(int type) const` | Is a packet type encrypted?
`public virtual bool AddEncryptionMapping(const `[`Address`](#classyojimbo_1_1_address)` & address,const uint8_t * sendKey,const uint8_t * receiveKey,double timeout)` | Associates an address with keys for packet encryption.
`public virtual bool RemoveEncryptionMapping(const `[`Address`](#classyojimbo_1_1_address)` & address)` | Remove the encryption mapping for an address.
`public virtual int FindEncryptionMapping(const `[`Address`](#classyojimbo_1_1_address)` & address)` | Find the index of an encryption mapping by address.
`public virtual void ResetEncryptionMappings()` | Reset all encryption mappings.
`public virtual bool AddContextMapping(const `[`Address`](#classyojimbo_1_1_address)` & address,const `[`TransportContext`](#structyojimbo_1_1_transport_context)` & context)` | Add a transport context that is specific for an address.
`public virtual bool RemoveContextMapping(const `[`Address`](#classyojimbo_1_1_address)` & address)` | Remove a context mapping.
`public virtual void ResetContextMappings()` | Reset all context mappings.
`public virtual void AdvanceTime(double time)` | Advance transport time.
`public virtual double GetTime() const` | Gets the current transport time.
`public virtual uint64_t GetCounter(int index) const` | Get a counter value.
`public virtual void ResetCounters()` | Reset all counters to zero.
`public virtual void SetFlags(uint64_t flags)` | Set transport flags.
`public virtual uint64_t GetFlags() const` | Get the current transport flags.
`public virtual const `[`Address`](#classyojimbo_1_1_address)` & GetAddress() const` | Get the address of the transport.
`public virtual uint64_t GetProtocolId() const` | Get the protocol id for the transport.
`protected `[`TransportContext`](#structyojimbo_1_1_transport_context)` m_context` | The default transport context used if no context can be found for the specific address.
`protected `[`Address`](#classyojimbo_1_1_address)` m_address` | The address of the transport. This is the address to send packets to so that this transport would receive them.
`protected double m_time` | The current transport time. See [Transport::AdvanceTime](#classyojimbo_1_1_transport_1afbc173c15dc6a2d2eee3ac1f93d47d90).
`protected uint64_t m_flags` | The transport flags. See [Transport::SetFlags](#classyojimbo_1_1_transport_1aa7cd42ebe2b149a71ed05438c76d3417).
`protected uint64_t m_protocolId` | The protocol id. You set it when you create the transport and it's used to filter out any packets sent that have different protocols ids. Lets you implement basic versioning (eg. ignore packets sent by other versions of your protocol on the network).
`protected `[`Allocator`](#classyojimbo_1_1_allocator)` * m_allocator` | The allocator passed in to the transport constructor. Used for all allocations inside the transport class, except for packets, messages and stream allocations, which are determined by the factories and allocators passed in to the context.
`protected `[`PacketProcessor`](#classyojimbo_1_1_packet_processor)` * m_packetProcessor` | The packet processor. This is the engine for reading and writing packets, as well as packet encryption and decryption.
`protected `[`Queue`](#classyojimbo_1_1_queue)`< PacketEntry > m_sendQueue` | The packet send queue. Packets sent via [Transport::SendPacket](#classyojimbo_1_1_transport_1aa9c2dec77b36ddd19296c35533173b5f) are added to this queue (unless the immediate flag is true). This queue is flushed to the network each time [Transport::WritePackets](#classyojimbo_1_1_transport_1a903705df34df07acd44eccf3c6ccacff) is called.
`protected `[`Queue`](#classyojimbo_1_1_queue)`< PacketEntry > m_receiveQueue` | The packet receive queue. As packets are read from the network in [Transport::ReadPackets](#classyojimbo_1_1_transport_1abbd058ddc3242bcfced01954c3d78dfa) they are added to this queue. Packets are popped off the receive queue by Transport::ReadPacket.
`protected uint8_t * m_allPacketTypes` | An array with each entry for valid packet types from the [PacketFactory](#classyojimbo_1_1_packet_factory) set to 1.
`protected uint8_t * m_packetTypeIsEncrypted` | An array with each entry set to 1 if that packet type is encrypted. See [Transport::EnablePacketEncryption](#classyojimbo_1_1_transport_1ac3c1c9cd04e06ce14af28c2097d960e8).
`protected uint8_t * m_packetTypeIsUnencrypted` | An array with each entry set to 1 if that packet type is NOT encrypted.
`protected `[`EncryptionManager`](#classyojimbo_1_1_encryption_manager)` * m_encryptionManager` | The encryption manager. Manages encryption contexts and lets the transport look up send and receive keys for encrypting and decrypting packets.
`protected `[`TransportContextManager`](#classyojimbo_1_1_transport_context_manager)` * m_contextManager` | The context manager. Manages the set of contexts on the transport, which allows certain addresses to be assigned to their own set of resources (like packet factories, message factories and allocators).
`protected bool m_allocateNetworkSimulator` | True if the network simulator was allocated in the constructor, and must be freed in the destructor.
`protected class `[`NetworkSimulator`](#classyojimbo_1_1_network_simulator)` * m_networkSimulator` | The network simulator. May be NULL.
`protected uint64_t m_counters` | The array of transport counters. Used for stats, debugging and telemetry.
`protected void ClearSendQueue()` | Clear the packet send queue.
`protected void ClearReceiveQueue()` | Clear the packet receive queue.
`protected const uint8_t * WritePacket(const `[`Address`](#classyojimbo_1_1_address)` & address,`[`Packet`](#classyojimbo_1_1_packet)` * packet,uint64_t sequence,int & packetBytes)` | Writes a packet to a scratch buffer and returns a pointer to the packet data.
`protected void WritePacketToSimulator(const `[`Address`](#classyojimbo_1_1_address)` & address,`[`Packet`](#classyojimbo_1_1_packet)` * packet,uint64_t sequence)` | Write a packet and queue it up in the network simulator.
`protected void WriteAndFlushPacket(const `[`Address`](#classyojimbo_1_1_address)` & address,`[`Packet`](#classyojimbo_1_1_packet)` * packet,uint64_t sequence)` | Write a packet and flush it to the network.
`protected `[`Packet`](#classyojimbo_1_1_packet)` * ReadPacket(const `[`Address`](#classyojimbo_1_1_address)` & address,uint8_t * packetBuffer,int packetBytes,uint64_t & sequence)` | Read a packet buffer that arrived from the network and deserialize it into a newly created packet object.
`protected virtual bool ShouldPacketsGoThroughSimulator()` | Should sent packets go through the simulator first before they are flushed to the network?
`protected void InternalSendPacket(const `[`Address`](#classyojimbo_1_1_address)` & to,const void * packetData,int packetBytes)` | Internal function to send a packet over the network.
`protected int InternalReceivePacket(`[`Address`](#classyojimbo_1_1_address)` & from,void * packetData,int maxPacketSize)` | Internal function to receive a packet from the network.

## Members

#### `public  BaseTransport(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,const `[`Address`](#classyojimbo_1_1_address)` & address,uint64_t protocolId,double time,int maxPacketSize,int sendQueueSize,int receiveQueueSize,bool allocateNetworkSimulator)` 

Base transport constructor.

#### Parameters
* `allocator` The allocator used for transport allocations. 


* `address` The address of the packet. This is how other other transports would send packets to this transport. 


* `protocolId` The protocol id for this transport. Protocol id is included in the packet header, packets received with a different protocol id are discarded. This allows multiple versions of your protocol to exist on the same network. 


* `time` The current time value in seconds. 


* `maxPacketSize` The maximum packet size that can be sent across this transport. 


* `sendQueueSize` The size of the packet send queue (number of packets). 


* `receiveQueueSize` The size of the packet receive queue (number of packets). 


* `allocateNetworkSimulator` If true then a network simulator is allocated, otherwise a network simulator is not allocated. You can use this to disable the network simulator if you are not using it, to save memory.

#### `public  ~BaseTransport()` 





#### `public virtual void SetContext(const `[`TransportContext`](#structyojimbo_1_1_transport_context)` & context)` 

Set the transport context.

This is the default context to be used for reading and writing packets. See [TransportContext](#structyojimbo_1_1_transport_context).

The user can also call [Transport::AddContextMapping](#classyojimbo_1_1_transport_1a4d0e96d87d6a7b3890ca077614e7a4e8) to associate up to [yojimbo::MaxContextMappings](#namespaceyojimbo_1a3eb912b430ab10ab4afc8737d3ea883a) contexts with particular addresses. When a context is associated with an address, that context is when reading/writing packets from/to that address instead of this one.


#### Parameters
* `context` The default context for reading and writing packets.

#### `public virtual void ClearContext()` 

Clear context.

This function clears the default context set on the transport.

This returns the transport to the state before a context was set on it. While in this state, the transport cannot read or write packets and will early out of [Transport::ReadPackets](#classyojimbo_1_1_transport_1abbd058ddc3242bcfced01954c3d78dfa) and [Transport::WritePackets](#classyojimbo_1_1_transport_1a903705df34df07acd44eccf3c6ccacff).

If the transport is set on a client, it is returned to this state after the client disconnects. On the server, the transport is returned to this state when the server stops.


**See also**: [Client::Disconnect](#classyojimbo_1_1_client_1a9eb150732daeadb0be17e1b16f3b7658)


**See also**: [Server::Stop](#classyojimbo_1_1_server_1a4f42d2c99693559c322cf2eb1eefbba9)

#### `public virtual void Reset()` 

Reset the transport.

This function completely resets the transport, clearing all packet send and receive queues and resetting all contexts and encryption mappings.

This gets the transport back into a pristine state, ready to be used for some other purpose, without needing to destroy and recreate the transport object.

#### `public virtual void SendPacket(const `[`Address`](#classyojimbo_1_1_address)` & address,`[`Packet`](#classyojimbo_1_1_packet)` * packet,uint64_t sequence,bool immediate)` 

[Queue](#classyojimbo_1_1_queue) a packet to be sent.

Ownership of the packet object is transferred to the transport. Don't call [Packet::Destroy](#classyojimbo_1_1_packet_1a597f853846f3d75213281625b25c9d16) on the packet after you send it with this function, the transport will do that for you automatically.

IMPORTANT: The packet will be sent over a UDP-equivalent network. It may arrive out of order, in duplicate or not at all.


#### Parameters
* `address` The address the packet should be sent to. 


* `packet` The packet that is being sent. 


* `sequence` The 64 bit sequence number of the packet used as a nonce for packet encryption. Should increase with each packet sent per-encryption mapping, but this is up to the caller. 


* `immediate` If true the the packet is written and flushed to the network immediately, rather than in the next call to [Transport::WritePackets](#classyojimbo_1_1_transport_1a903705df34df07acd44eccf3c6ccacff).

#### `public virtual `[`Packet`](#classyojimbo_1_1_packet)` * ReceivePacket(`[`Address`](#classyojimbo_1_1_address)` & from,uint64_t * sequence)` 

Receive a packet.

This function pops a packet off the receive queue, if any are available to be received.

This function transfers ownership of the packet pointer to the caller. It is the callers responsibility to call [Packet::Destroy](#classyojimbo_1_1_packet_1a597f853846f3d75213281625b25c9d16) after processing the packet.

To make sure packet latency is minimized call Transport::ReceivePackets just before looping and calling this function until it returns NULL.


#### Parameters
* `from` The address that the packet was received from. 


* `sequence` Pointer to an unsigned 64bit sequence number (optional). If the pointer is not NULL, it will be dereferenced to store the sequence number of the received packet. Only encrypted packets have these sequence numbers. 





#### Returns
The next packet in the receive queue, or NULL if no packets are left in the receive queue.

#### `public virtual void WritePackets()` 

Iterates across all packets in the send queue and writes them to the network.

To minimize packet latency, call this function shortly after you have finished queueing up packets to send via [Transport::SendPacket](#classyojimbo_1_1_transport_1aa9c2dec77b36ddd19296c35533173b5f).

#### `public virtual void ReadPackets()` 

Reads packets from the network and adds them to the packet receive queue.

To minimize packet latency call this function right before you loop calling [Transport::ReceivePacket](#classyojimbo_1_1_transport_1affa39a75fc878fde8641dfd888a21137) until it returns NULL.

#### `public virtual int GetMaxPacketSize() const` 

Returns the maximum packet size supported by this transport.

This is typically configured in the constructor of the transport implementation.

It's added here so callers using the transport interface know the maximum packet size that can be sent over the transport.


#### Returns
The maximum packet size that can be sent over this transport (bytes).

#### `public virtual void SetNetworkConditions(float latency,float jitter,float packetLoss,float duplicate)` 

Add simulated network conditions on top of this transport.

By default no latency, jitter, packet loss, or duplicates are added on top of the network.

However, during development you often want to simulate some latency and packet loss while the game is being played over the LAN, so people developing your game do so under conditions that you can expect to encounter when it's being played over the Internet.

I recommend adding around 125ms of latency and somewhere between 2-5% packet loss, and +/- one frame of jitter @ 60HZ (eg. 20ms or so).

IMPORTANT: Take care when sitting simulated network conditions because they are implemented on packet send only, so you usually want to set HALF of the latency you want on client, and HALF of the latency on the server transport. So for 100ms of latency in total, you'd set 50ms latency on the client transport, and 50ms latency on the server transport, which adds up to of 100ms extra round trip delay.

The network simulator allocated here can take up a significant amount of memory. If you want to save memory, you might want to disable the network simulator. You can do this in the constructor of your transport implementation. See [NetworkTransport::NetworkTransport](#classyojimbo_1_1_network_transport_1add26cbdcc1e3584a892004f3d134cdf6) for details.


#### Parameters
* `latency` The amount of latency to add to each packet sent (milliseconds). 


* `jitter` The amount of jitter to add to each packet sent (milliseconds). The packet delivery time is adjusted by some random amount within +/- jitter. This is best used in combination with some amount of latency, otherwise the jitter is not truly +/-. 


* `packetLoss` The percentage of packets to drop on send. 100% drops all packets. 0% drops no packets. 


* `duplicate` The percentage of packets to be duplicated. Duplicate packets are scheduled to be sent at some random time up to 1 second in the future, to grossly approximate duplicate packets that occur from IP route changes.

#### `public virtual void ClearNetworkConditions()` 

Clear network conditions back to defaults.

After this function latency, jitter, packet loss and duplicate packets are all set back to zero.

#### `public virtual void EnablePacketEncryption()` 

Turns on packet encryption and enables it for all packet types.

When a packet is sent the transport checks if that packet type is encrypted. If it's an encrypted, the transport looks for an encryption mapping for the destination address, and encrypts the packet with that key. Otherwise, the packet is sent out unencrypted.

When a packet is received, the transport checks if that packetet type should be encrypted. If it should be, but the packet itself is not encrypted, the packet is discarded. Otherwise, the server looks for a decryption key for the packet sender address and decrypts the packet before adding it to the packet receive queue.

The exception to this rule is when you enable TRANSPORT_FLAG_INSECURE_MODE via [Transport::SetFlags](#classyojimbo_1_1_transport_1aa7cd42ebe2b149a71ed05438c76d3417), which makes encryption optional for encrypted packet types.

This allows a mixture of secure and insecure client connections, which is convenient for development over a LAN, but should NEVER be enabled in production.

Please make sure you #define YOJIMBO_SECURE_MODE 1 in your production build!


**See also**: [Transport::DisablePacketEncryption](#classyojimbo_1_1_transport_1ab319325cba1edddf68ebf2f4fa593acb)


**See also**: [Transport::DisableEncryptionForPacketType](#classyojimbo_1_1_transport_1aa102f26f89f0b48bf8de436344d924e0)

#### `public virtual void DisablePacketEncryption()` 

Disables encryption for all packet types.

**See also**: [Transport::DisablePacketEncryption](#classyojimbo_1_1_transport_1ab319325cba1edddf68ebf2f4fa593acb)


**See also**: [Transport::DisableEncryptionForPacketType](#classyojimbo_1_1_transport_1aa102f26f89f0b48bf8de436344d924e0)

#### `public virtual void DisableEncryptionForPacketType(int type)` 

Disables encryption for a specific packet type.

Typical usage is to enable packet encryption (for all types) via [Transport::EnablePacketEncryption](#classyojimbo_1_1_transport_1ac3c1c9cd04e06ce14af28c2097d960e8), and then selectively disable it for packet types that you don't want to be encrypted.

For example, the client/server protocol sends connection request packets as unencrypted, because they contain connect token data which is already encrypted, and every other packet sent is encrypted.


#### Parameters
* `type` The packet type that should be set as not encrypted.

#### `public virtual bool IsEncryptedPacketType(int type) const` 

Is a packet type encrypted?

#### Returns
True if the packet type is an encrypted packet, false otherwise.


**See also**: [EnablePacketEncryption](#classyojimbo_1_1_base_transport_1a89be1972b133dea439b66610dd8464e1)


**See also**: [DisablePacketEncryption](#classyojimbo_1_1_base_transport_1a229464d93f4d55781466d40858ba3dc0)


**See also**: [DisableEncryptionForPacketType](#classyojimbo_1_1_base_transport_1a24a2a293d8012058b74c883415e8dafc)


#### Returns
True if the packet type is encrypted, false otherwise.

#### `public virtual bool AddEncryptionMapping(const `[`Address`](#classyojimbo_1_1_address)` & address,const uint8_t * sendKey,const uint8_t * receiveKey,double timeout)` 

Associates an address with keys for packet encryption.

This mapping is used by the transport on packet send and receive determine what key should be used when sending a packet to an address, and what key should be used to decrypt a packet received from an address.

For example, the server adds an encryption mapping for clients when it receives a connection request packet with a valid connect token, enabling encrypted packets to be exchanged between the server and that client past that point. The encryption mapping is removed when the client disconnects from the server.

Encryption mappings also time out, making them ideal for pending clients, who may never reply back and complete the connection negotiation. As a result, there are more encryption mappings than client slots. See [yojimbo::MaxEncryptionMappings](#namespaceyojimbo_1ac788010bac7b62c964e84bbfd35fa5a4).

See [EncryptionManager](#classyojimbo_1_1_encryption_manager) for further details.


#### Parameters
* `address` The address to associate with encryption keys. 


* `sendKey` The key used to encrypt packets sent to this address. 


* `receiveKey` The key used to decrypt packets received from this address. 


* `timeout` The timeout value in seconds for this encryption mapping (seconds). Encyrption mapping times out if no packets are sent to or received from this address in the timeout period.





#### Returns
True if the encryption mapping was added successfully, false otherwise.

#### `public virtual bool RemoveEncryptionMapping(const `[`Address`](#classyojimbo_1_1_address)` & address)` 

Remove the encryption mapping for an address.

#### Parameters
* `address` The address of the encryption mapping to remove.





#### Returns
True if an encryption mapping for the address exists and was removed, false if no encryption mapping could be found for the address.

#### `public virtual int FindEncryptionMapping(const `[`Address`](#classyojimbo_1_1_address)` & address)` 

Find the index of an encryption mapping by address.

#### Returns
The index of the encryption mapping in the range [0,MaxEncryptionMappings-1], or -1 if no encryption mapping exists for the specified address.

#### `public virtual void ResetEncryptionMappings()` 

Reset all encryption mappings.

All encryption mappings set on the transport are removed.


**See also**: [Transport::Reset](#classyojimbo_1_1_transport_1a3e113502a15c39867026864b72b2d125)

#### `public virtual bool AddContextMapping(const `[`Address`](#classyojimbo_1_1_address)` & address,const `[`TransportContext`](#structyojimbo_1_1_transport_context)` & context)` 

Add a transport context that is specific for an address.

Context mappings are used to give each connected client on the server its own set of resources, so malicious clients can't exhaust resources shared with other clients.

When a client establishes connection with the server, a context mapping is added for that client. When the client disconnects from the server, the context mapping is removed.


#### Parameters
* `address` The address to associate with a transport context. 


* `context` The transport context to be copied across. All data in the context must remain valid while it is set on the transport.





**See also**: [TransportContextManager](#classyojimbo_1_1_transport_context_manager)


**See also**: [yojimbo::MaxContextMappings](#namespaceyojimbo_1a3eb912b430ab10ab4afc8737d3ea883a)

#### `public virtual bool RemoveContextMapping(const `[`Address`](#classyojimbo_1_1_address)` & address)` 

Remove a context mapping.

Context mappings are used to give each connected client on the server its own set of resources, so malicious clients can't exhaust resources shared with other clients.

When a client establishes connection with the server, a context mapping is added for that client. When the client disconnects from the server, the context mapping is removed.


#### Parameters
* `address` The address of the context to remove. 





#### Returns
True if a context mapping was found at the address and removed, false if no context mapping could be found for the address.

#### `public virtual void ResetContextMappings()` 

Reset all context mappings.

After this function is called all context mappings are removed.

#### `public virtual void AdvanceTime(double time)` 

Advance transport time.

Call this at the end of each frame to advance the transport time forward.

IMPORTANT: Please use a double for your time value so it maintains sufficient accuracy as time increases.

#### `public virtual double GetTime() const` 

Gets the current transport time.

**See also**: [Transport::AdvanceTime](#classyojimbo_1_1_transport_1afbc173c15dc6a2d2eee3ac1f93d47d90)

#### `public virtual uint64_t GetCounter(int index) const` 

Get a counter value.

Counters are used to track event and actions performed by the transport. They are useful for debugging, testing and telemetry.


#### Returns
The counter value. See [yojimbo::TransportCounters](#namespaceyojimbo_1a2d157756c1035b5c7f088771cd9aca07) for the set of transport counters.

#### `public virtual void ResetCounters()` 

Reset all counters to zero.

This is typically used with a telemetry application after uploading the current set of counters to the telemetry backend.

This way you can continue to accumulate events, and upload them at some frequency, like every 5 minutes to the telemetry backend, without double counting events.

#### `public virtual void SetFlags(uint64_t flags)` 

Set transport flags.

Flags are used to enable and disable transport functionality.


#### Parameters
* `flags` The transport flags to set. See [yojimbo::TransportFlags](#namespaceyojimbo_1a597f4f1f57cad3cfc85f0d89f74cbd6d) for the set of transport flags that can be passed in.





**See also**: [Transport::GetFlags](#classyojimbo_1_1_transport_1aa57ac8bdf036a39e4cf9952bb1f9753d)

#### `public virtual uint64_t GetFlags() const` 

Get the current transport flags.

#### Returns
The transport flags. See [yojimbo::TransportFlags](#namespaceyojimbo_1a597f4f1f57cad3cfc85f0d89f74cbd6d) for the set of transport flags.


**See also**: [Transport::SetFlags](#classyojimbo_1_1_transport_1aa7cd42ebe2b149a71ed05438c76d3417)

#### `public virtual const `[`Address`](#classyojimbo_1_1_address)` & GetAddress() const` 

Get the address of the transport.

This is the address that packet should be sent to to be received by this transport.


#### Returns
The transport address.

#### `public virtual uint64_t GetProtocolId() const` 

Get the protocol id for the transport.

Protocol id is used to enable multiple versions of your protocol at the same time, by excluding communication between protocols with different ids.

#### `protected `[`TransportContext`](#structyojimbo_1_1_transport_context)` m_context` 

The default transport context used if no context can be found for the specific address.



#### `protected `[`Address`](#classyojimbo_1_1_address)` m_address` 

The address of the transport. This is the address to send packets to so that this transport would receive them.



#### `protected double m_time` 

The current transport time. See [Transport::AdvanceTime](#classyojimbo_1_1_transport_1afbc173c15dc6a2d2eee3ac1f93d47d90).



#### `protected uint64_t m_flags` 

The transport flags. See [Transport::SetFlags](#classyojimbo_1_1_transport_1aa7cd42ebe2b149a71ed05438c76d3417).



#### `protected uint64_t m_protocolId` 

The protocol id. You set it when you create the transport and it's used to filter out any packets sent that have different protocols ids. Lets you implement basic versioning (eg. ignore packets sent by other versions of your protocol on the network).



#### `protected `[`Allocator`](#classyojimbo_1_1_allocator)` * m_allocator` 

The allocator passed in to the transport constructor. Used for all allocations inside the transport class, except for packets, messages and stream allocations, which are determined by the factories and allocators passed in to the context.



#### `protected `[`PacketProcessor`](#classyojimbo_1_1_packet_processor)` * m_packetProcessor` 

The packet processor. This is the engine for reading and writing packets, as well as packet encryption and decryption.



#### `protected `[`Queue`](#classyojimbo_1_1_queue)`< PacketEntry > m_sendQueue` 

The packet send queue. Packets sent via [Transport::SendPacket](#classyojimbo_1_1_transport_1aa9c2dec77b36ddd19296c35533173b5f) are added to this queue (unless the immediate flag is true). This queue is flushed to the network each time [Transport::WritePackets](#classyojimbo_1_1_transport_1a903705df34df07acd44eccf3c6ccacff) is called.



#### `protected `[`Queue`](#classyojimbo_1_1_queue)`< PacketEntry > m_receiveQueue` 

The packet receive queue. As packets are read from the network in [Transport::ReadPackets](#classyojimbo_1_1_transport_1abbd058ddc3242bcfced01954c3d78dfa) they are added to this queue. Packets are popped off the receive queue by Transport::ReadPacket.



#### `protected uint8_t * m_allPacketTypes` 

An array with each entry for valid packet types from the [PacketFactory](#classyojimbo_1_1_packet_factory) set to 1.



#### `protected uint8_t * m_packetTypeIsEncrypted` 

An array with each entry set to 1 if that packet type is encrypted. See [Transport::EnablePacketEncryption](#classyojimbo_1_1_transport_1ac3c1c9cd04e06ce14af28c2097d960e8).



#### `protected uint8_t * m_packetTypeIsUnencrypted` 

An array with each entry set to 1 if that packet type is NOT encrypted.



#### `protected `[`EncryptionManager`](#classyojimbo_1_1_encryption_manager)` * m_encryptionManager` 

The encryption manager. Manages encryption contexts and lets the transport look up send and receive keys for encrypting and decrypting packets.



#### `protected `[`TransportContextManager`](#classyojimbo_1_1_transport_context_manager)` * m_contextManager` 

The context manager. Manages the set of contexts on the transport, which allows certain addresses to be assigned to their own set of resources (like packet factories, message factories and allocators).



#### `protected bool m_allocateNetworkSimulator` 

True if the network simulator was allocated in the constructor, and must be freed in the destructor.



#### `protected class `[`NetworkSimulator`](#classyojimbo_1_1_network_simulator)` * m_networkSimulator` 

The network simulator. May be NULL.



#### `protected uint64_t m_counters` 

The array of transport counters. Used for stats, debugging and telemetry.



#### `protected void ClearSendQueue()` 

Clear the packet send queue.



#### `protected void ClearReceiveQueue()` 

Clear the packet receive queue.



#### `protected const uint8_t * WritePacket(const `[`Address`](#classyojimbo_1_1_address)` & address,`[`Packet`](#classyojimbo_1_1_packet)` * packet,uint64_t sequence,int & packetBytes)` 

Writes a packet to a scratch buffer and returns a pointer to the packet data.

If the packet is an encrypted packet type, this function also performs encryption.


#### Parameters
* `address` The address of the packet. This is how other other transports would send packets to this transport. 


* `packet` The packet object to be serialized (written). 


* `sequence` The sequence number of the packet being written. If this is an encrypted packet, this serves as the nonce, and it is the users responsibility to increase this value with each packet sent per-encryption context. Not used for unencrypted packets (pass in zero). 


* `packetBytes` The number of packet bytes written to the buffer [out]. This is the wire size of the packet to be sent via sendto or equivalent.





#### Returns
A const pointer to the packet data written to a scratch buffer. Don't hold on to this pointer and don't free it. The scratch buffer is internal and managed by the transport.

#### `protected void WritePacketToSimulator(const `[`Address`](#classyojimbo_1_1_address)` & address,`[`Packet`](#classyojimbo_1_1_packet)` * packet,uint64_t sequence)` 

Write a packet and queue it up in the network simulator.

This is used when network simulation is enabled via [Transport::SetNetworkConditions](#classyojimbo_1_1_transport_1a42d02a4f403daa594568dec26a1ef5d2) to add latency, packet loss and so on to sent packets.

Packets are first queued up in the network simulation, and then when they pop off the network simulator they are flushed to the network.

This codepath is not called when network simulation is disabled. Packets are written and flushed directly to the network in that case.


#### Parameters
* `address` The address the packet is being sent to. 


* `packet` The packet object to be serialized (written). 


* `sequence` The sequence number of the packet being written. If this is an encrypted packet, this serves as the nonce, and it is the users responsibility to increase this value with each packet sent per-encryption context. Not used for unencrypted packets (pass in zero).





**See also**: BaseTransport::ShouldPacketGoThroughSimulator

#### `protected void WriteAndFlushPacket(const `[`Address`](#classyojimbo_1_1_address)` & address,`[`Packet`](#classyojimbo_1_1_packet)` * packet,uint64_t sequence)` 

Write a packet and flush it to the network.

This codepath is used when network simulation is disabled (eg. when no latency or packet loss is simulated).


#### Parameters
* `address` The address the packet is being sent to. 


* `packet` The packet object to be serialized (written). 


* `sequence` The sequence number of the packet being written. If this is an encrypted packet, this serves as the nonce, and it is the users responsibility to increase this value with each packet sent per-encryption context. Not used for unencrypted packets (pass in zero).





**See also**: [BaseTransport::InternalSendPacket](#classyojimbo_1_1_base_transport_1a65f560533fa6a2a1d7ec0c1a0044361a)


**See also**: BaseTransport::ShouldPacketGoThroughSimulator

#### `protected `[`Packet`](#classyojimbo_1_1_packet)` * ReadPacket(const `[`Address`](#classyojimbo_1_1_address)` & address,uint8_t * packetBuffer,int packetBytes,uint64_t & sequence)` 

Read a packet buffer that arrived from the network and deserialize it into a newly created packet object.

#### Parameters
* `address` The address that sent the packet. 


* `packetBuffer` The byte buffer containing the packet data received from the network. 


* `packetBytes` The size of the packet data being read in bytes. 


* `sequence` Reference to the sequence number for the packet. If this is an encrypted packet, it will be filled with the sequence number from the encrypted packet header (eg. the nonce). If this is an unencrypted packet, it will be set to zero.





#### Returns
The packet object if the packet buffer was successfully read (and decrypted), NULL otherwise. The caller owns the packet pointer and is responsible for destroying it. See [Packet::Destroy](#classyojimbo_1_1_packet_1a597f853846f3d75213281625b25c9d16).

#### `protected virtual bool ShouldPacketsGoThroughSimulator()` 

Should sent packets go through the simulator first before they are flushed to the network?

This is true in the case where network conditions are set on the simulator, which is how latency and packet loss is added on top of the transport.

Returning false means packets are written and flushed directly to the network via [BaseTransport::InternalSendPacket](#classyojimbo_1_1_base_transport_1a65f560533fa6a2a1d7ec0c1a0044361a).


#### Returns
True if packets should be directed through the simulator (see [BaseTransport::WritePacketToSimulator](#classyojimbo_1_1_base_transport_1aefe71ca0aff4b718ff7c10c0ccf51571)) and false if packets should be flushed directly to the network (see [BaseTransport::WriteAndFlushPacket](#classyojimbo_1_1_base_transport_1a9394b95e2e2aea523768c5102d279055)).


**See also**: [Transport::SetNetworkConditions](#classyojimbo_1_1_transport_1a42d02a4f403daa594568dec26a1ef5d2)

#### `protected void InternalSendPacket(const `[`Address`](#classyojimbo_1_1_address)` & to,const void * packetData,int packetBytes)` 

Internal function to send a packet over the network.

IMPORTANT: Override this to implement your own packet send function for derived transport classes.


#### Parameters
* `to` The address the packet should be sent to. 


* `packetData` The serialized, and potentially encrypted packet data generated by [BaseTransport::WritePacket](#classyojimbo_1_1_base_transport_1a7a2a0abc1756c513695118ea5d78b503). 


* `packetBytes` The size of the packet data in bytes.

#### `protected int InternalReceivePacket(`[`Address`](#classyojimbo_1_1_address)` & from,void * packetData,int maxPacketSize)` 

Internal function to receive a packet from the network.

IMPORTANT: Override this to implement your own packet receive function for derived transport classes.

IMPORTANT: This call must be non-blocking.


#### Parameters
* `from` The address that sent the packet [out]. 


* `packetData` The buffer to which will receive packet data read from the network. 


* `maxPacketSize` The size of your packet data buffer. Packets received from the network that are larger than this are discarded.





#### Returns
The number of packet bytes read from the network, 0 if no packet data was read.

# class `yojimbo::BitArray` 


A simple bit array class.

You can create a bit array with a number of bits, set, clear and test if each bit is set.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  BitArray(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,int size)` | The bit array constructor.
`public inline  ~BitArray()` | The bit array destructor.
`public inline void Clear()` | Clear all bit values to zero.
`public inline void SetBit(int index)` | Set a bit to 1.
`public inline void ClearBit(int index)` | Clear a bit to 0.
`public inline uint64_t GetBit(int index) const` | Get the value of the bit.
`public inline int GetSize() const` | Gets the size of the bit array, in number of bits.

## Members

#### `public inline  BitArray(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,int size)` 

The bit array constructor.

#### Parameters
* `allocator` The allocator to use. 


* `size` The number of bits in the bit array.





All bits are initially set to zero.

#### `public inline  ~BitArray()` 

The bit array destructor.



#### `public inline void Clear()` 

Clear all bit values to zero.



#### `public inline void SetBit(int index)` 

Set a bit to 1.

#### Parameters
* `index` The index of the bit.

#### `public inline void ClearBit(int index)` 

Clear a bit to 0.

#### Parameters
* `index` The index of the bit.

#### `public inline uint64_t GetBit(int index) const` 

Get the value of the bit.

Returns 1 if the bit is set, 0 if the bit is not set.


#### Parameters
* `index` The index of the bit.

#### `public inline int GetSize() const` 

Gets the size of the bit array, in number of bits.

#### Returns
The number of bits.

# class `yojimbo::BitReader` 


Reads bit packed integer values from a buffer.

Relies on the user reconstructing the exact same set of bit reads as bit writes when the buffer was written. This is an unattributed bitpacked binary stream!

Implementation: 32 bit dwords are read in from memory to the high bits of a scratch buffer as required. The user reads off bit values from the scratch buffer from the right, after which the scrathc buffer is shifted by the same number of bits.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  BitReader(const void * data,int bytes)` | Bit reader constructor.
`public inline bool WouldReadPastEnd(int bits) const` | Would the bit reader would read past the end of the buffer if it read this many bits?
`public inline uint32_t ReadBits(int bits)` | Read bits from the bit buffer.
`public inline bool ReadAlign()` | Read an align.
`public inline void ReadBytes(uint8_t * data,int bytes)` | Read bytes from the bitpacked data.
`public inline int GetAlignBits() const` | How many align bits would be read, if we were to read an align right now?
`public inline int GetBitsRead() const` | How many bits have we read so far?
`public inline int GetBitsRemaining() const` | How many bits are still available to read?

## Members

#### `public inline  BitReader(const void * data,int bytes)` 

Bit reader constructor.

Non-multiples of four buffer sizes are supported, as this naturally tends to occur when packets are read from the network.

However, actual buffer allocated for the packet data must round up at least to the next 4 bytes in memory, because the bit reader reads dwords from memory not bytes.


#### Parameters
* `data` Pointer to the bitpacked data to read. 


* `bytes` The number of bytes of bitpacked data to read.





**See also**: [BitWriter](#classyojimbo_1_1_bit_writer)

#### `public inline bool WouldReadPastEnd(int bits) const` 

Would the bit reader would read past the end of the buffer if it read this many bits?

#### Parameters
* `bits` The number of bits that would be read.





#### Returns
True if reading the number of bits would read past the end of the buffer.

#### `public inline uint32_t ReadBits(int bits)` 

Read bits from the bit buffer.

This function will assert in debug builds if this read would read past the end of the buffer.

In production situations, the higher level [ReadStream](#classyojimbo_1_1_read_stream) takes care of checking all packet data and never calling this function if it would read past the end of the buffer.


#### Parameters
* `bits` The number of bits to read in [1,32].





#### Returns
The integer value read in range [0,(1<<bits)-1].


**See also**: [BitReader::WouldReadPastEnd](#classyojimbo_1_1_bit_reader_1a2d36e85eae9b12b5d3cd8b0fb98710df)


**See also**: [BitWriter::WriteBits](#classyojimbo_1_1_bit_writer_1a7f90deecf2a3c7b9ad453ff1810ec3f4)

#### `public inline bool ReadAlign()` 

Read an align.

Call this on read to correspond to a WriteAlign call when the bitpacked buffer was written.

This makes sure we skip ahead to the next aligned byte index. As a safety check, we verify that the padding to next byte is zero bits and return false if that's not the case.

This will typically abort packet read. Just another safety measure...


#### Returns
True if we successfully read an align and skipped ahead past zero pad, false otherwise (probably means, no align was written to the stream).


**See also**: [BitWriter::WriteAlign](#classyojimbo_1_1_bit_writer_1a9ea7e451dee4326fe42b7ddf2e5f7ba1)

#### `public inline void ReadBytes(uint8_t * data,int bytes)` 

Read bytes from the bitpacked data.

**See also**: [BitWriter::WriteBytes](#classyojimbo_1_1_bit_writer_1a74adb11fd54ade6e57b626425ac519d1)

#### `public inline int GetAlignBits() const` 

How many align bits would be read, if we were to read an align right now?

#### Returns
Result in [0,7], where 0 is zero bits required to align (already aligned) and 7 is worst case.

#### `public inline int GetBitsRead() const` 

How many bits have we read so far?

#### Returns
The number of bits read from the bit buffer so far.

#### `public inline int GetBitsRemaining() const` 

How many bits are still available to read?

For example, if the buffer size is 4, we have 32 bits available to read, if we have already written 10 bytes then 22 are still available.


#### Returns
The number of bits available to read.

# class `yojimbo::BitWriter` 


Bitpacks unsigned integer values to a buffer.

Integer bit values are written to a 64 bit scratch value from right to left.

Once the low 32 bits of the scratch is filled with bits it is flushed to memory as a dword and the scratch buffer is shifted right by 32.

The bit stream is written to memory in little endian order, which is considered network byte order for this library.


**See also**: [BitReader](#classyojimbo_1_1_bit_reader)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  BitWriter(void * data,int bytes)` | Bit writer constructor.
`public inline void WriteBits(uint32_t value,int bits)` | Write bits to the buffer.
`public inline void WriteAlign()` | Write an alignment to the bit stream, padding zeros so the bit index becomes is a multiple of 8.
`public inline void WriteBytes(const uint8_t * data,int bytes)` | Write an array of bytes to the bit stream.
`public inline void FlushBits()` | Flush any remaining bits to memory.
`public inline int GetAlignBits() const` | How many align bits would be written, if we were to write an align right now?
`public inline int GetBitsWritten() const` | How many bits have we written so far?
`public inline int GetBitsAvailable() const` | How many bits are still available to write?
`public inline const uint8_t * GetData() const` | Get a pointer to the data written by the bit writer.
`public inline int GetBytesWritten() const` | The number of bytes flushed to memory.

## Members

#### `public inline  BitWriter(void * data,int bytes)` 

Bit writer constructor.

Creates a bit writer object to write to the specified buffer.


#### Parameters
* `data` The pointer to the buffer to fill with bitpacked data. 


* `bytes` The size of the buffer in bytes. Must be a multiple of 4, because the bitpacker reads and writes memory as dwords, not bytes.

#### `public inline void WriteBits(uint32_t value,int bits)` 

Write bits to the buffer.

Bits are written to the buffer as-is, without padding to nearest byte. Will assert if you try to write past the end of the buffer.

A boolean value writes just 1 bit to the buffer, a value in range [0,31] can be written with just 5 bits and so on.

IMPORTANT: When you have finished writing to your buffer, take care to call BitWrite::FlushBits, otherwise the last dword of data will not get flushed to memory!


#### Parameters
* `value` The integer value to write to the buffer. Must be in [0,(1<<bits)-1]. 


* `bits` The number of bits to encode in [1,32].





**See also**: [BitReader::ReadBits](#classyojimbo_1_1_bit_reader_1ac0bf045e1448d1706a661b55a4ea23b5)

#### `public inline void WriteAlign()` 

Write an alignment to the bit stream, padding zeros so the bit index becomes is a multiple of 8.

This is useful if you want to write some data to a packet that should be byte aligned. For example, an array of bytes, or a string.

IMPORTANT: If the current bit index is already a multiple of 8, nothing is written.


**See also**: [BitReader::ReadAlign](#classyojimbo_1_1_bit_reader_1a3ddb546a61f0e6d88e9683555679e0d5)

#### `public inline void WriteBytes(const uint8_t * data,int bytes)` 

Write an array of bytes to the bit stream.

Use this when you have to copy a large block of data into your bitstream.

Faster than just writing each byte to the bit stream via BitWriter::WriteBits( value, 8 ), because it aligns to byte index and copies into the buffer without bitpacking.


#### Parameters
* `data` The byte array data to write to the bit stream. 


* `bytes` The number of bytes to write.





**See also**: [BitReader::ReadBytes](#classyojimbo_1_1_bit_reader_1a4d93c27f260a8021784f435c138dfa98)

#### `public inline void FlushBits()` 

Flush any remaining bits to memory.

Call this once after you've finished writing bits to flush the last dword of scratch to memory!


**See also**: [BitWriter::WriteBits](#classyojimbo_1_1_bit_writer_1a7f90deecf2a3c7b9ad453ff1810ec3f4)

#### `public inline int GetAlignBits() const` 

How many align bits would be written, if we were to write an align right now?

#### Returns
Result in [0,7], where 0 is zero bits required to align (already aligned) and 7 is worst case.

#### `public inline int GetBitsWritten() const` 

How many bits have we written so far?

#### Returns
The number of bits written to the bit buffer.

#### `public inline int GetBitsAvailable() const` 

How many bits are still available to write?

For example, if the buffer size is 4, we have 32 bits available to write, if we have already written 10 bytes then 22 are still available to write.


#### Returns
The number of bits available to write.

#### `public inline const uint8_t * GetData() const` 

Get a pointer to the data written by the bit writer.

Corresponds to the data block passed in to the constructor.


#### Returns
Pointer to the data written by the bit writer.

#### `public inline int GetBytesWritten() const` 

The number of bytes flushed to memory.

This is effectively the size of the packet that you should send after you have finished bitpacking values with this class.

The returned value is not always a multiple of 4, even though we flush dwords to memory. You won't miss any data in this case because the order of bits written is designed to work with the little endian memory layout.

IMPORTANT: Make sure you call [BitWriter::FlushBits](#classyojimbo_1_1_bit_writer_1a03fb30b2bb170ace192f883334995cd3) before calling this method, otherwise you risk missing the last dword of data.

# class `yojimbo::BlockMessage` 

```
class yojimbo::BlockMessage
  : public yojimbo::Message
```  

A message that can have a block of data attached to it.

Attaching blocks of data is very useful, especially over a reliable-ordered channel where these blocks can be larger that the maximum packet size. Blocks sent over a reliable-ordered channel are automatically split up into fragments and reassembled on the other side.

This gives you have the convenience of a reliable-ordered control messages, while attaching large blocks of data (larger than max packet size), while having all messages delivered reliably and in-order.

Situations where this can be useful is when sending down the initial state of the world on client connect, or block of configuration data to send up from the client to server on connect.

It can also be used for messages sent across an unreliable-unordered channel, but in that case blocks aren't split up into fragments. Make sure you consider this when designing your channel budgets when sending blocks over unreliable-unordered channels.


**See also**: [ChannelConfig](#structyojimbo_1_1_channel_config)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  explicit BlockMessage()` | Block message constructor.
`public inline void AttachBlock(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,uint8_t * blockData,int blockSize)` | Attach a block to this message.
`public inline void DetachBlock()` | Detach the block from this message.
`public inline `[`Allocator`](#classyojimbo_1_1_allocator)` * GetAllocator()` | Get the allocator used to allocate the block.
`public inline uint8_t * GetBlockData()` | Get the block data pointer.
`public inline int GetBlockSize() const` | Get the size of the block attached to this message.
`public template<typename Stream>`  <br/>`inline bool Serialize(Stream & stream)` | Templated serialize function for the block message.
`public  YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()` | 
`protected inline  ~BlockMessage()` | If a block was attached to the message, it is freed here.

## Members

#### `public inline  explicit BlockMessage()` 

Block message constructor.

Don't call this directly, use a message factory instead.


**See also**: [MessageFactory::Create](#classyojimbo_1_1_message_factory_1a29aad59d68607c3d279c61d394f5b56a)

#### `public inline void AttachBlock(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,uint8_t * blockData,int blockSize)` 

Attach a block to this message.

You can only attach one block. This method will assert if a block is already attached.

#### `public inline void DetachBlock()` 

Detach the block from this message.

By doing this you are responsible for copying the block pointer and allocator and making sure the block is freed.

This could be used for example, if you wanted to copy off the block and store it somewhere, without the cost of copying it.

#### `public inline `[`Allocator`](#classyojimbo_1_1_allocator)` * GetAllocator()` 

Get the allocator used to allocate the block.

#### Returns
The allocator for the block. NULL if no block is attached to this message.

#### `public inline uint8_t * GetBlockData()` 

Get the block data pointer.

#### Returns
The block data pointer. NULL if no block is attached.

#### `public inline int GetBlockSize() const` 

Get the size of the block attached to this message.

#### Returns
The size of the block (bytes). 0 if no block is attached.

#### `public template<typename Stream>`  <br/>`inline bool Serialize(Stream & stream)` 

Templated serialize function for the block message.

Doesn't do anything. The block data is serialized elsewhere.

You can override the serialize methods on a block message to implement your own serialize function. It's just like a regular message with a block attached to it.


**See also**: [ConnectionPacket](#structyojimbo_1_1_connection_packet)


**See also**: [ChannelPacketData](#structyojimbo_1_1_channel_packet_data)


**See also**: [ReliableOrderedChannel](#classyojimbo_1_1_reliable_ordered_channel)


**See also**: [UnreliableUnorderedChannel](#classyojimbo_1_1_unreliable_unordered_channel)

#### `public  YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()` 





#### `protected inline  ~BlockMessage()` 

If a block was attached to the message, it is freed here.



# class `yojimbo::Channel` 


Common functionality shared across all channel types.



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  Channel(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,`[`MessageFactory`](#classyojimbo_1_1_message_factory)` & messageFactory,const `[`ChannelConfig`](#structyojimbo_1_1_channel_config)` & config,int channelId)` | [Channel](#classyojimbo_1_1_channel) constructor.
`public inline virtual  ~Channel()` | [Channel](#classyojimbo_1_1_channel) destructor.
`public void Reset()` | Reset the channel.
`public bool CanSendMsg() const` | Returns true if a message can be sent over this channel.
`public void SendMsg(`[`Message`](#classyojimbo_1_1_message)` * message)` | [Queue](#classyojimbo_1_1_queue) a message to be sent across this channel.
`public `[`Message`](#classyojimbo_1_1_message)` * ReceiveMsg()` | Pops the next message off the receive queue if one is available.
`public void AdvanceTime(double time)` | Advance channel time.
`public int GetPacketData(`[`ChannelPacketData`](#structyojimbo_1_1_channel_packet_data)` & packetData,uint16_t packetSequence,int availableBits)` | Get channel packet data for this channel.
`public void ProcessPacketData(const `[`ChannelPacketData`](#structyojimbo_1_1_channel_packet_data)` & packetData,uint16_t packetSequence)` | Process packet data included in a connection packet.
`public void ProcessAck(uint16_t sequence)` | Process a connection packet ack.
`public inline void SetListener(`[`ChannelListener`](#classyojimbo_1_1_channel_listener)` * listener)` | Set the channel listener.
`public `[`ChannelError`](#namespaceyojimbo_1a5d1a971be9217895726d6e06802479b8)` GetError() const` | Get the channel error level.
`public int GetChannelId() const` | Gets the channel id.
`public uint64_t GetCounter(int index) const` | Get a counter value.
`public void ResetCounters()` | Resets all counter values to zero.
`protected const `[`ChannelConfig`](#structyojimbo_1_1_channel_config)` m_config` | [Channel](#classyojimbo_1_1_channel) configuration data.
`protected `[`Allocator`](#classyojimbo_1_1_allocator)` * m_allocator` | [Allocator](#classyojimbo_1_1_allocator) for allocations matching life cycle of this channel.
`protected int m_channelId` | The channel id in [0,numChannels-1].
`protected double m_time` | The current time.
`protected `[`ChannelError`](#namespaceyojimbo_1a5d1a971be9217895726d6e06802479b8)` m_error` | The channel error level.
`protected `[`ChannelListener`](#classyojimbo_1_1_channel_listener)` * m_listener` | [Channel](#classyojimbo_1_1_channel) listener for callbacks. Optional.
`protected `[`MessageFactory`](#classyojimbo_1_1_message_factory)` * m_messageFactory` | [Message](#classyojimbo_1_1_message) factory for creaing and destroying messages.
`protected uint64_t m_counters` | Counters for unit testing, stats etc.
`protected void SetError(`[`ChannelError`](#namespaceyojimbo_1a5d1a971be9217895726d6e06802479b8)` error)` | Set the channel error state.

## Members

#### `public  Channel(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,`[`MessageFactory`](#classyojimbo_1_1_message_factory)` & messageFactory,const `[`ChannelConfig`](#structyojimbo_1_1_channel_config)` & config,int channelId)` 

[Channel](#classyojimbo_1_1_channel) constructor.



#### `public inline virtual  ~Channel()` 

[Channel](#classyojimbo_1_1_channel) destructor.



#### `public void Reset()` 

Reset the channel.



#### `public bool CanSendMsg() const` 

Returns true if a message can be sent over this channel.



#### `public void SendMsg(`[`Message`](#classyojimbo_1_1_message)` * message)` 

[Queue](#classyojimbo_1_1_queue) a message to be sent across this channel.

#### Parameters
* `message` The message to be sent.

#### `public `[`Message`](#classyojimbo_1_1_message)` * ReceiveMsg()` 

Pops the next message off the receive queue if one is available.

#### Returns
A pointer to the received message, NULL if there are no messages to receive. The caller owns the message object returned by this function and is responsible for releasing it via [Message::Release](#classyojimbo_1_1_message_1a5979f1559786c1606dde8df944b24d26).

#### `public void AdvanceTime(double time)` 

Advance channel time.

Called by [Connection::AdvanceTime](#classyojimbo_1_1_connection_1a268d70c87da4a4185bfe6b947b8e7b7a) for each channel configured on the connection.

#### `public int GetPacketData(`[`ChannelPacketData`](#structyojimbo_1_1_channel_packet_data)` & packetData,uint16_t packetSequence,int availableBits)` 

Get channel packet data for this channel.

#### Parameters
* `packetData` The channel packet data to be filled [out] 


* `packetSequence` The sequence number of the packet being generated. 


* `availableBits` The maximum number of bits of packet data the channel is allowed to write.





#### Returns
The number of bits of packet data written by the channel.


**See also**: [ConnectionPacket](#structyojimbo_1_1_connection_packet)


**See also**: [Connection::GeneratePacket](#classyojimbo_1_1_connection_1aaf8a19bcdd9ad05ff598b6e975881290)

#### `public void ProcessPacketData(const `[`ChannelPacketData`](#structyojimbo_1_1_channel_packet_data)` & packetData,uint16_t packetSequence)` 

Process packet data included in a connection packet.

#### Parameters
* `packetData` The channel packet data to process. 


* `packetSequence` The sequence number of the connection packet that contains the channel packet data.





**See also**: [ConnectionPacket](#structyojimbo_1_1_connection_packet)


**See also**: [Connection::ProcessPacket](#classyojimbo_1_1_connection_1a6118515c3a0f1764e5f27a378df87886)

#### `public void ProcessAck(uint16_t sequence)` 

Process a connection packet ack.

Depending on the channel type: 1. Acks messages and block fragments so they stop being included in outgoing connection packets (reliable-ordered channel)

2. Does nothing at all (unreliable-unordered).



#### Parameters
* `sequence` The sequence number of the connection packet that was acked.

#### `public inline void SetListener(`[`ChannelListener`](#classyojimbo_1_1_channel_listener)` * listener)` 

Set the channel listener.

The typical usage is to set the connection as the channel listener, so it gets callbacks from channels it owns.


**See also**: [Connection](#classyojimbo_1_1_connection)

#### `public `[`ChannelError`](#namespaceyojimbo_1a5d1a971be9217895726d6e06802479b8)` GetError() const` 

Get the channel error level.

#### Returns
The channel error.

#### `public int GetChannelId() const` 

Gets the channel id.

#### Returns
The channel id in [0,numChannels-1].

#### `public uint64_t GetCounter(int index) const` 

Get a counter value.

#### Parameters
* `index` The index of the counter to retrieve. See ChannelCounters. 





#### Returns
The value of the counter.


**See also**: [ResetCounters](#classyojimbo_1_1_channel_1a2ef33acbf8c109d2f639721b68767660)

#### `public void ResetCounters()` 

Resets all counter values to zero.



#### `protected const `[`ChannelConfig`](#structyojimbo_1_1_channel_config)` m_config` 

[Channel](#classyojimbo_1_1_channel) configuration data.



#### `protected `[`Allocator`](#classyojimbo_1_1_allocator)` * m_allocator` 

[Allocator](#classyojimbo_1_1_allocator) for allocations matching life cycle of this channel.



#### `protected int m_channelId` 

The channel id in [0,numChannels-1].



#### `protected double m_time` 

The current time.



#### `protected `[`ChannelError`](#namespaceyojimbo_1a5d1a971be9217895726d6e06802479b8)` m_error` 

The channel error level.



#### `protected `[`ChannelListener`](#classyojimbo_1_1_channel_listener)` * m_listener` 

[Channel](#classyojimbo_1_1_channel) listener for callbacks. Optional.



#### `protected `[`MessageFactory`](#classyojimbo_1_1_message_factory)` * m_messageFactory` 

[Message](#classyojimbo_1_1_message) factory for creaing and destroying messages.



#### `protected uint64_t m_counters` 

Counters for unit testing, stats etc.



#### `protected void SetError(`[`ChannelError`](#namespaceyojimbo_1a5d1a971be9217895726d6e06802479b8)` error)` 

Set the channel error state.

All errors go through this function to make debug logging easier.


**See also**: [yojimbo::debug_printf](#namespaceyojimbo_1a51e7d7911f0505532fef5117cfdc266f)

# class `yojimbo::ChannelListener` 


Implement this interface to receive callbacks for channel events.



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline virtual  ~ChannelListener()` | 
`public inline virtual void OnChannelFragmentReceived(class `[`Channel`](#classyojimbo_1_1_channel)` * channel,uint16_t messageId,uint16_t fragmentId,int fragmentBytes,int numFragmentsReceived,int numFragmentsInBlock)` | Override this method to get a callback when a block fragment is received.

## Members

#### `public inline virtual  ~ChannelListener()` 





#### `public inline virtual void OnChannelFragmentReceived(class `[`Channel`](#classyojimbo_1_1_channel)` * channel,uint16_t messageId,uint16_t fragmentId,int fragmentBytes,int numFragmentsReceived,int numFragmentsInBlock)` 

Override this method to get a callback when a block fragment is received.

#### Parameters
* `channel` The channel the block is being sent over. 


* `messageId` The message id the block is attached to. 


* `fragmentId` The fragment id that is being processed. Fragment ids are in the range [0,numFragments-1]. 


* `fragmentBytes` The size of the fragment in bytes. 


* `numFragmentsReceived` The number of fragments received for this block so far (including this one). 


* `numFragmentsInBlock` The total number of fragments in this block. The block receive completes when all fragments are received.





**See also**: [BlockMessage::AttachBlock](#classyojimbo_1_1_block_message_1af88bdd0b14d6ff6d73144dddb6581563)

# class `yojimbo::Client` 

```
class yojimbo::Client
  : public yojimbo::ConnectionListener
```  

A client that connects to a server.

This class is designed to be inherited from to create your own client class.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  explicit Client(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,`[`Transport`](#classyojimbo_1_1_transport)` & transport,const `[`ClientServerConfig`](#structyojimbo_1_1_client_server_config)` & config,double time)` | The client constructor.
`public virtual  ~Client()` | The client destructor.
`public void SetUserContext(void * context)` | Set the user context.
`public void InsecureConnect(uint64_t clientId,const `[`Address`](#classyojimbo_1_1_address)` & serverAddress)` | Connect to a server (insecure).
`public void InsecureConnect(uint64_t clientId,const `[`Address`](#classyojimbo_1_1_address)` serverAddresses,int numServerAddresses)` | Connect to a list of servers (insecure).
`public void Connect(uint64_t clientId,const `[`Address`](#classyojimbo_1_1_address)` & serverAddress,const uint8_t * connectTokenData,const uint8_t * connectTokenNonce,const uint8_t * clientToServerKey,const uint8_t * serverToClientKey,uint64_t connectTokenExpireTimestamp)` | Connect to a server (secure).
`public void Connect(uint64_t clientId,const `[`Address`](#classyojimbo_1_1_address)` serverAddresses,int numServerAddresses,const uint8_t * connectTokenData,const uint8_t * connectTokenNonce,const uint8_t * clientToServerKey,const uint8_t * serverToClientKey,uint64_t connectTokenExpireTimestamp)` | Connect to a list of servers (secure).
`public void Disconnect(`[`ClientState`](#namespaceyojimbo_1ab646aeedaf6bd6841c1e7fd9917a47f8)` clientState,bool sendDisconnectPacket)` | Disconnect from the server.
`public void SendPackets()` | Send packets to server.
`public void ReceivePackets()` | Receive packets sent from the server.
`public void CheckForTimeOut()` | Check for timeouts.
`public void AdvanceTime(double time)` | Advance client time.
`public bool IsConnecting() const` | Is the client connecting to a server?
`public bool IsConnected() const` | Is the client connected to a server?
`public bool IsDisconnected() const` | Is the client in a disconnected state?
`public bool ConnectionFailed() const` | Is the client in an error state?
`public `[`ClientState`](#namespaceyojimbo_1ab646aeedaf6bd6841c1e7fd9917a47f8)` GetClientState() const` | Get the current client state.
`public double GetTime() const` | Gets the current client time.
`public uint64_t GetClientId() const` | Get the client id (globally unique).
`public int GetClientIndex() const` | Get the client index.
`public uint64_t GetCounter(int index) const` | Get a counter value.
`public `[`Message`](#classyojimbo_1_1_message)` * CreateMsg(int type)` | Create a message of the specified type.
`public bool CanSendMsg(int channelId)` | Check if there is room in the channel send queue to send one message.
`public void SendMsg(`[`Message`](#classyojimbo_1_1_message)` * message,int channelId)` | [Queue](#classyojimbo_1_1_queue) a message to be sent to the server.
`public `[`Message`](#classyojimbo_1_1_message)` * ReceiveMsg(int channelId)` | Poll this method to receive messages from the server.
`public void ReleaseMsg(`[`Message`](#classyojimbo_1_1_message)` * message)` | Release a message returned by [Client::ReceiveMsg](#classyojimbo_1_1_client_1a4a95a108142666064ecdac692a33de43).
`public `[`MessageFactory`](#classyojimbo_1_1_message_factory)` & GetMsgFactory()` | Get the message factory used by the client.
`public `[`Allocator`](#classyojimbo_1_1_allocator)` & GetClientAllocator()` | Get the allocator used for client allocations.
`protected `[`ClientServerConfig`](#structyojimbo_1_1_client_server_config)` m_config` | The client/server configuration.
`protected `[`Allocator`](#classyojimbo_1_1_allocator)` * m_allocator` | The allocator passed in to the client on creation.
`protected void * m_userContext` | The user context. Lets the user pass information to packet serialize functions.
`protected uint8_t * m_clientMemory` | Memory backing the client allocator. Allocated from m_allocator.
`protected `[`Allocator`](#classyojimbo_1_1_allocator)` * m_clientAllocator` | The client allocator. Everything allocated between [Client::Connect](#classyojimbo_1_1_client_1a005a49077acaddff7fd29ea26fe07687) and [Client::Disconnect](#classyojimbo_1_1_client_1a9eb150732daeadb0be17e1b16f3b7658) is allocated and freed via this allocator.
`protected `[`PacketFactory`](#classyojimbo_1_1_packet_factory)` * m_packetFactory` | [Packet](#classyojimbo_1_1_packet) factory for creating and destroying messages. Created via Client::CreatePacketFactory.
`protected `[`MessageFactory`](#classyojimbo_1_1_message_factory)` * m_messageFactory` | [Message](#classyojimbo_1_1_message) factory for creating and destroying messages. Created via Client::CreateMessageFactory. Optional.
`protected `[`ReplayProtection`](#classyojimbo_1_1_replay_protection)` * m_replayProtection` | Replay protection discards old and duplicate packets. Protects against packet replay attacks.
`protected bool m_allocateConnection` | True if we should allocate a connection object. This is true when [ClientServerConfig::enableMessages](#structyojimbo_1_1_client_server_config_1a6e1a7fd033a718b0a0d08e4b494585bf) is true.
`protected `[`Connection`](#classyojimbo_1_1_connection)` * m_connection` | The connection object for exchanging messages with the server. Optional.
`protected uint64_t m_clientId` | The globally unique client id (set on each call to connect).
`protected int m_clientIndex` | The client slot index on the server [0,maxClients-1]. -1 if not connected.
`protected `[`TransportContext`](#structyojimbo_1_1_transport_context)` m_transportContext` | [Transport](#classyojimbo_1_1_transport) context for reading and writing packets.
`protected `[`ConnectionContext`](#structyojimbo_1_1_connection_context)` m_connectionContext` | [Connection](#classyojimbo_1_1_connection) context for serializing connection packets.
`protected `[`ClientState`](#namespaceyojimbo_1ab646aeedaf6bd6841c1e7fd9917a47f8)` m_clientState` | The current client state.
`protected uint64_t m_connectTokenExpireTimestamp` | Expire timestamp for connect token used in secure connect. This is used as the additional data in the connect token AEAD, so we can quickly reject stale connect tokens without decrypting them.
`protected int m_serverAddressIndex` | Current index in the server address array. This is the server we are currently connecting to.
`protected int m_numServerAddresses` | Number of server addresses in the array.
`protected `[`Address`](#classyojimbo_1_1_address)` m_serverAddresses` | List of server addresses we are connecting to.
`protected `[`Address`](#classyojimbo_1_1_address)` m_serverAddress` | The current server address we are connecting/connected to.
`protected double m_lastPacketSendTime` | The last time we sent a packet to the server.
`protected double m_lastPacketReceiveTime` | The last time we received a packet from the server.
`protected `[`Transport`](#classyojimbo_1_1_transport)` * m_transport` | The transport for sending and receiving packets.
`protected bool m_shouldDisconnect` | Set to true when we receive a disconnect packet from the server so we can defer disconnection until the update.
`protected `[`ClientState`](#namespaceyojimbo_1ab646aeedaf6bd6841c1e7fd9917a47f8)` m_shouldDisconnectState` | The client state to transition to on disconnect via m_shouldDisconnect.
`protected double m_time` | The current client time. See [Client::AdvanceTime](#classyojimbo_1_1_client_1a1f7c779a9398a21da6ee554c90be64ec).
`protected uint64_t m_clientSalt` | The client salt used for insecure connect. This distinguishes the current insecure client session from a previous one from the same IP:port combination, making insecure reconnects much more robust.
`protected uint64_t m_sequence` | Sequence # for packets sent from client to server.
`protected uint64_t m_counters` | Counters to aid with debugging and telemetry.
`protected uint8_t m_connectTokenData` | Encrypted connect token data for the connection request packet.
`protected uint8_t m_connectTokenNonce` | Nonce to send to server so it can decrypt the connect token.
`protected uint8_t m_challengeTokenData` | Encrypted challenge token data for challenge response packet.
`protected uint8_t m_challengeTokenNonce` | Nonce to send to server so it can decrypt the challenge token.
`protected uint8_t m_clientToServerKey` | [Client](#classyojimbo_1_1_client) to server packet encryption key.
`protected uint8_t m_serverToClientKey` | [Server](#classyojimbo_1_1_server) to client packet encryption key.
`protected `[`Packet`](#classyojimbo_1_1_packet)` * CreatePacket(int type)` | Helper function to create a packet by type.
`protected inline virtual void OnConnect(const `[`Address`](#classyojimbo_1_1_address)` & address)` | Override this method to get a callback when the client starts to connect to a server.
`protected inline virtual void OnClientStateChange(`[`ClientState`](#namespaceyojimbo_1ab646aeedaf6bd6841c1e7fd9917a47f8)` previousState,`[`ClientState`](#namespaceyojimbo_1ab646aeedaf6bd6841c1e7fd9917a47f8)` currentState)` | Override this method to get a callback when the client state changes.
`protected inline virtual void OnDisconnect()` | Override this method to get a callback when the client disconnects from the server.
`protected inline virtual void OnPacketSent(int packetType,const `[`Address`](#classyojimbo_1_1_address)` & to,bool immediate)` | Override this method to get a callback when the client sends a packet.
`protected inline virtual void OnPacketReceived(int packetType,const `[`Address`](#classyojimbo_1_1_address)` & from)` | Override this method to get a callback when the client receives a packet.
`protected inline virtual void OnConnectionPacketGenerated(`[`Connection`](#classyojimbo_1_1_connection)` * connection,uint16_t sequence)` | Override this method to get a callback when a connection packet is generated and sent to the server.
`protected inline virtual void OnConnectionPacketAcked(`[`Connection`](#classyojimbo_1_1_connection)` * connection,uint16_t sequence)` | Override this method to get a callback when a connection packet is acked by the client (eg.
`protected inline virtual void OnConnectionPacketReceived(`[`Connection`](#classyojimbo_1_1_connection)` * connection,uint16_t sequence)` | Override this method to get a callback when a connection packet is received from the server.
`protected inline virtual void OnConnectionFragmentReceived(`[`Connection`](#classyojimbo_1_1_connection)` * connection,int channelId,uint16_t messageId,uint16_t fragmentId,int fragmentBytes,int numFragmentsReceived,int numFragmentsInBlock)` | Override this method to get a callback when a block fragment is received from the server.
`protected inline virtual bool ProcessUserPacket(`[`Packet`](#classyojimbo_1_1_packet)` * packet)` | Override this method to process user packets sent from the server.
`protected virtual void InitializeConnection(uint64_t clientId)` | 
`protected virtual void ShutdownConnection()` | 
`protected virtual void SetEncryptedPacketTypes()` | 
`protected virtual `[`PacketFactory`](#classyojimbo_1_1_packet_factory)` * CreatePacketFactory(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator)` | 
`protected virtual `[`MessageFactory`](#classyojimbo_1_1_message_factory)` * CreateMessageFactory(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator)` | 
`protected virtual void SetClientState(`[`ClientState`](#namespaceyojimbo_1ab646aeedaf6bd6841c1e7fd9917a47f8)` clientState)` | 
`protected virtual void ResetConnectionData(`[`ClientState`](#namespaceyojimbo_1ab646aeedaf6bd6841c1e7fd9917a47f8)` clientState)` | 
`protected virtual void ResetBeforeNextConnect()` | 
`protected virtual bool ConnectToNextServer()` | 
`protected virtual void InternalInsecureConnect(const `[`Address`](#classyojimbo_1_1_address)` & serverAddress)` | 
`protected virtual void InternalSecureConnect(const `[`Address`](#classyojimbo_1_1_address)` & serverAddress)` | 
`protected virtual void SendPacketToServer(`[`Packet`](#classyojimbo_1_1_packet)` * packet)` | 
`protected void ProcessConnectionDenied(const `[`ConnectionDeniedPacket`](#structyojimbo_1_1_connection_denied_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address)` | 
`protected void ProcessChallenge(const `[`ChallengePacket`](#structyojimbo_1_1_challenge_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address)` | 
`protected void ProcessKeepAlive(const `[`KeepAlivePacket`](#structyojimbo_1_1_keep_alive_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address)` | 
`protected void ProcessDisconnect(const `[`DisconnectPacket`](#structyojimbo_1_1_disconnect_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address)` | 
`protected void ProcessConnectionPacket(`[`ConnectionPacket`](#structyojimbo_1_1_connection_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address)` | 
`protected void ProcessPacket(`[`Packet`](#classyojimbo_1_1_packet)` * packet,const `[`Address`](#classyojimbo_1_1_address)` & address,uint64_t sequence)` | 
`protected bool IsPendingConnect()` | 
`protected void CompletePendingConnect(int clientIndex)` | 
`protected void Defaults()` | 
`protected virtual void CreateAllocators()` | 
`protected virtual void DestroyAllocators()` | 
`protected virtual `[`Allocator`](#classyojimbo_1_1_allocator)` * CreateAllocator(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,void * memory,size_t bytes)` | 

## Members

#### `public  explicit Client(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,`[`Transport`](#classyojimbo_1_1_transport)` & transport,const `[`ClientServerConfig`](#structyojimbo_1_1_client_server_config)` & config,double time)` 

The client constructor.

#### Parameters
* `allocator` The allocator for all memory used by the client. 


* `transport` The transport for sending and receiving packets. 


* `config` The client/server configuration. 


* `time` The current time in seconds. See [Client::AdvanceTime](#classyojimbo_1_1_client_1a1f7c779a9398a21da6ee554c90be64ec)

#### `public virtual  ~Client()` 

The client destructor.

IMPORTANT: Please call [Client::Disconnect](#classyojimbo_1_1_client_1a9eb150732daeadb0be17e1b16f3b7658) before destroying the client. This is necessary because Disconnect is virtual and calling virtual methods from destructors does not give the expected behavior when you override that method.

#### `public void SetUserContext(void * context)` 

Set the user context.

The user context is set on the stream when packets and read and written. It lets you pass in a pointer to some structure that you want to have available when reading and writing packets.

Typical use case is to pass in an array of min/max ranges for values determined by some data that is loaded from a toolchain vs. being known at compile time.

If you do use a user context, please make sure the data that contributes to the user context is checksummed and included in the protocol id, so clients and servers with incompatible data can't connect to each other.


**See also**: Stream::GetUserContext

#### `public void InsecureConnect(uint64_t clientId,const `[`Address`](#classyojimbo_1_1_address)` & serverAddress)` 

Connect to a server (insecure).

IMPORTANT: Insecure connections are not encrypted and do not provide authentication.

They are provided for convienence in development only, and should not be used in production code!

You can completely disable insecure connections in your retail build by defining YOJIMBO_SECURE_MODE 1


#### Parameters
* `clientId` A globally unique client id used to identify clients when your server talks to your web backend. If you don't have a concept of client id yet, roll a random 64bit integer. 


* `serverAddress` The address of the server to connect to.

#### `public void InsecureConnect(uint64_t clientId,const `[`Address`](#classyojimbo_1_1_address)` serverAddresses,int numServerAddresses)` 

Connect to a list of servers (insecure).

The client tries to connect to each server in the list, in turn, until one of the servers is connected to, or it reaches the end of the server address list.

IMPORTANT: Insecure connections are not encrypted and do not provide authentication.

They are provided for convienence in development only, and should not be used in production code!

You can completely disable insecure connections in your retail build by defining YOJIMBO_SECURE_MODE 1


#### Parameters
* `clientId` A globally unique client id used to identify clients when your server talks to your web backend. If you don't have a concept of client id yet, roll a random 64bit integer. 


* `serverAddresses` The list of server addresses to connect to, in order of first to last. 


* `numServerAddresses` Number of server addresses in [1,[yojimbo::MaxServersPerConnect](#namespaceyojimbo_1a63bfd0da7001020a05dc6b39dd1f8ec3)].

#### `public void Connect(uint64_t clientId,const `[`Address`](#classyojimbo_1_1_address)` & serverAddress,const uint8_t * connectTokenData,const uint8_t * connectTokenNonce,const uint8_t * clientToServerKey,const uint8_t * serverToClientKey,uint64_t connectTokenExpireTimestamp)` 

Connect to a server (secure).

This function takes a connect token generated by matcher.go and passes it to the server to establish a secure connection.

Secure connections are encrypted and authenticated. If the server runs in secure mode, it will only accept connections from clients with a connect token, thus stopping unauthenticated clients from connecting to your server.


#### Parameters
* `clientId` A globally unique client id used to identify clients when your server talks to your web backend. If you don't have a concept of client id yet, roll a random 64bit integer. 


* `serverAddress` The address of the server to connect to. 


* `connectTokenData` Pointer to the connect token data from the matcher. 


* `connectTokenNonce` Pointer to the connect token nonce from the matcher. 


* `clientToServerKey` The encryption key for client to server packets. 


* `serverToClientKey` The encryption key for server to client packets. 


* `connectTokenExpireTimestamp` The timestamp for when the connect token expires. Used by the server to quickly reject stale connect tokens without decrypting them.

#### `public void Connect(uint64_t clientId,const `[`Address`](#classyojimbo_1_1_address)` serverAddresses,int numServerAddresses,const uint8_t * connectTokenData,const uint8_t * connectTokenNonce,const uint8_t * clientToServerKey,const uint8_t * serverToClientKey,uint64_t connectTokenExpireTimestamp)` 

Connect to a list of servers (secure).

The client tries to connect to each server in the list, in turn, until one of the servers is connected to, or it reaches the end of the server address list.

This is designed for use with the matcher, which can return a list of up to [yojimbo::MaxServersPerConnect](#namespaceyojimbo_1a63bfd0da7001020a05dc6b39dd1f8ec3) with the connect token, to work around race conditions where the server sends multiple clients to fill the same slot on a server.

Secure connections are encrypted and authenticated. If the server runs in secure mode, it will only accept connections from clients with a connect token, thus stopping unauthenticated from connecting to your server.


#### Parameters
* `clientId` A globally unique client id used to identify clients when your server talks to your web backend. If you don't have a concept of client id yet, roll a random 64bit integer. 


* `serverAddresses` The list of server addresses to connect to, in order of first to last. 


* `numServerAddresses` Number of server addresses in [1,[yojimbo::MaxServersPerConnect](#namespaceyojimbo_1a63bfd0da7001020a05dc6b39dd1f8ec3)]. 


* `connectTokenData` Pointer to the connect token data from the matcher. 


* `connectTokenNonce` Pointer to the connect token nonce from the matcher. 


* `clientToServerKey` The encryption key for client to server packets. 


* `serverToClientKey` The encryption key for server to client packets. 


* `connectTokenExpireTimestamp` The timestamp for when the connect token expires. Used by the server to quickly reject stale connect tokens without decrypting them.

#### `public void Disconnect(`[`ClientState`](#namespaceyojimbo_1ab646aeedaf6bd6841c1e7fd9917a47f8)` clientState,bool sendDisconnectPacket)` 

Disconnect from the server.

This function is safe to call if not currently connected. In that case it will do nothing.


#### Parameters
* `clientState` The client state to transition to on disconnect. By default CLIENT_STATE_DISCONNECTED, but you may also specify one of the negative client state values for error states. 


* `sendDisconnectPacket` If true then disconnect packets will be sent immediately to the server to notify it that the client disconnected. This makes the server client slot get cleaned up and recycled faster for other clients to connect, vs. timing out in 5-10 seconds. The only situation where this should be false is if the client is disconnecting because it timed out from the server.

#### `public void SendPackets()` 

Send packets to server.

This function drives the sending of packets to the server such as connection request, challenge response, keep-alive packet and connection packets to transmit messages from client to server.

Packets sent from this function are queued for sending on the transport, and are not actually serialized and sent to the network until you call [Transport::WritePackets](#classyojimbo_1_1_transport_1a903705df34df07acd44eccf3c6ccacff).

#### `public void ReceivePackets()` 

Receive packets sent from the server.

This function receives and processes packets from the receive queue of the [Transport](#classyojimbo_1_1_transport). To minimize the latency of packet processing, make sure you call [Transport::ReadPackets](#classyojimbo_1_1_transport_1abbd058ddc3242bcfced01954c3d78dfa) shortly before calling this function.


**See also**: ProcessPackets

#### `public void CheckForTimeOut()` 

Check for timeouts.

Compares the last time a packet was received from the server vs. the current time.

If no packet has been received within the timeout period, the client is disconnected and set to an error state. See ClientState for details.


**See also**: [Client::AdvanceTime](#classyojimbo_1_1_client_1a1f7c779a9398a21da6ee554c90be64ec)


**See also**: [ClientServerConfig::connectionTimeOut](#structyojimbo_1_1_client_server_config_1a71c455645f804ab75499e26ec728eb9c)


**See also**: [ClientServerConfig::connectionNegotiationTimeOut](#structyojimbo_1_1_client_server_config_1a30d025c12f2eac6c5b3ea9c069d86148)

#### `public void AdvanceTime(double time)` 

Advance client time.

Call this at the end of each frame to advance the client time forward.

IMPORTANT: Please use a double for your time value so it maintains sufficient accuracy as time increases.

#### `public bool IsConnecting() const` 

Is the client connecting to a server?

This is true while the client is negotiation connection with a server. This is the period after a call to [Client::Connect](#classyojimbo_1_1_client_1a005a49077acaddff7fd29ea26fe07687) or [Client::InsecureConnect](#classyojimbo_1_1_client_1acf884a58a6b98aef48d7063c6181a622), but before the client establishes a connection, or goes into an error state because it couldn't connect.


#### Returns
true if the client is currently connecting to, but is not yet connected to a server.

#### `public bool IsConnected() const` 

Is the client connected to a server?

This is true once a client successfully finishes connection negotiation, and connects to a server. It is false while connecting to a server.

Corresponds to the client being in CLIENT_STATE_CONNECTED.


#### Returns
true if the client is connected to a server.

#### `public bool IsDisconnected() const` 

Is the client in a disconnected state?

A disconnected state is CLIENT_STATE_DISCONNECTED, or any of the negative client error state values. Effectively, true if client state is <= 0.


#### Returns
true if the client is in a disconnected state.

#### `public bool ConnectionFailed() const` 

Is the client in an error state?

When the client disconnects because of a client-side error, it disconnects and sets one of the negative client state values. Effectively, true if client state < 0.


#### Returns
true if the client disconnected to an error state.

#### `public `[`ClientState`](#namespaceyojimbo_1ab646aeedaf6bd6841c1e7fd9917a47f8)` GetClientState() const` 

Get the current client state.

The client state machine is used to negotiate connection with the server, and handle error states. Each state corresponds to an entry in the ClientState enum.


**See also**: [yojimbo::GetClientStateName](#namespaceyojimbo_1a2a1f66339311e929369fa43cb29f3044)

#### `public double GetTime() const` 

Gets the current client time.

**See also**: [Client::AdvanceTime](#classyojimbo_1_1_client_1a1f7c779a9398a21da6ee554c90be64ec)

#### `public uint64_t GetClientId() const` 

Get the client id (globally unique).

This corresponds to the client id parameter passed into the last call to [Client::Connect](#classyojimbo_1_1_client_1a005a49077acaddff7fd29ea26fe07687) or [Client::InsecureConnect](#classyojimbo_1_1_client_1acf884a58a6b98aef48d7063c6181a622)


#### Returns
The globally unique client id for this client.

#### `public int GetClientIndex() const` 

Get the client index.

The client index is the slot number that the client is occupying on the server.


#### Returns
The client index in [0,maxClients-1], where maxClients is the number of client slots allocated on the server in [Server::Start](#classyojimbo_1_1_server_1ac79505b75a954a3f5cbab63a4aaf3436).

#### `public uint64_t GetCounter(int index) const` 

Get a counter value.

Counters are used to track event and actions performed by the client. They are useful for debugging, testing and telemetry.


#### Returns
The counter value. See [yojimbo::ClientCounters](#namespaceyojimbo_1a33643f34871b4b3ce3b0bc03338f9e17) for the set of client counters.

#### `public `[`Message`](#classyojimbo_1_1_message)` * CreateMsg(int type)` 

Create a message of the specified type.

The message created by this function is typically passed to [Client::SendMsg](#classyojimbo_1_1_client_1a64eaefcf7e8e74e76ac62111d79a6816). In this case, the send message function takes ownership of the message pointer and will release it for you.

If you are using the message in some other way, you are responsible for manually releasing it via [Client::ReleaseMsg](#classyojimbo_1_1_client_1aa0a85e0c292a7b0d75056a9df9bd10a1).


#### Parameters
* `type` The message type. The set of message types depends on the message factory set on the client.





#### Returns
A pointer to the message created, or NULL if no message could be created.


**See also**: [MessageFactory](#classyojimbo_1_1_message_factory)

#### `public bool CanSendMsg(int channelId)` 

Check if there is room in the channel send queue to send one message.

This function is useful in soak tests and unit tests where I want to send messages as quickly as possible, but don't want to overflow the send queue.

You don't need to call this function manually each time you call [Client::SendMsg](#classyojimbo_1_1_client_1a64eaefcf7e8e74e76ac62111d79a6816). It's already asserted on in debug build and in release if it returns false it sets a runtime error that disconnects the client.


#### Parameters
* `channelId` The id of the channel in [0,numChannels-1].





#### Returns
True if the channel has room for one more message to be added to its send queue. False otherwise.

#### `public void SendMsg(`[`Message`](#classyojimbo_1_1_message)` * message,int channelId)` 

[Queue](#classyojimbo_1_1_queue) a message to be sent to the server.

Adds a message to the send queue of the specified channel.

The reliability and ordering guarantees of how the message will be received on the other side are determined by the configuration of the channel.

IMPORTANT: This function takes ownership of the message and ensures that the message is released when it finished being sent. This lets you create a message with [Client::CreateMsg](#classyojimbo_1_1_client_1a18acb7318e53d6f473d221796f58532f) and pass it directly into this function. You don't need to manually release the message.


#### Parameters
* `message` The message to be sent. It must be allocated from the message factory set on this client. 


* `channelId` The id of the channel to send the message across in [0,numChannels-1].





**See also**: [ChannelConfig](#structyojimbo_1_1_channel_config)


**See also**: [ClientServerConfig](#structyojimbo_1_1_client_server_config)

#### `public `[`Message`](#classyojimbo_1_1_message)` * ReceiveMsg(int channelId)` 

Poll this method to receive messages from the server.

Typical usage is to iterate across the set of channels and poll this to receive messages until it returns NULL.

IMPORTANT: The message returned by this function has one reference. You are responsible for releasing this message via [Client::ReleaseMsg](#classyojimbo_1_1_client_1aa0a85e0c292a7b0d75056a9df9bd10a1).


#### Parameters
* `channelId` The id of the channel to try to receive a message from.





#### Returns
A pointer to the received message, NULL if there are no messages to receive.

#### `public void ReleaseMsg(`[`Message`](#classyojimbo_1_1_message)` * message)` 

Release a message returned by [Client::ReceiveMsg](#classyojimbo_1_1_client_1a4a95a108142666064ecdac692a33de43).

This is a convenience function. It is equivalent to calling [MessageFactory::Release](#classyojimbo_1_1_message_factory_1a20529940091cb084c7876a4396954f50) on the message factory set on this client (see [Client::GetMsgFactory](#classyojimbo_1_1_client_1af473695a6cbd4922a5da492991ea799a)).


#### Parameters
* `message` The message to release. Must be non-NULL.





**See also**: [Client::ReceiveMsg](#classyojimbo_1_1_client_1a4a95a108142666064ecdac692a33de43)

#### `public `[`MessageFactory`](#classyojimbo_1_1_message_factory)` & GetMsgFactory()` 

Get the message factory used by the client.

The message factory determines the set of messages exchanged between the client and server.


#### Returns
The message factory.


**See also**: [YOJIMBO_CLIENT_MESSAGE_FACTORY](#yojimbo__client_8h_1ab26aa44bab0ab2cc1b8f76274ef2bbfa)

#### `public `[`Allocator`](#classyojimbo_1_1_allocator)` & GetClientAllocator()` 

Get the allocator used for client allocations.

This memory is used for packet, message allocations and stream allocations while the client is connecting/connected to the server.

The amount of memory backing this allocator is specified by [ClientServerConfig::clientMemory](#structyojimbo_1_1_client_server_config_1aa7df137b1177c2bd010930050020fa8e).


#### Returns
The allocator client allocator.

#### `protected `[`ClientServerConfig`](#structyojimbo_1_1_client_server_config)` m_config` 

The client/server configuration.



#### `protected `[`Allocator`](#classyojimbo_1_1_allocator)` * m_allocator` 

The allocator passed in to the client on creation.



#### `protected void * m_userContext` 

The user context. Lets the user pass information to packet serialize functions.



#### `protected uint8_t * m_clientMemory` 

Memory backing the client allocator. Allocated from m_allocator.



#### `protected `[`Allocator`](#classyojimbo_1_1_allocator)` * m_clientAllocator` 

The client allocator. Everything allocated between [Client::Connect](#classyojimbo_1_1_client_1a005a49077acaddff7fd29ea26fe07687) and [Client::Disconnect](#classyojimbo_1_1_client_1a9eb150732daeadb0be17e1b16f3b7658) is allocated and freed via this allocator.



#### `protected `[`PacketFactory`](#classyojimbo_1_1_packet_factory)` * m_packetFactory` 

[Packet](#classyojimbo_1_1_packet) factory for creating and destroying messages. Created via Client::CreatePacketFactory.



#### `protected `[`MessageFactory`](#classyojimbo_1_1_message_factory)` * m_messageFactory` 

[Message](#classyojimbo_1_1_message) factory for creating and destroying messages. Created via Client::CreateMessageFactory. Optional.



#### `protected `[`ReplayProtection`](#classyojimbo_1_1_replay_protection)` * m_replayProtection` 

Replay protection discards old and duplicate packets. Protects against packet replay attacks.



#### `protected bool m_allocateConnection` 

True if we should allocate a connection object. This is true when [ClientServerConfig::enableMessages](#structyojimbo_1_1_client_server_config_1a6e1a7fd033a718b0a0d08e4b494585bf) is true.



#### `protected `[`Connection`](#classyojimbo_1_1_connection)` * m_connection` 

The connection object for exchanging messages with the server. Optional.



#### `protected uint64_t m_clientId` 

The globally unique client id (set on each call to connect).



#### `protected int m_clientIndex` 

The client slot index on the server [0,maxClients-1]. -1 if not connected.



#### `protected `[`TransportContext`](#structyojimbo_1_1_transport_context)` m_transportContext` 

[Transport](#classyojimbo_1_1_transport) context for reading and writing packets.



#### `protected `[`ConnectionContext`](#structyojimbo_1_1_connection_context)` m_connectionContext` 

[Connection](#classyojimbo_1_1_connection) context for serializing connection packets.



#### `protected `[`ClientState`](#namespaceyojimbo_1ab646aeedaf6bd6841c1e7fd9917a47f8)` m_clientState` 

The current client state.



#### `protected uint64_t m_connectTokenExpireTimestamp` 

Expire timestamp for connect token used in secure connect. This is used as the additional data in the connect token AEAD, so we can quickly reject stale connect tokens without decrypting them.



#### `protected int m_serverAddressIndex` 

Current index in the server address array. This is the server we are currently connecting to.



#### `protected int m_numServerAddresses` 

Number of server addresses in the array.



#### `protected `[`Address`](#classyojimbo_1_1_address)` m_serverAddresses` 

List of server addresses we are connecting to.



#### `protected `[`Address`](#classyojimbo_1_1_address)` m_serverAddress` 

The current server address we are connecting/connected to.



#### `protected double m_lastPacketSendTime` 

The last time we sent a packet to the server.



#### `protected double m_lastPacketReceiveTime` 

The last time we received a packet from the server.



#### `protected `[`Transport`](#classyojimbo_1_1_transport)` * m_transport` 

The transport for sending and receiving packets.



#### `protected bool m_shouldDisconnect` 

Set to true when we receive a disconnect packet from the server so we can defer disconnection until the update.



#### `protected `[`ClientState`](#namespaceyojimbo_1ab646aeedaf6bd6841c1e7fd9917a47f8)` m_shouldDisconnectState` 

The client state to transition to on disconnect via m_shouldDisconnect.



#### `protected double m_time` 

The current client time. See [Client::AdvanceTime](#classyojimbo_1_1_client_1a1f7c779a9398a21da6ee554c90be64ec).



#### `protected uint64_t m_clientSalt` 

The client salt used for insecure connect. This distinguishes the current insecure client session from a previous one from the same IP:port combination, making insecure reconnects much more robust.



#### `protected uint64_t m_sequence` 

Sequence # for packets sent from client to server.



#### `protected uint64_t m_counters` 

Counters to aid with debugging and telemetry.



#### `protected uint8_t m_connectTokenData` 

Encrypted connect token data for the connection request packet.



#### `protected uint8_t m_connectTokenNonce` 

Nonce to send to server so it can decrypt the connect token.



#### `protected uint8_t m_challengeTokenData` 

Encrypted challenge token data for challenge response packet.



#### `protected uint8_t m_challengeTokenNonce` 

Nonce to send to server so it can decrypt the challenge token.



#### `protected uint8_t m_clientToServerKey` 

[Client](#classyojimbo_1_1_client) to server packet encryption key.



#### `protected uint8_t m_serverToClientKey` 

[Server](#classyojimbo_1_1_server) to client packet encryption key.



#### `protected `[`Packet`](#classyojimbo_1_1_packet)` * CreatePacket(int type)` 

Helper function to create a packet by type.

Just a shortcut to [PacketFactory::CreatePacket](#classyojimbo_1_1_packet_factory_1a11b97e868075ee097220d641545fa25e) for convenience.


#### Parameters
* `type` The type of packet to create.





#### Returns
The packet object that was created. NULL if a packet could be created. You *must* check this. It *will* happen when the packet factory runs out of memory to allocate packets!

#### `protected inline virtual void OnConnect(const `[`Address`](#classyojimbo_1_1_address)` & address)` 

Override this method to get a callback when the client starts to connect to a server.

#### Parameters
* `address` The address of the server that is being connected to.

#### `protected inline virtual void OnClientStateChange(`[`ClientState`](#namespaceyojimbo_1ab646aeedaf6bd6841c1e7fd9917a47f8)` previousState,`[`ClientState`](#namespaceyojimbo_1ab646aeedaf6bd6841c1e7fd9917a47f8)` currentState)` 

Override this method to get a callback when the client state changes.

The previous and current state are guaranteed to be different, otherwise this callback is not called.

IMPORTANT: This callback is called at the end of the connection start process, so all client server data is valid, such as the server address and client state at the point this callback is executed.


#### Parameters
* `previousState` The previous client state. 


* `currentState` The current client state that was just transitioned to. 





**See also**: [yojimbo::GetClientStateName](#namespaceyojimbo_1a2a1f66339311e929369fa43cb29f3044)

#### `protected inline virtual void OnDisconnect()` 

Override this method to get a callback when the client disconnects from the server.

IMPORTANT: This callback is executed before the client disconnect, so all details and states regarding the client connection are still valid (eg. GetServerAddress) and so on.

If you want more detail about why the client is disconnecting, override [Client::OnClientStateChange](#classyojimbo_1_1_client_1a416f36dd35c7d4b71448bd122c1dce86) and follow state transitions that way.

#### `protected inline virtual void OnPacketSent(int packetType,const `[`Address`](#classyojimbo_1_1_address)` & to,bool immediate)` 

Override this method to get a callback when the client sends a packet.

#### Parameters
* `packetType` The type of packet being sent, according to the client packet factory. See [PacketFactory](#classyojimbo_1_1_packet_factory). 


* `to` The address the packet is being sent to. 


* `immediate` True if the packet is to be flushed to the network and sent immediately.





**See also**: [Client::SendPackets](#classyojimbo_1_1_client_1af941386f5a53dabd0914c76aa8e2a3d1)

#### `protected inline virtual void OnPacketReceived(int packetType,const `[`Address`](#classyojimbo_1_1_address)` & from)` 

Override this method to get a callback when the client receives a packet.

#### Parameters
* `packetType` The type of packet being sent, according to the client packet factory. See [PacketFactory](#classyojimbo_1_1_packet_factory). 


* `from` The address of the machine that sent the packet.





**See also**: [Client::ReceivePackets](#classyojimbo_1_1_client_1aa9a4655f3dd6dbe9df866e6189eb14f5)

#### `protected inline virtual void OnConnectionPacketGenerated(`[`Connection`](#classyojimbo_1_1_connection)` * connection,uint16_t sequence)` 

Override this method to get a callback when a connection packet is generated and sent to the server.

[Connection](#classyojimbo_1_1_connection) packets are carrier packets that transmit messages between the client and server. They are only generated if you enabled messages in the [ClientServerConfig](#structyojimbo_1_1_client_server_config) (true by default).


#### Parameters
* `connection` The connection that the packet belongs to. There is just one connection object on the client-side. 


* `sequence` The sequence number of the connection packet being sent.





**See also**: [ClientServerConfig::enableMessages](#structyojimbo_1_1_client_server_config_1a6e1a7fd033a718b0a0d08e4b494585bf)

#### `protected inline virtual void OnConnectionPacketAcked(`[`Connection`](#classyojimbo_1_1_connection)` * connection,uint16_t sequence)` 

Override this method to get a callback when a connection packet is acked by the client (eg.

the client notified the server that packet was received).

[Connection](#classyojimbo_1_1_connection) packets are carrier packets that transmit messages between the client and server. They are automatically generated when messages are enabled in [ClientServerConfig](#structyojimbo_1_1_client_server_config) (true by default).


#### Parameters
* `connection` The connection that the packet belongs to. There is just one connection object on the client-side. 


* `sequence` The packet sequence number of the connection packet that was acked by the server.





**See also**: [ClientServerConfig::enableMessages](#structyojimbo_1_1_client_server_config_1a6e1a7fd033a718b0a0d08e4b494585bf)

#### `protected inline virtual void OnConnectionPacketReceived(`[`Connection`](#classyojimbo_1_1_connection)` * connection,uint16_t sequence)` 

Override this method to get a callback when a connection packet is received from the server.

[Connection](#classyojimbo_1_1_connection) packets are carrier packets that transmit messages between the client and server. They are automatically generated when messages are enabled in [ClientServerConfig](#structyojimbo_1_1_client_server_config) (true by default).


#### Parameters
* `connection` The connection that the packet belongs to. There is just one connection object on the client-side. 


* `sequence` The sequence number of the connection packet that was received.





**See also**: [ClientServerConfig::enableMessages](#structyojimbo_1_1_client_server_config_1a6e1a7fd033a718b0a0d08e4b494585bf)

#### `protected inline virtual void OnConnectionFragmentReceived(`[`Connection`](#classyojimbo_1_1_connection)` * connection,int channelId,uint16_t messageId,uint16_t fragmentId,int fragmentBytes,int numFragmentsReceived,int numFragmentsInBlock)` 

Override this method to get a callback when a block fragment is received from the server.

This callback lets you implement a progress bar for large block transmissions.


#### Parameters
* `connection` The connection that the block fragment belongs to. There is just one connection object on the client-side. 


* `channelId` The channel the block is being sent over. 


* `messageId` The message id the block is attached to. 


* `fragmentId` The fragment id that is being processed. Fragment ids are in the range [0,numFragments-1]. 


* `fragmentBytes` The size of the fragment in bytes. 


* `numFragmentsReceived` The number of fragments received for this block so far (including this one). 


* `numFragmentsInBlock` The total number of fragments in this block. The block receive completes when all fragments are received.





**See also**: [BlockMessage::AttachBlock](#classyojimbo_1_1_block_message_1af88bdd0b14d6ff6d73144dddb6581563)

#### `protected inline virtual bool ProcessUserPacket(`[`Packet`](#classyojimbo_1_1_packet)` * packet)` 

Override this method to process user packets sent from the server.

User packets let you extend the yojimbo by adding your own packet types to be exchanged between client and server. See [PacketFactory](#classyojimbo_1_1_packet_factory).

Most users won't need to create custom packet types, and will extend the protocol by defining their own message types instead. See [MessageFactory](#classyojimbo_1_1_message_factory).


#### Parameters
* `packet` The user packet received from the server.





#### Returns
Return true if the user packet was processed successfully. Returning false if the packet could not be processed, or if is of a type you don't expect. This ensures that unexpected packet types don't keep the connection alive when it should time out. out.

#### `protected virtual void InitializeConnection(uint64_t clientId)` 





#### `protected virtual void ShutdownConnection()` 





#### `protected virtual void SetEncryptedPacketTypes()` 





#### `protected virtual `[`PacketFactory`](#classyojimbo_1_1_packet_factory)` * CreatePacketFactory(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator)` 





#### `protected virtual `[`MessageFactory`](#classyojimbo_1_1_message_factory)` * CreateMessageFactory(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator)` 





#### `protected virtual void SetClientState(`[`ClientState`](#namespaceyojimbo_1ab646aeedaf6bd6841c1e7fd9917a47f8)` clientState)` 





#### `protected virtual void ResetConnectionData(`[`ClientState`](#namespaceyojimbo_1ab646aeedaf6bd6841c1e7fd9917a47f8)` clientState)` 





#### `protected virtual void ResetBeforeNextConnect()` 





#### `protected virtual bool ConnectToNextServer()` 





#### `protected virtual void InternalInsecureConnect(const `[`Address`](#classyojimbo_1_1_address)` & serverAddress)` 





#### `protected virtual void InternalSecureConnect(const `[`Address`](#classyojimbo_1_1_address)` & serverAddress)` 





#### `protected virtual void SendPacketToServer(`[`Packet`](#classyojimbo_1_1_packet)` * packet)` 





#### `protected void ProcessConnectionDenied(const `[`ConnectionDeniedPacket`](#structyojimbo_1_1_connection_denied_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address)` 





#### `protected void ProcessChallenge(const `[`ChallengePacket`](#structyojimbo_1_1_challenge_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address)` 





#### `protected void ProcessKeepAlive(const `[`KeepAlivePacket`](#structyojimbo_1_1_keep_alive_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address)` 





#### `protected void ProcessDisconnect(const `[`DisconnectPacket`](#structyojimbo_1_1_disconnect_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address)` 





#### `protected void ProcessConnectionPacket(`[`ConnectionPacket`](#structyojimbo_1_1_connection_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address)` 





#### `protected void ProcessPacket(`[`Packet`](#classyojimbo_1_1_packet)` * packet,const `[`Address`](#classyojimbo_1_1_address)` & address,uint64_t sequence)` 





#### `protected bool IsPendingConnect()` 





#### `protected void CompletePendingConnect(int clientIndex)` 





#### `protected void Defaults()` 





#### `protected virtual void CreateAllocators()` 





#### `protected virtual void DestroyAllocators()` 





#### `protected virtual `[`Allocator`](#classyojimbo_1_1_allocator)` * CreateAllocator(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,void * memory,size_t bytes)` 





# class `yojimbo::Connection` 

```
class yojimbo::Connection
  : public yojimbo::ChannelListener
```  

Implements packet level acks and transmits messages.

The connection class is used internally by client and server to send and receive messages over connection packets.

You don't need to interact with this class, unless you are bypassing client/server entirely and using the low-level parts of libyojimbo directly.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  Connection(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,`[`PacketFactory`](#classyojimbo_1_1_packet_factory)` & packetFactory,`[`MessageFactory`](#classyojimbo_1_1_message_factory)` & messageFactory,const `[`ConnectionConfig`](#structyojimbo_1_1_connection_config)` & connectionConfig)` | The connection constructor.
`public  ~Connection()` | The connection destructor.
`public void Reset()` | Reset the connection.
`public bool CanSendMsg(int channelId) const` | Check if there is room in the send queue to send a message.
`public void SendMsg(`[`Message`](#classyojimbo_1_1_message)` * message,int channelId)` | [Queue](#classyojimbo_1_1_queue) a message to be sent.
`public `[`Message`](#classyojimbo_1_1_message)` * ReceiveMsg(int channelId)` | Poll this method to receive messages.
`public `[`ConnectionPacket`](#structyojimbo_1_1_connection_packet)` * GeneratePacket()` | Generate a connection packet.
`public bool ProcessPacket(`[`ConnectionPacket`](#structyojimbo_1_1_connection_packet)` * packet)` | Process a connection packet.
`public void AdvanceTime(double time)` | Advance connection time.
`public `[`ConnectionError`](#namespaceyojimbo_1aab00f7c22434d6602ba6a53f4c0bb25a)` GetError() const` | Get the connection error level.
`public inline void SetListener(`[`ConnectionListener`](#classyojimbo_1_1_connection_listener)` * listener)` | Set a listener object to receive callbacks (optional).
`public inline void SetClientIndex(int clientIndex)` | Set the client index.
`public inline int GetClientIndex() const` | Get the client index set on the connection.
`public uint64_t GetCounter(int index) const` | Get a counter value.
`protected void InsertAckPacketEntry(uint16_t sequence)` | This is called for each connection packet generated, to mark that connection packet as unacked.
`protected void ProcessAcks(uint16_t ack,uint32_t ack_bits)` | This is the payload function called to process acks in the packet header of the connection packet.
`protected void PacketAcked(uint16_t sequence)` | This method is called when a packet is acked.
`protected virtual void OnPacketAcked(uint16_t sequence)` | 
`protected virtual void OnChannelFragmentReceived(class `[`Channel`](#classyojimbo_1_1_channel)` * channel,uint16_t messageId,uint16_t fragmentId,int fragmentBytes,int numFragmentsReceived,int numFragmentsInBlock)` | Override this method to get a callback when a block fragment is received.

## Members

#### `public  Connection(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,`[`PacketFactory`](#classyojimbo_1_1_packet_factory)` & packetFactory,`[`MessageFactory`](#classyojimbo_1_1_message_factory)` & messageFactory,const `[`ConnectionConfig`](#structyojimbo_1_1_connection_config)` & connectionConfig)` 

The connection constructor.

#### Parameters
* `allocator` The allocator to use. 


* `packetFactory` The packet factory for creating and destroying connection packets. 


* `messageFactory` The message factory to use for creating and destroying messages sent across this connection. 


* `connectionConfig` The connection configuration. Specifies the number and type of channels on the connection, as well as other per-connection and per-channel properties.

#### `public  ~Connection()` 

The connection destructor.



#### `public void Reset()` 

Reset the connection.

Resets message send and receive queues and all properties for a connection, so it may be reused again for some other purpose.

For example, the server calls this function when a client disconnects, so the connection can be reused by the next client that joins in that client slot.

#### `public bool CanSendMsg(int channelId) const` 

Check if there is room in the send queue to send a message.

In reliable-ordered channels, all messages that are sent are guaranteed to arrive, however, if the send queue is full, we have nowhere to buffer the message, so it is lost. This is a fatal error.

Therefore, in debug builds [Connection::SendMsg](#classyojimbo_1_1_connection_1a76d605af9211f49fdd118daed9ba5c16) checks this method and asserts it returns true. In release builds, this function is checked and an error flag is set on the connection if it returns false.

This error flag triggers the automatic disconnection of any client that overflows its message send queue. This is by design so you don't have to do this checking manually.

All you have to do is make sure you have a sufficiently large send queue configured via [ChannelConfig::sendQueueSize](#structyojimbo_1_1_channel_config_1a4d00ebbf1365c13d86d246dafe78f851).


#### Parameters
* `channelId` The id of the channel in [0,numChannels-1]. 





#### Returns
True if the channel has room for one more message in its send queue. False otherwise.


**See also**: [ChannelConfig](#structyojimbo_1_1_channel_config)


**See also**: [ConnectionConfig](#structyojimbo_1_1_connection_config)


**See also**: [Connection::SendMsg](#classyojimbo_1_1_connection_1a76d605af9211f49fdd118daed9ba5c16)

#### `public void SendMsg(`[`Message`](#classyojimbo_1_1_message)` * message,int channelId)` 

[Queue](#classyojimbo_1_1_queue) a message to be sent.

This function adds a message to the send queue of the specified channel.

The reliability and ordering guarantees of how the message will be received on the other side are determined by the configuration of the channel.


#### Parameters
* `message` The message to be sent. It must be allocated from the message factory set on this connection. 


* `channelId` The id of the channel to send the message across in [0,numChannels-1].





**See also**: [Connection::CanSendMsg](#classyojimbo_1_1_connection_1a9aa56543a981ea79269f14ae3f929845)


**See also**: [ChannelConfig](#structyojimbo_1_1_channel_config)

#### `public `[`Message`](#classyojimbo_1_1_message)` * ReceiveMsg(int channelId)` 

Poll this method to receive messages.

Typical usage is to iterate across the set of channels and poll this to receive messages until it returns NULL.

IMPORTANT: The message returned by this function has one reference. You are responsible for releasing this message via [MessageFactory::Release](#classyojimbo_1_1_message_factory_1a20529940091cb084c7876a4396954f50).


#### Parameters
* `channelId` The id of the channel to try to receive a message from. 





#### Returns
A pointer to the received message, NULL if there are no messages to receive.

#### `public `[`ConnectionPacket`](#structyojimbo_1_1_connection_packet)` * GeneratePacket()` 

Generate a connection packet.

This function generates a connection packet containing messages from the send queues of channels specified on this connection. These messages are sent and resent in connection packets until one of those connection packets are acked.

This packet is sent across the network and processed be the connection object on the other side, which drives the transmission of messages from this connection to the other. On the other side, when the connection packet is received it is passed in to [Connection::ProcessPacket](#classyojimbo_1_1_connection_1a6118515c3a0f1764e5f27a378df87886).

The connection packet also handles packet level acks. This ack system is based around the expectation that connection packets are generated and sent regularly in both directions. There are no separate ack packets. Acks are encoded in the packet header of the connection packet.


#### Returns
The connection packet that was generated.


**See also**: [ConnectionPacket](#structyojimbo_1_1_connection_packet)


**See also**: [ProcessPacket](#classyojimbo_1_1_connection_1a6118515c3a0f1764e5f27a378df87886)

#### `public bool ProcessPacket(`[`ConnectionPacket`](#structyojimbo_1_1_connection_packet)` * packet)` 

Process a connection packet.

Connections are endpoints. Across a logical connection on one side is connection A, and on the other side is connection object B.

When a packet generated by A is sent to B, B receives that packet and calls this function to process it, adding any messages contained in the packet its message receive queues.

This transmission of connection packets is bidirectional. A -> B, and B -> A. The ack system relies on packets being sent regularly (10,20,30HZ) in both directions to work properly. There are no separate ack packets.


#### Parameters
* `packet` The connection packet to process. 





#### Returns
True if the packet was processed successfully, false if something went wrong.

#### `public void AdvanceTime(double time)` 

Advance connection time.

Call this at the end of each frame to advance the connection time forward.

IMPORTANT: Please use a double for your time value so it maintains sufficient accuracy as time increases.

#### `public `[`ConnectionError`](#namespaceyojimbo_1aab00f7c22434d6602ba6a53f4c0bb25a)` GetError() const` 

Get the connection error level.

#### Returns
The connection error level. See ConnectionError for details.

#### `public inline void SetListener(`[`ConnectionListener`](#classyojimbo_1_1_connection_listener)` * listener)` 

Set a listener object to receive callbacks (optional).

#### Parameters
* `listener` The listener object.

#### `public inline void SetClientIndex(int clientIndex)` 

Set the client index.

This is used by the server to associate per-client connection objects with the index of their client slot. This makes it easier to work out which client a connection object belongs to on the server.


#### Parameters
* `clientIndex` The client index in [0,numClients-1]. If you don't set this, it defaults to zero.





**See also**: [GetClientIndex](#classyojimbo_1_1_connection_1a97503e22bdb599176a64f6e70655e548)

#### `public inline int GetClientIndex() const` 

Get the client index set on the connection.

This is a helper function to make it easy to work out which client a connection object belongs to on the server.


#### Returns
The client index in [0,numClients-1].


**See also**: [SetClientIndex](#classyojimbo_1_1_connection_1a3accae32e3ad897d6597d6c6ff2ce461)

#### `public uint64_t GetCounter(int index) const` 

Get a counter value.

Counters are used to track event and actions performed by the connection. They are useful for debugging, testing and telemetry.


#### Returns
The counter value. See [yojimbo::ConnectionCounters](#namespaceyojimbo_1a4e3262ace7fbc1b1199d8a88227b5217) for the set of connection counters.

#### `protected void InsertAckPacketEntry(uint16_t sequence)` 

This is called for each connection packet generated, to mark that connection packet as unacked.

Later on this entry is used to determine if a connection packet has already been acked, so we only trigger packet acked callbacks the first time we receive an ack for that packet.


#### Parameters
* `sequence` The sequence number of the connection packet that was generated.

#### `protected void ProcessAcks(uint16_t ack,uint32_t ack_bits)` 

This is the payload function called to process acks in the packet header of the connection packet.

It walks across the ack bits and if bit n is set, then sequence number "ack - n" has been received be the other side, so it should be acked if it is not already.


#### Parameters
* `ack` The most recent acked packet sequence number. 


* `ack_bits` The ack bitfield. Bit n is set if ack - n packet has been received.





**See also**: [ConnectionPacket](#structyojimbo_1_1_connection_packet)

#### `protected void PacketAcked(uint16_t sequence)` 

This method is called when a packet is acked.

It drives all the things that must happen when a packet is acked, such as callbacks, acking messages and data block fragments in send queues, and so on.


#### Parameters
* `sequence` The sequence number of the packet that was acked.

#### `protected virtual void OnPacketAcked(uint16_t sequence)` 





#### `protected virtual void OnChannelFragmentReceived(class `[`Channel`](#classyojimbo_1_1_channel)` * channel,uint16_t messageId,uint16_t fragmentId,int fragmentBytes,int numFragmentsReceived,int numFragmentsInBlock)` 

Override this method to get a callback when a block fragment is received.

#### Parameters
* `channel` The channel the block is being sent over. 


* `messageId` The message id the block is attached to. 


* `fragmentId` The fragment id that is being processed. Fragment ids are in the range [0,numFragments-1]. 


* `fragmentBytes` The size of the fragment in bytes. 


* `numFragmentsReceived` The number of fragments received for this block so far (including this one). 


* `numFragmentsInBlock` The total number of fragments in this block. The block receive completes when all fragments are received.





**See also**: [BlockMessage::AttachBlock](#classyojimbo_1_1_block_message_1af88bdd0b14d6ff6d73144dddb6581563)

# class `yojimbo::ConnectionListener` 


Implement this interface to get callbacks when connection events occur.



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline virtual  ~ConnectionListener()` | 
`public inline virtual void OnConnectionPacketGenerated(class `[`Connection`](#classyojimbo_1_1_connection)` * connection,uint16_t sequence)` | 
`public inline virtual void OnConnectionPacketAcked(class `[`Connection`](#classyojimbo_1_1_connection)` * connection,uint16_t sequence)` | 
`public inline virtual void OnConnectionPacketReceived(class `[`Connection`](#classyojimbo_1_1_connection)` * connection,uint16_t sequence)` | 
`public inline virtual void OnConnectionFragmentReceived(class `[`Connection`](#classyojimbo_1_1_connection)` * connection,int channelId,uint16_t messageId,uint16_t fragmentId,int fragmentBytes,int numFragmentsReceived,int numFragmentsInBlock)` | 

## Members

#### `public inline virtual  ~ConnectionListener()` 





#### `public inline virtual void OnConnectionPacketGenerated(class `[`Connection`](#classyojimbo_1_1_connection)` * connection,uint16_t sequence)` 





#### `public inline virtual void OnConnectionPacketAcked(class `[`Connection`](#classyojimbo_1_1_connection)` * connection,uint16_t sequence)` 





#### `public inline virtual void OnConnectionPacketReceived(class `[`Connection`](#classyojimbo_1_1_connection)` * connection,uint16_t sequence)` 





#### `public inline virtual void OnConnectionFragmentReceived(class `[`Connection`](#classyojimbo_1_1_connection)` * connection,int channelId,uint16_t messageId,uint16_t fragmentId,int fragmentBytes,int numFragmentsReceived,int numFragmentsInBlock)` 





# class `yojimbo::DefaultAllocator` 

```
class yojimbo::DefaultAllocator
  : public yojimbo::Allocator
```  

[Allocator](#classyojimbo_1_1_allocator) implementation based on malloc and free.



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  DefaultAllocator()` | Default constructor.
`public virtual void * Allocate(size_t size,const char * file,int line)` | Allocates a block of memory using "malloc".
`public virtual void Free(void * p,const char * file,int line)` | Free a block of memory by calling "free".

## Members

#### `public inline  DefaultAllocator()` 

Default constructor.



#### `public virtual void * Allocate(size_t size,const char * file,int line)` 

Allocates a block of memory using "malloc".

IMPORTANT: Don't call this directly. Use the YOJIMBO_NEW or YOJIMBO_ALLOCATE macros instead, because they automatically pass in the source filename and line number for you.


#### Parameters
* `size` The size of the block of memory to allocate (bytes). 


* `file` The source code filename that is performing the allocation. Used for tracking allocations and reporting on memory leaks. 


* `line` The line number in the source code file that is performing the allocation.





#### Returns
A block of memory of the requested size, or NULL if the allocation could not be performed. If NULL is returned, the error level is set to ALLOCATION_ERROR_FAILED_TO_ALLOCATE.

#### `public virtual void Free(void * p,const char * file,int line)` 

Free a block of memory by calling "free".

IMPORTANT: Don't call this directly. Use the YOJIMBO_DELETE or YOJIMBO_FREE macros instead, because they automatically pass in the source filename and line number for you.


#### Parameters
* `p` Pointer to the block of memory to free. Must be non-NULL block of memory that was allocated with this allocator. Will assert otherwise. 


* `file` The source code filename that is performing the free. Used for tracking allocations and reporting on memory leaks. 


* `line` The line number in the source code file that is performing the free.

# class `yojimbo::EncryptionManager` 


Associates addresses with encryption keys so each client gets their own keys for packet encryption.

Separate keys are used for packets sent to an address vs. packets received from this address.

This was done to allow the client/server to use the sequence numbers of packets as a nonce in both directions. An alternative would have been to set the high bit of the packet sequence number in one of the directions, but I felt this was cleaner.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  EncryptionManager()` | Encryption manager constructor.
`public bool AddEncryptionMapping(const `[`Address`](#classyojimbo_1_1_address)` & address,const uint8_t * sendKey,const uint8_t * receiveKey,double time,double timeout)` | Associates an address with send and receive keys for packet encryption.
`public bool RemoveEncryptionMapping(const `[`Address`](#classyojimbo_1_1_address)` & address,double time)` | Remove the encryption mapping for an address.
`public void ResetEncryptionMappings()` | Reset all encryption mappings.
`public int FindEncryptionMapping(const `[`Address`](#classyojimbo_1_1_address)` & address,double time)` | Find an encryption mapping (index) for the specified address.
`public const uint8_t * GetSendKey(int index) const` | Get the send key for an encryption mapping (by index).
`public const uint8_t * GetReceiveKey(int index) const` | Get the receive key for an encryption mapping (by index).

## Members

#### `public  EncryptionManager()` 

Encryption manager constructor.



#### `public bool AddEncryptionMapping(const `[`Address`](#classyojimbo_1_1_address)` & address,const uint8_t * sendKey,const uint8_t * receiveKey,double time,double timeout)` 

Associates an address with send and receive keys for packet encryption.

#### Parameters
* `address` The address to associate with encryption keys. 


* `sendKey` The key used to encrypt packets sent to this address. 


* `receiveKey` The key used to decrypt packets received from this address. 


* `time` The current time (seconds). 


* `timeout` The timeout value in seconds for this encryption mapping (seconds). Encyrption mapping times out if no packets are sent to or received from this address in the timeout period.





#### Returns
True if the encryption mapping was added successfully, false otherwise.

#### `public bool RemoveEncryptionMapping(const `[`Address`](#classyojimbo_1_1_address)` & address,double time)` 

Remove the encryption mapping for an address.

#### Parameters
* `address` The address of the encryption mapping to remove. 


* `time` The current time (seconds).





#### Returns
True if an encryption mapping for the address exists and was removed, false if no encryption mapping could be found for the address.

#### `public void ResetEncryptionMappings()` 

Reset all encryption mappings.

Any encryption mappings that were added are cleared and the encryption manager is reset back to default state.

#### `public int FindEncryptionMapping(const `[`Address`](#classyojimbo_1_1_address)` & address,double time)` 

Find an encryption mapping (index) for the specified address.

IMPORTANT: This function "touches" the encryption mapping and resets its last access time to the current time. As long as this is called regularly for an encryption mapping it won't time out.


#### Parameters
* `address` The address of the encryption mapping to search for. 


* `time` The current time (seconds).





#### Returns
The index of the encryption mapping if one exists for the address. -1 if no encryption mapping was found.


**See also**: [EncryptionManager::GetSendKey](#classyojimbo_1_1_encryption_manager_1a279508078199d89b868930342741ead5)


**See also**: [EncryptionManager::GetReceiveKey](#classyojimbo_1_1_encryption_manager_1a688223bb6e76386c9c1f6f97562f6d84)

#### `public const uint8_t * GetSendKey(int index) const` 

Get the send key for an encryption mapping (by index).

**See also**: [EncryptionManager::FindEncryptionMapping](#classyojimbo_1_1_encryption_manager_1aa7664538221c241a6ce472c3b740df59)


#### Parameters
* `index` The encryption mapping index. See EncryptionMapping::FindEncryptionMapping





#### Returns
The key to encrypt sent packets.

#### `public const uint8_t * GetReceiveKey(int index) const` 

Get the receive key for an encryption mapping (by index).

**See also**: [EncryptionManager::FindEncryptionMapping](#classyojimbo_1_1_encryption_manager_1aa7664538221c241a6ce472c3b740df59)


#### Parameters
* `index` The encryption mapping index. See EncryptionMapping::FindEncryptionMapping





#### Returns
The key to decrypt received packets.

# class `yojimbo::LocalTransport` 

```
class yojimbo::LocalTransport
  : public yojimbo::BaseTransport
```  

Implements a local transport built on top of a network simulator.

This transport does not provide any networking sockets, and is used in unit tests and loopback.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  LocalTransport(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,class `[`NetworkSimulator`](#classyojimbo_1_1_network_simulator)` & networkSimulator,const `[`Address`](#classyojimbo_1_1_address)` & address,uint64_t protocolId,double time,int maxPacketSize,int sendQueueSize,int receiveQueueSize)` | Local transport constructor.
`public  ~LocalTransport()` | 
`public virtual void Reset()` | The local transport guarantees that any packets sent to this transport prior to a call to reset will not cross this boundary.
`public virtual void AdvanceTime(double time)` | As an optimization, the local transport receives packets from the simulator inside this function. This avoids O(n^2) performance, where n is the maximum number of packets that can be stored in the simulator.
`protected void DiscardReceivePackets()` | 
`protected void PumpReceivePacketsFromSimulator()` | 
`protected virtual void InternalSendPacket(const `[`Address`](#classyojimbo_1_1_address)` & to,const void * packetData,int packetBytes)` | Internal function to send a packet over the network.
`protected virtual int InternalReceivePacket(`[`Address`](#classyojimbo_1_1_address)` & from,void * packetData,int maxPacketSize)` | Internal function to receive a packet from the network.

## Members

#### `public  LocalTransport(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,class `[`NetworkSimulator`](#classyojimbo_1_1_network_simulator)` & networkSimulator,const `[`Address`](#classyojimbo_1_1_address)` & address,uint64_t protocolId,double time,int maxPacketSize,int sendQueueSize,int receiveQueueSize)` 

Local transport constructor.

#### Parameters
* `allocator` The allocator used for transport allocations. 


* `networkSimulator` The network simulator to use. Typically, you use one network simulator shared across multiple local transports. See test.cpp and client_server.cpp for examples. 


* `address` The address of the packet. This is how other other transports would send packets to this transport. 


* `protocolId` The protocol id for this transport. Protocol id is included in the packet header, packets received with a different protocol id are discarded. This allows multiple versions of your protocol to exist on the same network. 


* `time` The current time value in seconds. 


* `maxPacketSize` The maximum packet size that can be sent across this transport. 


* `sendQueueSize` The size of the packet send queue (number of packets). 


* `receiveQueueSize` The size of the packet receive queue (number of packets).

#### `public  ~LocalTransport()` 





#### `public virtual void Reset()` 

The local transport guarantees that any packets sent to this transport prior to a call to reset will not cross this boundary.



#### `public virtual void AdvanceTime(double time)` 

As an optimization, the local transport receives packets from the simulator inside this function. This avoids O(n^2) performance, where n is the maximum number of packets that can be stored in the simulator.



#### `protected void DiscardReceivePackets()` 





#### `protected void PumpReceivePacketsFromSimulator()` 





#### `protected virtual void InternalSendPacket(const `[`Address`](#classyojimbo_1_1_address)` & to,const void * packetData,int packetBytes)` 

Internal function to send a packet over the network.

IMPORTANT: Override this to implement your own packet send function for derived transport classes.


#### Parameters
* `to` The address the packet should be sent to. 


* `packetData` The serialized, and potentially encrypted packet data generated by [BaseTransport::WritePacket](#classyojimbo_1_1_base_transport_1a7a2a0abc1756c513695118ea5d78b503). 


* `packetBytes` The size of the packet data in bytes.

#### `protected virtual int InternalReceivePacket(`[`Address`](#classyojimbo_1_1_address)` & from,void * packetData,int maxPacketSize)` 

Internal function to receive a packet from the network.

IMPORTANT: Override this to implement your own packet receive function for derived transport classes.

IMPORTANT: This call must be non-blocking.


#### Parameters
* `from` The address that sent the packet [out]. 


* `packetData` The buffer to which will receive packet data read from the network. 


* `maxPacketSize` The size of your packet data buffer. Packets received from the network that are larger than this are discarded.





#### Returns
The number of packet bytes read from the network, 0 if no packet data was read.

# class `yojimbo::Matcher` 


Communicates with the matcher web service over HTTPS.

See docker/matcher/matcher.go for details. Launch the matcher via "premake5 matcher".

This class will be improved in the future, most importantly to make [Matcher::RequestMatch](#classyojimbo_1_1_matcher_1ae4ef8e778f57397c4279a0fb7ae2aea4) a non-blocking operation.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  explicit Matcher(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator)` | [Matcher](#classyojimbo_1_1_matcher) constructor.
`public  ~Matcher()` | [Matcher](#classyojimbo_1_1_matcher) destructor.
`public bool Initialize()` | Initialize the matcher.
`public void RequestMatch(uint64_t protocolId,uint64_t clientId)` | Request a match.
`public `[`MatchStatus`](#namespaceyojimbo_1ad547327903c685eb434fce170b809eed)` GetMatchStatus()` | Get the current match status.
`public void GetMatchResponse(`[`MatchResponse`](#structyojimbo_1_1_match_response)` & matchResponse)` | Get match response data.
`protected bool ParseMatchResponse(const char * json,`[`MatchResponse`](#structyojimbo_1_1_match_response)` & matchResponse)` | Helper function to parse the match response JSON into the [MatchResponse](#structyojimbo_1_1_match_response) struct.

## Members

#### `public  explicit Matcher(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator)` 

[Matcher](#classyojimbo_1_1_matcher) constructor.

#### Parameters
* `allocator` The allocator to use for allocations.

#### `public  ~Matcher()` 

[Matcher](#classyojimbo_1_1_matcher) destructor.



#### `public bool Initialize()` 

Initialize the matcher.

#### Returns
True if the matcher initialized successfully, false otherwise.

#### `public void RequestMatch(uint64_t protocolId,uint64_t clientId)` 

Request a match.

This is how clients get connect tokens from matcher.go.

They request a match and the server replies with a set of servers to connect to, and a connect token to pass to that server.

IMPORTANT: This function is currently blocking. It will be made non-blocking in the near future.


#### Parameters
* `protocolId` The protocol id that we are using. Used to filter out servers with different protocol versions. 


* `clientId` A unique client identifier that identifies each client to your back end services. If you don't have this yet, just roll a random 64bit number.





**See also**: [Matcher::GetMatchStatus](#classyojimbo_1_1_matcher_1a28b5623108bdcb3962af375c44bc82e6)


**See also**: [Matcher::GetMatchResponse](#classyojimbo_1_1_matcher_1ae559a5c66674d8d30c10cc0e730a3f7d)

#### `public `[`MatchStatus`](#namespaceyojimbo_1ad547327903c685eb434fce170b809eed)` GetMatchStatus()` 

Get the current match status.

Because [Matcher::RequestMatch](#classyojimbo_1_1_matcher_1ae4ef8e778f57397c4279a0fb7ae2aea4) is currently blocking this will be MATCH_READY or MATCH_FAILED immediately after that function returns.

If the status is MATCH_READY you can call [Matcher::GetMatchResponse](#classyojimbo_1_1_matcher_1ae559a5c66674d8d30c10cc0e730a3f7d) to get the match response data corresponding to the last call to [Matcher::RequestMatch](#classyojimbo_1_1_matcher_1ae4ef8e778f57397c4279a0fb7ae2aea4).


#### Returns
The current match status.

#### `public void GetMatchResponse(`[`MatchResponse`](#structyojimbo_1_1_match_response)` & matchResponse)` 

Get match response data.

This can only be called if the match status is MATCH_READY.


#### Parameters
* `matchResponse` The match response data to fill [out].





**See also**: [Matcher::RequestMatch](#classyojimbo_1_1_matcher_1ae4ef8e778f57397c4279a0fb7ae2aea4)


**See also**: [Matcher::GetMatchStatus](#classyojimbo_1_1_matcher_1a28b5623108bdcb3962af375c44bc82e6)

#### `protected bool ParseMatchResponse(const char * json,`[`MatchResponse`](#structyojimbo_1_1_match_response)` & matchResponse)` 

Helper function to parse the match response JSON into the [MatchResponse](#structyojimbo_1_1_match_response) struct.

#### Parameters
* `json` The JSON match response string to parse. 


* `matchResponse` The match response structure to fill [out].





#### Returns
True if the match response JSON was successfully parsed, false otherwise.

# class `yojimbo::MeasureStream` 

```
class yojimbo::MeasureStream
  : public yojimbo::BaseStream
```  

Stream class for estimating how many bits it would take to serialize something.

This class acts like a bit writer (IsWriting is 1, IsReading is 0), but it doesn't actually write anything, it just counts how many bits would be written.

It's used by the connection channel classes to work out how many messages will fit in the channel packet budget.

Note that when the serialization includes alignment to byte (see [MeasureStream::SerializeAlign](#classyojimbo_1_1_measure_stream_1abdfeabfe8b36a8f16a3478b91f6017dc)), this is an estimate and not an exact measurement. The estimate is guaranteed to be conservative.


**See also**: [BitWriter](#classyojimbo_1_1_bit_writer)


**See also**: [BitReader](#classyojimbo_1_1_bit_reader)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  explicit MeasureStream(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator)` | Measure stream constructor.
`public inline bool SerializeInteger(int32_t value,int32_t min,int32_t max)` | Serialize an integer (measure).
`public inline bool SerializeBits(uint32_t value,int bits)` | Serialize a number of bits (write).
`public inline bool SerializeBytes(const uint8_t * data,int bytes)` | Serialize an array of bytes (measure).
`public inline bool SerializeAlign()` | Serialize an align (measure).
`public inline int GetAlignBits() const` | If we were to write an align right now, how many bits would be required?
`public inline bool SerializeCheck()` | Serialize a safety check to the stream (measure).
`public inline int GetBitsProcessed() const` | Get number of bits written so far.
`public inline int GetBytesProcessed() const` | How many bytes have been written so far?

## Members

#### `public inline  explicit MeasureStream(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator)` 

Measure stream constructor.

#### Parameters
* `allocator` The allocator to use for stream allocations. This lets you dynamically allocate memory as you read and write packets.

#### `public inline bool SerializeInteger(int32_t value,int32_t min,int32_t max)` 

Serialize an integer (measure).

#### Parameters
* `value` The integer value to write. Not actually used or checked. 


* `min` The minimum value. 


* `max` The maximum value.





#### Returns
Always returns true. All checking is performed by debug asserts only on measure.

#### `public inline bool SerializeBits(uint32_t value,int bits)` 

Serialize a number of bits (write).

#### Parameters
* `value` The unsigned integer value to serialize. Not actually used or checked. 


* `bits` The number of bits to write in [1,32].





#### Returns
Always returns true. All checking is performed by debug asserts on write.

#### `public inline bool SerializeBytes(const uint8_t * data,int bytes)` 

Serialize an array of bytes (measure).

#### Parameters
* `data` Array of bytes to 'write'. Not actually used. 


* `bytes` The number of bytes to 'write'.





#### Returns
Always returns true. All checking is performed by debug asserts on write.

#### `public inline bool SerializeAlign()` 

Serialize an align (measure).

#### Returns
Always returns true. All checking is performed by debug asserts on write.

#### `public inline int GetAlignBits() const` 

If we were to write an align right now, how many bits would be required?

IMPORTANT: Since the number of bits required for alignment depends on where an object is written in the final bit stream, this measurement is conservative.


#### Returns
Always returns worst case 7 bits.

#### `public inline bool SerializeCheck()` 

Serialize a safety check to the stream (measure).

#### Returns
Always returns true. All checking is performed by debug asserts on write.

#### `public inline int GetBitsProcessed() const` 

Get number of bits written so far.

#### Returns
Number of bits written.

#### `public inline int GetBytesProcessed() const` 

How many bytes have been written so far?

#### Returns
Number of bytes written.

# class `yojimbo::Message` 

```
class yojimbo::Message
  : public yojimbo::Serializable
```  

A reference counted object that can be serialized to a bitstream.

Messages are objects that are sent between client and server across the connection. They are carried inside the [ConnectionPacket](#structyojimbo_1_1_connection_packet) generated by the [Connection](#classyojimbo_1_1_connection) class. Messages can be sent reliable-ordered, or unreliable-unordered, depending on the configuration of the channel they are sent over.

To use messages, create your own set of message classes by inheriting from this class (or from [BlockMessage](#classyojimbo_1_1_block_message), if you want to attach data blocks to your message), then setup an enum of all your message types and derive a message factory class to create your message instances by type.

There are macros to help make defining your message factory painless: YOJIMBO_MESSAGE_FACTORY_START
YOJIMBO_DECLARE_MESSAGE_TYPE
YOJIMBO_MESSAGE_FACTORY_FINISH


Once you have a message factory, register it with your declared inside your client and server classes using: YOJIMBO_MESSAGE_FACTORY


which overrides the Client::CreateMessageFactory and Server::CreateMessageFactory methods so the client and server classes use your message factory type.

See tests/shared.h for an example showing you how to do this, and the functional tests inside tests/test.cpp for examples showing how how to send and receive messages.


**See also**: [BlockMessage](#classyojimbo_1_1_block_message)


**See also**: [MessageFactory](#classyojimbo_1_1_message_factory)


**See also**: [ClientServerConfig](#structyojimbo_1_1_client_server_config)


**See also**: [Connection](#classyojimbo_1_1_connection)


**See also**: [ConnectionConfig](#structyojimbo_1_1_connection_config)


**See also**: [ChannelConfig](#structyojimbo_1_1_channel_config)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  Message(int blockMessage)` | [Message](#classyojimbo_1_1_message) constructor.
`public inline void SetId(uint16_t id)` | Set the message id.
`public inline int GetId() const` | Get the message id.
`public inline int GetType() const` | Get the message type.
`public inline int GetRefCount()` | Get the reference count on the message.
`public inline bool IsBlockMessage() const` | Is this a block message?
`public bool SerializeInternal(`[`ReadStream`](#classyojimbo_1_1_read_stream)` & stream)` | Virtual serialize function (read).
`public bool SerializeInternal(`[`WriteStream`](#classyojimbo_1_1_write_stream)` & stream)` | Virtual serialize function (write).
`public bool SerializeInternal(`[`MeasureStream`](#classyojimbo_1_1_measure_stream)` & stream)` | Virtual serialize function (measure).
`protected inline void SetType(int type)` | Set the message type.
`protected inline void AddRef()` | Add a reference to the message.
`protected inline void Release()` | Remove a reference from the message.
`protected inline virtual  ~Message()` | [Message](#classyojimbo_1_1_message) destructor.

## Members

#### `public inline  Message(int blockMessage)` 

[Message](#classyojimbo_1_1_message) constructor.

Don't call this directly, use a message factory instead.


#### Parameters
* `blockMessage` 1 if this is a block message, 0 otherwise.





**See also**: [MessageFactory::Create](#classyojimbo_1_1_message_factory_1a29aad59d68607c3d279c61d394f5b56a)

#### `public inline void SetId(uint16_t id)` 

Set the message id.

When messages are sent across the [ReliableOrderedChannel](#classyojimbo_1_1_reliable_ordered_channel), the message id starts at 0 and increases with each message sent over that channel, wrapping around from 65535 -> 0. It's used to reconstruct message order on the receiver, so messages are received in the same order they were sent.

Over the [UnreliableUnorderedChannel](#classyojimbo_1_1_unreliable_unordered_channel), there is no ordering, so the message id is set to the sequence number of the connection packet instead.


#### Parameters
* `id` The message id.

#### `public inline int GetId() const` 

Get the message id.

#### Returns
The message id.

#### `public inline int GetType() const` 

Get the message type.

This corresponds to the type enum value used to create the message in the message factory.


#### Returns
The message type.


**See also**: [MessageFactory](#classyojimbo_1_1_message_factory).

#### `public inline int GetRefCount()` 

Get the reference count on the message.

Messages start with a reference count of 1 when they are created. This is decreased when they are released.

When the reference count reaches 0, the message is destroyed.


#### Returns
The reference count on the message.

#### `public inline bool IsBlockMessage() const` 

Is this a block message?

Block messages are of type [BlockMessage](#classyojimbo_1_1_block_message) and can have a data block attached to the message.


#### Returns
True if this is a block message, false otherwise.


**See also**: [BlockMessage](#classyojimbo_1_1_block_message).

#### `public bool SerializeInternal(`[`ReadStream`](#classyojimbo_1_1_read_stream)` & stream)` 

Virtual serialize function (read).

Reads the message in from a bitstream.

Don't override this method directly, instead, use the YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS macro in your derived message class to redirect it to a templated serialize method.

This way you can implement serialization for your packets in a single method and the C++ compiler takes care of generating specialized read, write and measure implementations for you.

See tests/shared.h for some examples of this.

#### `public bool SerializeInternal(`[`WriteStream`](#classyojimbo_1_1_write_stream)` & stream)` 

Virtual serialize function (write).

Write the message to a bitstream.

Don't override this method directly, instead, use the YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS macro in your derived message class to redirect it to a templated serialize method.

This way you can implement serialization for your packets in a single method and the C++ compiler takes care of generating specialized read, write and measure implementations for you.

See tests/shared.h for some examples of this.

#### `public bool SerializeInternal(`[`MeasureStream`](#classyojimbo_1_1_measure_stream)` & stream)` 

Virtual serialize function (measure).

Measure how many bits this message would take to write. This is used when working out how many messages will fit within the channel packet budget.

Don't override this method directly, instead, use the YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS macro in your derived message class to redirect it to a templated serialize method.

This way you can implement serialization for your packets in a single method and the C++ compiler takes care of generating specialized read, write and measure implementations for you.

See tests/shared.h for some examples of this.

#### `protected inline void SetType(int type)` 

Set the message type.

Called by the message factory after it creates a message.


#### Parameters
* `type` The message type.

#### `protected inline void AddRef()` 

Add a reference to the message.

This is called when a message is included in a packet and added to the receive queue.

This way we don't have to pass messages by value (more efficient) and messages get cleaned up when they are delivered and no packets refer to them.

#### `protected inline void Release()` 

Remove a reference from the message.

[Message](#classyojimbo_1_1_message) are deleted when the number of references reach zero. Messages have reference count of 1 after creation.

#### `protected inline virtual  ~Message()` 

[Message](#classyojimbo_1_1_message) destructor.

Protected because you aren't supposed delete messages directly because they are reference counted. Use [MessageFactory::Release](#classyojimbo_1_1_message_factory_1a20529940091cb084c7876a4396954f50) instead.


**See also**: [MessageFactory::Release](#classyojimbo_1_1_message_factory_1a20529940091cb084c7876a4396954f50)

# class `yojimbo::MessageFactory` 


Defines the set of message types that can be created.

You can derive a message factory yourself to create your own message types, or you can use these helper macros to do it for you: YOJIMBO_MESSAGE_FACTORY_START
YOJIMBO_DECLARE_MESSAGE_TYPE
YOJIMBO_MESSAGE_FACTORY_FINISH


See tests/shared.h for an example showing how to use the macros.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  MessageFactory(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,int numTypes)` | [Message](#classyojimbo_1_1_message) factory allocator.
`public inline virtual  ~MessageFactory()` | [Message](#classyojimbo_1_1_message) factory destructor.
`public inline `[`Message`](#classyojimbo_1_1_message)` * Create(int type)` | Create a message by type.
`public inline void AddRef(`[`Message`](#classyojimbo_1_1_message)` * message)` | Add a reference to a message.
`public inline void Release(`[`Message`](#classyojimbo_1_1_message)` * message)` | Remove a reference from a message.
`public inline int GetNumTypes() const` | Get the number of message types supported by this message factory.
`public inline `[`Allocator`](#classyojimbo_1_1_allocator)` & GetAllocator()` | Get the allocator used to create messages.
`public inline int GetError() const` | Get the error level.
`public inline void ClearError()` | Clear the error level back to no error.
`protected inline virtual `[`Message`](#classyojimbo_1_1_message)` * CreateMessage(int type)` | This method is overridden to create messages by type.
`protected inline void SetMessageType(`[`Message`](#classyojimbo_1_1_message)` * message,int type)` | Set the message type of a message.

## Members

#### `public inline  MessageFactory(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,int numTypes)` 

[Message](#classyojimbo_1_1_message) factory allocator.

Pass in the number of message types for the message factory from the derived class.


#### Parameters
* `allocator` The allocator used to create messages. 


* `numTypes` The number of message types. Valid types are in [0,numTypes-1].

#### `public inline virtual  ~MessageFactory()` 

[Message](#classyojimbo_1_1_message) factory destructor.

Checks for message leaks if YOJIMBO_DEBUG_MESSAGE_LEAKS is defined and not equal to zero. This is on by default in debug build.

#### `public inline `[`Message`](#classyojimbo_1_1_message)` * Create(int type)` 

Create a message by type.

IMPORTANT: Check the message pointer returned by this call. It can be NULL if there is no memory to create a message!

Messages returned from this function have one reference added to them. When you are finished with the message, pass it to [MessageFactory::Release](#classyojimbo_1_1_message_factory_1a20529940091cb084c7876a4396954f50).


#### Parameters
* `type` The message type in [0,numTypes-1].





#### Returns
The allocated message, or NULL if the message could not be allocated. If the message allocation fails, the message factory error level is set to MESSAGE_FACTORY_ERROR_FAILED_TO_ALLOCATE_MESSAGE.


**See also**: [MessageFactory::AddRef](#classyojimbo_1_1_message_factory_1ad4fa19b6542cfb384eafed1363b76a7e)


**See also**: [MessageFactory::Release](#classyojimbo_1_1_message_factory_1a20529940091cb084c7876a4396954f50)

#### `public inline void AddRef(`[`Message`](#classyojimbo_1_1_message)` * message)` 

Add a reference to a message.

#### Parameters
* `message` The message to add a reference to.





**See also**: [MessageFactory::Create](#classyojimbo_1_1_message_factory_1a29aad59d68607c3d279c61d394f5b56a)


**See also**: [MessageFactory::Release](#classyojimbo_1_1_message_factory_1a20529940091cb084c7876a4396954f50)

#### `public inline void Release(`[`Message`](#classyojimbo_1_1_message)` * message)` 

Remove a reference from a message.

Messages have 1 reference when created. When the reference count reaches 0, they are destroyed.


**See also**: [MessageFactory::Create](#classyojimbo_1_1_message_factory_1a29aad59d68607c3d279c61d394f5b56a)


**See also**: [MessageFactory::AddRef](#classyojimbo_1_1_message_factory_1ad4fa19b6542cfb384eafed1363b76a7e)

#### `public inline int GetNumTypes() const` 

Get the number of message types supported by this message factory.

#### Returns
The number of message types.

#### `public inline `[`Allocator`](#classyojimbo_1_1_allocator)` & GetAllocator()` 

Get the allocator used to create messages.

#### Returns
The allocator.

#### `public inline int GetError() const` 

Get the error level.

When used with a client or server, an error level on a message factory other than MESSAGE_FACTORY_ERROR_NONE triggers a client disconnect.

#### `public inline void ClearError()` 

Clear the error level back to no error.



#### `protected inline virtual `[`Message`](#classyojimbo_1_1_message)` * CreateMessage(int type)` 

This method is overridden to create messages by type.

#### Parameters
* `type` The type of message to be created.





#### Returns
The message created. Its reference count is 1.

#### `protected inline void SetMessageType(`[`Message`](#classyojimbo_1_1_message)` * message,int type)` 

Set the message type of a message.

Put here because Message::SetMessageType is protected, but we need to be able to call this inside the overridden [MessageFactory::CreateMessage](#classyojimbo_1_1_message_factory_1ac7de3a0deabefe256988879fcbca5fbc) method.


#### Parameters
* `message` The message object. 


* `type` The message type to set.

# class `yojimbo::NetworkSimulator` 


Simulates packet loss, latency, jitter and duplicate packets.

One of these is created for each network transport created, allowing you to simulate bad network conditions on top of your transport.

This is useful during development, so your game is tested and played under real world conditions, instead of ideal LAN conditions.

This simulator works on packet send. This means that if you want 125ms of latency (round trip), you must to add 125/2 = 62.5ms of latency to each side.


**See also**: [NetworkTransport](#classyojimbo_1_1_network_transport)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  NetworkSimulator(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,int numPackets)` | Create a network simulator.
`public  ~NetworkSimulator()` | Network simulator destructor.
`public void SetLatency(float milliseconds)` | Set the latency in milliseconds.
`public void SetJitter(float milliseconds)` | Set the packet jitter in milliseconds.
`public void SetPacketLoss(float percent)` | Set the amount of packet loss to apply on send.
`public void SetDuplicate(float percent)` | Set percentage chance of packet duplicates.
`public bool IsActive() const` | Is the network simulator active?
`public void SendPacket(const `[`Address`](#classyojimbo_1_1_address)` & from,const `[`Address`](#classyojimbo_1_1_address)` & to,uint8_t * packetData,int packetSize)` | [Queue](#classyojimbo_1_1_queue) a packet up for send.
`public int ReceivePackets(int maxPackets,uint8_t * packetData,int packetSize,`[`Address`](#classyojimbo_1_1_address)` from,`[`Address`](#classyojimbo_1_1_address)` to)` | Receive packets sent to any address.
`public int ReceivePacketsSentToAddress(int maxPackets,const `[`Address`](#classyojimbo_1_1_address)` & to,uint8_t * packetData,int packetSize,`[`Address`](#classyojimbo_1_1_address)` from)` | Receive packets sent to a specified address.
`public void DiscardPackets()` | Discard all packets in the network simulator.
`public void DiscardPacketsFromAddress(const `[`Address`](#classyojimbo_1_1_address)` & address)` | Discard all packets sent from an address.
`public void AdvanceTime(double time)` | Advance network simulator time.
`public inline `[`Allocator`](#classyojimbo_1_1_allocator)` & GetAllocator()` | Get the allocator to use to free packet data.
`protected void UpdateActive()` | Helper function to update the active flag whenever network settings are changed.
`protected void UpdatePendingReceivePackets()` | Packets ready to be received are removed from the main simulator buffer and put into pending receive packet arrays by this function.

## Members

#### `public  NetworkSimulator(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,int numPackets)` 

Create a network simulator.

Initial network conditions are set to: Latency: 0ms
Jitter: 0ms
Packet Loss: 0%
Duplicates: 0%


This means the simulator should not in theory have any affect on packets sent through it. See [NetworkSimulator::IsActive](#classyojimbo_1_1_network_simulator_1a44042ef8bb3f56a117b3266aeb68b92b).


#### Parameters
* `allocator` The allocator to use. 


* `numPackets` The maximum number of packets that can be stored in the simulator at any time.

#### `public  ~NetworkSimulator()` 

Network simulator destructor.

Any packet data still in the network simulator is destroyed.

#### `public void SetLatency(float milliseconds)` 

Set the latency in milliseconds.

This latency is added on packet send. To simulate a round trip time of 100ms, add 50ms of latency to both sides of the connection.


#### Parameters
* `milliseconds` The latency to add in milliseconds.

#### `public void SetJitter(float milliseconds)` 

Set the packet jitter in milliseconds.

Jitter is applied +/- this amount in milliseconds. To be truly effective, jitter must be applied together with some latency.


#### Parameters
* `milliseconds` The amount of jitter to add in milliseconds (+/-).

#### `public void SetPacketLoss(float percent)` 

Set the amount of packet loss to apply on send.

#### Parameters
* `percent` The packet loss percentage. 0% = no packet loss. 100% = all packets are dropped.

#### `public void SetDuplicate(float percent)` 

Set percentage chance of packet duplicates.

If the duplicate chance succeeds, a duplicate packet is added to the queue with a random delay of up to 1 second.


#### Parameters
* `percent` The percentage chance of a packet duplicate being sent. 0% = no duplicate packets. 100% = all packets have a duplicate sent.

#### `public bool IsActive() const` 

Is the network simulator active?

The network simulator is active when packet loss, latency, duplicates or jitter are non-zero values.

This is used by the transport to know whether it should shunt packets through the simulator, or send them directly to the network. This is a minor optimization.

#### `public void SendPacket(const `[`Address`](#classyojimbo_1_1_address)` & from,const `[`Address`](#classyojimbo_1_1_address)` & to,uint8_t * packetData,int packetSize)` 

[Queue](#classyojimbo_1_1_queue) a packet up for send.

IMPORTANT: Ownership of the packet data pointer is *not* transferred to the network simulator. It makes a copy of the data instead.


#### Parameters
* `from` The address the packet is sent from. 


* `to` The addresrs the packet is sent to. 


* `packetData` The packet data. 


* `packetSize` The packet size (bytes).

#### `public int ReceivePackets(int maxPackets,uint8_t * packetData,int packetSize,`[`Address`](#classyojimbo_1_1_address)` from,`[`Address`](#classyojimbo_1_1_address)` to)` 

Receive packets sent to any address.

IMPORTANT: You take ownership of the packet data you receive and are responsible for freeing it. See [NetworkSimulator::GetAllocator](#classyojimbo_1_1_network_simulator_1afe7940951311a3dceb534191e98ded63).


#### Parameters
* `maxPackets` The maximum number of packets to receive. 


* `packetData` Array of packet data pointers to be filled [out]. 


* `packetSize` Array of packet sizes to be filled [out]. 


* `from` Array of from addresses to be filled [out]. 


* `to` Array of to addresses to be filled [out].





#### Returns
The number of packets received.

#### `public int ReceivePacketsSentToAddress(int maxPackets,const `[`Address`](#classyojimbo_1_1_address)` & to,uint8_t * packetData,int packetSize,`[`Address`](#classyojimbo_1_1_address)` from)` 

Receive packets sent to a specified address.

IMPORTANT: You take ownership of the packet data you receive and are responsible for freeing it. See [NetworkSimulator::GetAllocator](#classyojimbo_1_1_network_simulator_1afe7940951311a3dceb534191e98ded63).


#### Parameters
* `maxPackets` The maximum number of packets to receive. 


* `to` Only packets sent to this address will be received. 


* `packetData` Array of packet data pointers to be filled [out]. 


* `packetSize` Array of packet sizes to be filled [out]. 


* `from` Array of from addresses to be filled [out].





#### Returns
The number of packets received.

#### `public void DiscardPackets()` 

Discard all packets in the network simulator.

This is useful if the simulator needs to be reset and used for another purpose.

#### `public void DiscardPacketsFromAddress(const `[`Address`](#classyojimbo_1_1_address)` & address)` 

Discard all packets sent from an address.

#### Parameters
* `address` Packets sent from this address will be discarded.

#### `public void AdvanceTime(double time)` 

Advance network simulator time.

You must pump this regularly otherwise the network simulator won't work.


#### Parameters
* `time` The current time value. Please make sure you use double values for time so you retain sufficient precision as time increases.

#### `public inline `[`Allocator`](#classyojimbo_1_1_allocator)` & GetAllocator()` 

Get the allocator to use to free packet data.

#### Returns
The allocator that packet data is allocated with.

#### `protected void UpdateActive()` 

Helper function to update the active flag whenever network settings are changed.

Active is set to true if any of the network conditions are non-zero. This allows you to quickly check if the network simulator is active and would actually do something.

#### `protected void UpdatePendingReceivePackets()` 

Packets ready to be received are removed from the main simulator buffer and put into pending receive packet arrays by this function.

This is an optimization that reduces the amount of work that needs to be done when walking the simulator to dequeue packets sent to a particular address.

# class `yojimbo::NetworkTransport` 

```
class yojimbo::NetworkTransport
  : public yojimbo::BaseTransport
```  

Implements a network transport built on top of non-blocking sendto and recvfrom socket APIs.



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  NetworkTransport(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,const `[`Address`](#classyojimbo_1_1_address)` & address,uint64_t protocolId,double time,int maxPacketSize,int sendQueueSize,int receiveQueueSize,int socketSendBufferSize,int socketReceiveBufferSize)` | Network transport constructor.
`public  ~NetworkTransport()` | 
`public bool IsError() const` | You should call this after creating a network transport, to make sure the socket was created successfully.
`public int GetError() const` | Get the socket error code.
`protected virtual void InternalSendPacket(const `[`Address`](#classyojimbo_1_1_address)` & to,const void * packetData,int packetBytes)` | Overridden internal packet send function. Effectively just calls through to sendto.
`protected virtual int InternalReceivePacket(`[`Address`](#classyojimbo_1_1_address)` & from,void * packetData,int maxPacketSize)` | Overriden internal packet receive function. Effectively just wraps recvfrom.

## Members

#### `public  NetworkTransport(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,const `[`Address`](#classyojimbo_1_1_address)` & address,uint64_t protocolId,double time,int maxPacketSize,int sendQueueSize,int receiveQueueSize,int socketSendBufferSize,int socketReceiveBufferSize)` 

Network transport constructor.

#### Parameters
* `allocator` The allocator used for transport allocations. 


* `address` The address to send packets to that would be received by this transport. 


* `protocolId` The protocol id for this transport. Protocol id is included in the packet header, packets received with a different protocol id are discarded. This allows multiple versions of your protocol to exist on the same network. 


* `time` The current time value in seconds. 


* `maxPacketSize` The maximum packet size that can be sent across this transport. 


* `sendQueueSize` The size of the packet send queue (number of packets). 


* `receiveQueueSize` The size of the packet receive queue (number of packets). 


* `socketSendBufferSize` The size of the send buffers to set on the socket (SO_SNDBUF). 


* `socketReceiveBufferSize` The size of the send buffers to set on the socket (SO_RCVBUF).

#### `public  ~NetworkTransport()` 





#### `public bool IsError() const` 

You should call this after creating a network transport, to make sure the socket was created successfully.

#### Returns
True if the socket is in error state.


**See also**: [NetworkTransport::GetError](#classyojimbo_1_1_network_transport_1adc76fe00e728b5847aa12e330dc65a77)

#### `public int GetError() const` 

Get the socket error code.

#### Returns
The socket error code. One of the values in [yojimbo::SocketError](#namespaceyojimbo_1a7d06104bf0ad6d55ace64fe1f1d03349) enum.


**See also**: [yojimbo::SocketError](#namespaceyojimbo_1a7d06104bf0ad6d55ace64fe1f1d03349)


**See also**: [NetworkTransport::IsError](#classyojimbo_1_1_network_transport_1a9fc95b7bb283a103ba9b2c219595982a)

#### `protected virtual void InternalSendPacket(const `[`Address`](#classyojimbo_1_1_address)` & to,const void * packetData,int packetBytes)` 

Overridden internal packet send function. Effectively just calls through to sendto.



#### `protected virtual int InternalReceivePacket(`[`Address`](#classyojimbo_1_1_address)` & from,void * packetData,int maxPacketSize)` 

Overriden internal packet receive function. Effectively just wraps recvfrom.



# class `yojimbo::Packet` 

```
class yojimbo::Packet
  : public yojimbo::Serializable
```  

A packet that can be serialized to a bit stream.



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  Packet()` | 
`public void Destroy()` | Destroy the packet.
`public inline bool IsValid() const` | Checks if the packet is valid.
`public inline int GetType() const` | Get the type of the packet.
`public inline `[`PacketFactory`](#classyojimbo_1_1_packet_factory)` & GetPacketFactory()` | Get the packet factory that was used to create this packet.
`protected inline void SetType(int type)` | Set the type of the packet factory.
`protected inline void SetPacketFactory(`[`PacketFactory`](#classyojimbo_1_1_packet_factory)` & packetFactory)` | Set the packet factory.
`protected inline virtual  ~Packet()` | [Packet](#classyojimbo_1_1_packet) destructor.

## Members

#### `public inline  Packet()` 





#### `public void Destroy()` 

Destroy the packet.

Unlike messages, packets are not reference counted. Call this method to destroy the packet. It takes care to call through to the message factory that created the packet for you.

IMPORTANT: If the packet lives longer than the packet that created it, the packet factory will assert that you have a packet leak its destructor, as a safety feature. Make sure you destroy all packets before cleaning up the packet factory!

#### `public inline bool IsValid() const` 

Checks if the packet is valid.

[Packet](#classyojimbo_1_1_packet) type is clearedh to -1 when a packet is destroyed, to aid with tracking down pointers to already deleted packets.


#### Returns
True if the packet is valid, false otherwise.

#### `public inline int GetType() const` 

Get the type of the packet.

Corresponds to the type value passed in to the packet factory when this packet object was created.


#### Returns
The packet type in [0,numTypes-1] as defined by the packet factory.


**See also**: [PacketFactory::Create](#classyojimbo_1_1_packet_factory_1a17dcd3f61ce38230fbe0bff253828b2f)

#### `public inline `[`PacketFactory`](#classyojimbo_1_1_packet_factory)` & GetPacketFactory()` 

Get the packet factory that was used to create this packet.

IMPORTANT: The packet factory must remain valid while packets created with it still exist. Make sure you destroy all packets before destroying the message factory that created them.


#### Returns
The packet factory.

#### `protected inline void SetType(int type)` 

Set the type of the packet factory.

Used internally by the packet factory to set the packet type on the packet object on creation.

Protected because a bunch of stuff would break if the user were to change the packet type dynamically.


#### Parameters
* `type` The packet type.

#### `protected inline void SetPacketFactory(`[`PacketFactory`](#classyojimbo_1_1_packet_factory)` & packetFactory)` 

Set the packet factory.

Used internally by the packet factory to set its own pointer on packets it creates, so those packets remember this and call back to the packet factory that created them when they are destroyed.

Protected because everything would break if the user were to modify the packet factory on an object after creation.


#### Parameters
* `packetFactory` The packet factory that created this packet.

#### `protected inline virtual  ~Packet()` 

[Packet](#classyojimbo_1_1_packet) destructor.

Protected because you need to call in to [Packet::Destroy](#classyojimbo_1_1_packet_1a597f853846f3d75213281625b25c9d16) to destroy this packet, instead of just deleting it, to make sure it gets cleaned up with the packet factory and allocator that created it.

# class `yojimbo::PacketFactory` 


Defines the set of packet types and a function to create packets.

Packets are not reference counted. They are typically added to send/receive queues, dequeued, processed and then destroyed.


**See also**: [Packet::Destroy](#classyojimbo_1_1_packet_1a597f853846f3d75213281625b25c9d16)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  PacketFactory(class `[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,int numTypes)` | The packet factory constructor.
`public virtual  ~PacketFactory()` | [Packet](#classyojimbo_1_1_packet) factory destructor.
`public `[`Packet`](#classyojimbo_1_1_packet)` * Create(int type)` | Create a packet by type.
`public int GetNumPacketTypes() const` | Get the number of packet types that can be created with this factory.
`public `[`PacketFactoryError`](#namespaceyojimbo_1abadc1a3f8801d3685b8c2ba89ccebb0f)` GetError() const` | Get the error level of the packet factory.
`public void ClearError()` | Clears the error level back to none.
`protected void DestroyPacket(`[`Packet`](#classyojimbo_1_1_packet)` * packet)` | Internal method to destroy a packet called by [Packet::Destroy](#classyojimbo_1_1_packet_1a597f853846f3d75213281625b25c9d16).
`protected void SetPacketType(`[`Packet`](#classyojimbo_1_1_packet)` * packet,int type)` | Set the packet type on a packet.
`protected void SetPacketFactory(`[`Packet`](#classyojimbo_1_1_packet)` * packet)` | Set the packet factory on a packet.
`protected `[`Allocator`](#classyojimbo_1_1_allocator)` & GetAllocator()` | Get the allocator used to create packets.
`protected inline virtual `[`Packet`](#classyojimbo_1_1_packet)` * CreatePacket(int type)` | Internal function used to create packets.

## Members

#### `public  PacketFactory(class `[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,int numTypes)` 

The packet factory constructor.

#### Parameters
* `allocator` The allocator used to create packets. 


* `numTypes` The number of packet types that can be created with this factory.

#### `public virtual  ~PacketFactory()` 

[Packet](#classyojimbo_1_1_packet) factory destructor.

IMPORTANT: You must destroy all packets created by this factory before you destroy it.

As a safety check, in debug builds the packet factory track packets created and will assert if you don't destroy them all before destroying the factory.

#### `public `[`Packet`](#classyojimbo_1_1_packet)` * Create(int type)` 

Create a packet by type.

IMPORTANT: Check the packet pointer returned by this call. It can be NULL if there is no memory to create a packet!


#### Parameters
* `type` The type of packet to create in [0,numTypes-1].





#### Returns
The packet object created. NULL if packet could not be created. You are responsible for destroying non-NULL packets via [Packet::Destroy](#classyojimbo_1_1_packet_1a597f853846f3d75213281625b25c9d16), or to pass ownership of the packte to some other function like [Transport::SendPacket](#classyojimbo_1_1_transport_1aa9c2dec77b36ddd19296c35533173b5f).

#### `public int GetNumPacketTypes() const` 

Get the number of packet types that can be created with this factory.

[Packet](#classyojimbo_1_1_packet) types that can be created are in [0,numTypes-1].


#### Returns
The number of packet types.

#### `public `[`PacketFactoryError`](#namespaceyojimbo_1abadc1a3f8801d3685b8c2ba89ccebb0f)` GetError() const` 

Get the error level of the packet factory.

If any packet fails to allocate, the error level is set to [yojimbo::PACKET_FACTORY_ERROR_FAILED_TO_ALLOCATE_PACKET](#namespaceyojimbo_1abadc1a3f8801d3685b8c2ba89ccebb0fa23bab28481eb9634a3b15a75f1769c26).


**See also**: [PacketFactory::ClearError](#classyojimbo_1_1_packet_factory_1a9cf041c5089d706a96f83d84334b3733)

#### `public void ClearError()` 

Clears the error level back to none.



#### `protected void DestroyPacket(`[`Packet`](#classyojimbo_1_1_packet)` * packet)` 

Internal method to destroy a packet called by [Packet::Destroy](#classyojimbo_1_1_packet_1a597f853846f3d75213281625b25c9d16).

This is done so packets can be destroyed unilaterally, without the user needing to remember which packet factory they need to be destroyed with.

This is important because the server has one global packet factory (for connection negotiation packets) and one packet factory per-client for security reasons.

It would be too much of a burden and too error prone to require the user to look-up the packet factory by client index for packets belonging to client connections.


#### Parameters
* `packet` The packet to destroy.

#### `protected void SetPacketType(`[`Packet`](#classyojimbo_1_1_packet)` * packet,int type)` 

Set the packet type on a packet.

Called by the packet factory to set the type of packets it has just created in [PacketFactory::Create](#classyojimbo_1_1_packet_factory_1a17dcd3f61ce38230fbe0bff253828b2f).


#### Parameters
* `packet` The packet to set the type on. 


* `type` The packet type to be set.

#### `protected void SetPacketFactory(`[`Packet`](#classyojimbo_1_1_packet)` * packet)` 

Set the packet factory on a packet.

Called by packet factory to set the packet factory on packets it has created in [PacketFactory::Create](#classyojimbo_1_1_packet_factory_1a17dcd3f61ce38230fbe0bff253828b2f), so those packets know how to destroy themselves.


#### Parameters
* `packet` The packet to set the this packet factory on.

#### `protected `[`Allocator`](#classyojimbo_1_1_allocator)` & GetAllocator()` 

Get the allocator used to create packets.

#### Returns
The allocator.

#### `protected inline virtual `[`Packet`](#classyojimbo_1_1_packet)` * CreatePacket(int type)` 

Internal function used to create packets.

This is typically overridden using helper macros instead of doing it manually.

See: YOJIMBO_PACKET_FACTORY_START
YOJIMBO_DECLARE_PACKET_TYPE
YOJIMBO_PACKET_FACTORY_FINISH


See tests/shared.h for an example of usage.


#### Parameters
* `type` The type of packet to create.





#### Returns
The packet created, or NULL if no packet could be created (eg. the allocator is out of memory).

# class `yojimbo::PacketProcessor` 


Adds packet encryption and decryption on top of low-level read and write packet functions.

**See also**: [yojimbo::WritePacket](#namespaceyojimbo_1a4eedda89dd51a1f7987c424777884a9d)


**See also**: [yojimbo::ReadPacket](#namespaceyojimbo_1a7d69c820c31d373bb009a7842f327b3f)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  PacketProcessor(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,uint64_t protocolId,int maxPacketSize)` | [Packet](#classyojimbo_1_1_packet) processor constructor.
`public  ~PacketProcessor()` | [Packet](#classyojimbo_1_1_packet) processor destructor.
`public void SetContext(void * context)` | Set a context to pass to the stream.
`public void SetUserContext(void * context)` | Set a context to pass to the stream.
`public const uint8_t * WritePacket(`[`Packet`](#classyojimbo_1_1_packet)` * packet,uint64_t sequence,int & packetBytes,bool encrypt,const uint8_t * key,`[`Allocator`](#classyojimbo_1_1_allocator)` & streamAllocator,`[`PacketFactory`](#classyojimbo_1_1_packet_factory)` & packetFactory)` | Write a packet.
`public `[`Packet`](#classyojimbo_1_1_packet)` * ReadPacket(const uint8_t * packetData,uint64_t & sequence,int packetBytes,bool & encrypted,const uint8_t * key,const uint8_t * encryptedPacketTypes,const uint8_t * unencryptedPacketTypes,`[`Allocator`](#classyojimbo_1_1_allocator)` & streamAllocator,`[`PacketFactory`](#classyojimbo_1_1_packet_factory)` & packetFactory,`[`ReplayProtection`](#classyojimbo_1_1_replay_protection)` * replayProtection)` | Read a packet.
`public inline int GetMaxPacketSize() const` | Gets the maximum packet size to be generated.
`public inline int GetError() const` | Get the packet processor error level.

## Members

#### `public  PacketProcessor(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,uint64_t protocolId,int maxPacketSize)` 

[Packet](#classyojimbo_1_1_packet) processor constructor.

#### Parameters
* `allocator` The allocator to use. 


* `protocolId` The protocol id that identifies your protocol. Typically a hash of your data and a protocol version number. 


* `maxPacketSize` The maximum packet size that can be written (bytes).

#### `public  ~PacketProcessor()` 

[Packet](#classyojimbo_1_1_packet) processor destructor.



#### `public void SetContext(void * context)` 

Set a context to pass to the stream.

**See also**: [BaseStream::SetContext](#classyojimbo_1_1_base_stream_1a55e354621abb33a4f55f05387f07720c)

#### `public void SetUserContext(void * context)` 

Set a context to pass to the stream.

**See also**: [BaseStream::SetUserContext](#classyojimbo_1_1_base_stream_1a97f89c78784d6b8c68f4efa95961f8da)

#### `public const uint8_t * WritePacket(`[`Packet`](#classyojimbo_1_1_packet)` * packet,uint64_t sequence,int & packetBytes,bool encrypt,const uint8_t * key,`[`Allocator`](#classyojimbo_1_1_allocator)` & streamAllocator,`[`PacketFactory`](#classyojimbo_1_1_packet_factory)` & packetFactory)` 

Write a packet.

#### Parameters
* `packet` The packet to write. 


* `sequence` The sequence number of the packet. Used as the nonce for encrypted packets. Ignored for unencrypted packets. 


* `packetBytes` The number of bytes of packet data written [out]. 


* `encrypt` Should this packet be encrypted? 


* `key` The key used for packet encryption. 


* `streamAllocator` The allocator to set on the stream. See [BaseStream::GetAllocator](#classyojimbo_1_1_base_stream_1ab569e42a19a6262ce95e98c7f969f903). 


* `packetFactory` The packet factory so we know the range of packet types supported.





#### Returns
A pointer to the packet data written. NULL if the packet write failed. This is an internal scratch buffer. Do not cache it and do not free it.

#### `public `[`Packet`](#classyojimbo_1_1_packet)` * ReadPacket(const uint8_t * packetData,uint64_t & sequence,int packetBytes,bool & encrypted,const uint8_t * key,const uint8_t * encryptedPacketTypes,const uint8_t * unencryptedPacketTypes,`[`Allocator`](#classyojimbo_1_1_allocator)` & streamAllocator,`[`PacketFactory`](#classyojimbo_1_1_packet_factory)` & packetFactory,`[`ReplayProtection`](#classyojimbo_1_1_replay_protection)` * replayProtection)` 

Read a packet.

#### Parameters
* `packetData` The packet data to read. 


* `sequence` The packet sequence number [out]. Only set for encrypted packets. Set to 0 for unencrypted packets. 


* `packetBytes` The number of bytes of packet data to read. 


* `encrypted` Set to true if the packet is encrypted [out]. 


* `key` The key used to decrypt the packet, if it is encrypted. 


* `encryptedPacketTypes` Entry n is 1 if packet type n is encrypted. Passed into the low-level packet read as the set of allowed packet types, if the packet is encrypted. 


* `unencryptedPacketTypes` Entry n is 1 if packet type n is unencrypted. Passed into the low-level packet read as the set of allowed packet types, if the packet is not encrypted. 


* `streamAllocator` The allocator to set on the steram. See [BaseStream::GetAllocator](#classyojimbo_1_1_base_stream_1ab569e42a19a6262ce95e98c7f969f903). 


* `packetFactory` The packet factory used to create the packet. 


* `replayProtection` The replay protection buffer. Optional. Pass in NULL if not used.





#### Returns
The packet object if it was sucessfully read, NULL otherwise. You are responsible for destroying the packet created by this function.

#### `public inline int GetMaxPacketSize() const` 

Gets the maximum packet size to be generated.

#### Returns
The maximum packet size in bytes.

#### `public inline int GetError() const` 

Get the packet processor error level.

Use this to work out why read or write packet functions failed.


#### Returns
The packet processor error code.


**See also**: [PacketProcessor::ReadPacket](#classyojimbo_1_1_packet_processor_1aac6a8b1b8327c8cf052a0d842165bea3)


**See also**: [PacketProcessor::WritePacket](#classyojimbo_1_1_packet_processor_1a09ac3401d4e4c7c8591ba730ee1c59d9)

# class `yojimbo::Queue` 


A simple templated queue.

This is a FIFO queue. First entry in, first entry out.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  Queue(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,int size)` | [Queue](#classyojimbo_1_1_queue) constructor.
`public inline  ~Queue()` | [Queue](#classyojimbo_1_1_queue) destructor.
`public inline void Clear()` | Clear all entries in the queue and reset back to default state.
`public inline T Pop()` | Pop a value off the queue.
`public inline void Push(const T & value)` | Push a value on to the queue.
`public inline T & operator[](int index)` | Random access for entries in the queue.
`public inline const T & operator[](int index) const` | Random access for entries in the queue (const version).
`public inline int GetSize() const` | Get the size of the queue.
`public inline bool IsFull() const` | Is the queue currently full?
`public inline bool IsEmpty() const` | Is the queue currently empty?
`public inline int GetNumEntries() const` | Get the number of entries in the queue.

## Members

#### `public inline  Queue(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,int size)` 

[Queue](#classyojimbo_1_1_queue) constructor.

#### Parameters
* `allocator` The allocator to use. 


* `size` The maximum number of entries in the queue.

#### `public inline  ~Queue()` 

[Queue](#classyojimbo_1_1_queue) destructor.



#### `public inline void Clear()` 

Clear all entries in the queue and reset back to default state.



#### `public inline T Pop()` 

Pop a value off the queue.

IMPORTANT: This will assert if the queue is empty. Check [Queue::IsEmpty](#classyojimbo_1_1_queue_1a306b5368e4a1b5b80a41e9b292fbedae) or [Queue::GetNumEntries](#classyojimbo_1_1_queue_1a0e51a62024f925852e38b7cfaedebd70) first!


#### Returns
The value popped off the queue.

#### `public inline void Push(const T & value)` 

Push a value on to the queue.

#### Parameters
* `value` The value to push onto the queue.





IMPORTANT: Will assert if the queue is already full. Check [Queue::IsFull](#classyojimbo_1_1_queue_1a21a229b7d316c3eb3767161eaacfaa21) before calling this!

#### `public inline T & operator[](int index)` 

Random access for entries in the queue.

#### Parameters
* `index` The index into the queue. 0 is the oldest entry, [Queue::GetNumEntries()](#classyojimbo_1_1_queue_1a0e51a62024f925852e38b7cfaedebd70) - 1 is the newest.





#### Returns
The value in the queue at the index.

#### `public inline const T & operator[](int index) const` 

Random access for entries in the queue (const version).

#### Parameters
* `index` The index into the queue. 0 is the oldest entry, [Queue::GetNumEntries()](#classyojimbo_1_1_queue_1a0e51a62024f925852e38b7cfaedebd70) - 1 is the newest.





#### Returns
The value in the queue at the index.

#### `public inline int GetSize() const` 

Get the size of the queue.

This is the maximum number of values that can be pushed on the queue.


#### Returns
The size of the queue.

#### `public inline bool IsFull() const` 

Is the queue currently full?

#### Returns
True if the queue is full. False otherwise.

#### `public inline bool IsEmpty() const` 

Is the queue currently empty?

#### Returns
True if there are no entries in the queue.

#### `public inline int GetNumEntries() const` 

Get the number of entries in the queue.

#### Returns
The number of entries in the queue in [0,[GetSize()](#classyojimbo_1_1_queue_1aeec2c4bb3aa858788c44b046e79fc3e9)].

# class `yojimbo::ReadStream` 

```
class yojimbo::ReadStream
  : public yojimbo::BaseStream
```  

Stream class for reading bitpacked data.

This class is a wrapper around the bit reader class. Its purpose is to provide unified interface for reading and writing.

You can determine if you are reading from a stream by calling Stream::IsReading inside your templated serialize method.

This is evaluated at compile time, letting the compiler generate optimized serialize functions without the hassle of maintaining separate read and write functions.

IMPORTANT: Generally, you don't call methods on this class directly. Use the serialize_* macros instead. See test/shared.h for some examples.


**See also**: [BitReader](#classyojimbo_1_1_bit_reader)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  ReadStream(const uint8_t * buffer,int bytes,`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator)` | Read stream constructor.
`public inline bool SerializeInteger(int32_t & value,int32_t min,int32_t max)` | Serialize an integer (read).
`public inline bool SerializeBits(uint32_t & value,int bits)` | Serialize a number of bits (read).
`public inline bool SerializeBytes(uint8_t * data,int bytes)` | Serialize an array of bytes (read).
`public inline bool SerializeAlign()` | Serialize an align (read).
`public inline int GetAlignBits() const` | If we were to read an align right now, how many bits would we need to read?
`public inline bool SerializeCheck()` | Serialize a safety check from the stream (read).
`public inline int GetBitsProcessed() const` | Get number of bits read so far.
`public inline int GetBytesProcessed() const` | How many bytes have been read so far?

## Members

#### `public inline  ReadStream(const uint8_t * buffer,int bytes,`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator)` 

Read stream constructor.

#### Parameters
* `buffer` The buffer to read from. 


* `bytes` The number of bytes in the buffer. May be a non-multiple of four, however if it is, the underlying buffer allocated should be large enough to read the any remainder bytes as a dword. 


* `allocator` The allocator to use for stream allocations. This lets you dynamically allocate memory as you read and write packets.

#### `public inline bool SerializeInteger(int32_t & value,int32_t min,int32_t max)` 

Serialize an integer (read).

#### Parameters
* `value` The integer value read is stored here. It is guaranteed to be in [min,max] if this function succeeds. 


* `min` The minimum allowed value. 


* `max` The maximum allowed value.





#### Returns
Returns true if the serialize succeeded and the value is in the correct range. False otherwise.

#### `public inline bool SerializeBits(uint32_t & value,int bits)` 

Serialize a number of bits (read).

#### Parameters
* `value` The integer value read is stored here. Will be in range [0,(1<<bits)-1]. 


* `bits` The number of bits to read in [1,32].





#### Returns
Returns true if the serialize read succeeded, false otherwise.

#### `public inline bool SerializeBytes(uint8_t * data,int bytes)` 

Serialize an array of bytes (read).

#### Parameters
* `data` Array of bytes to read. 


* `bytes` The number of bytes to read.





#### Returns
Returns true if the serialize read succeeded. False otherwise.

#### `public inline bool SerializeAlign()` 

Serialize an align (read).

#### Returns
Returns true if the serialize read succeeded. False otherwise.

#### `public inline int GetAlignBits() const` 

If we were to read an align right now, how many bits would we need to read?

#### Returns
The number of zero pad bits required to achieve byte alignment in [0,7].

#### `public inline bool SerializeCheck()` 

Serialize a safety check from the stream (read).

Safety checks help track down desyncs. A check is written to the stream, and on the other side if the check is not present it asserts and fails the serialize.


#### Returns
Returns true if the serialize check passed. False otherwise.

#### `public inline int GetBitsProcessed() const` 

Get number of bits read so far.

#### Returns
Number of bits read.

#### `public inline int GetBytesProcessed() const` 

How many bytes have been read so far?

#### Returns
Number of bytes read. Effectively this is the number of bits read, rounded up to the next byte where necessary.

# class `yojimbo::ReliableOrderedChannel` 

```
class yojimbo::ReliableOrderedChannel
  : public yojimbo::Channel
```  

Messages sent across this channel are guaranteed to arrive in the order they were sent.

This channel type is best used for control messages and RPCs.

Messages sent over this channel are included in connection packets until one of those packets is acked. Messages are acked individually and remain in the send queue until acked.

Blocks attached to messages sent over this channel are split up into fragments. Each fragment of the block is included in a connection packet until one of those packets are acked. Eventually, all fragments are received on the other side, and block is reassembled and attached to the message.

Only one message block may be in flight over the network at any time, so blocks stall out message delivery slightly. Therefore, only use blocks for large data that won't fit inside a single connection packet where you actually need the channel to split it up into fragments. If your block fits inside a packet, just serialize it inside your message serialize via serialize_bytes instead.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  ReliableOrderedChannel(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,`[`MessageFactory`](#classyojimbo_1_1_message_factory)` & messageFactory,const `[`ChannelConfig`](#structyojimbo_1_1_channel_config)` & config,int channelId)` | Reliable ordered channel constructor.
`public  ~ReliableOrderedChannel()` | Reliable ordered channel destructor.
`public virtual void Reset()` | Reset the channel.
`public virtual bool CanSendMsg() const` | Returns true if a message can be sent over this channel.
`public virtual void SendMsg(`[`Message`](#classyojimbo_1_1_message)` * message)` | [Queue](#classyojimbo_1_1_queue) a message to be sent across this channel.
`public virtual `[`Message`](#classyojimbo_1_1_message)` * ReceiveMsg()` | Pops the next message off the receive queue if one is available.
`public virtual void AdvanceTime(double time)` | Advance channel time.
`public virtual int GetPacketData(`[`ChannelPacketData`](#structyojimbo_1_1_channel_packet_data)` & packetData,uint16_t packetSequence,int availableBits)` | Get channel packet data for this channel.
`public virtual void ProcessPacketData(const `[`ChannelPacketData`](#structyojimbo_1_1_channel_packet_data)` & packetData,uint16_t packetSequence)` | Process packet data included in a connection packet.
`public virtual void ProcessAck(uint16_t sequence)` | Process a connection packet ack.
`public bool HasMessagesToSend() const` | Are there any unacked messages in the send queue?
`public int GetMessagesToSend(uint16_t * messageIds,int & numMessageIds,int remainingPacketBits)` | Get messages to include in a packet.
`public void GetMessagePacketData(`[`ChannelPacketData`](#structyojimbo_1_1_channel_packet_data)` & packetData,const uint16_t * messageIds,int numMessageIds)` | Fill channel packet data with messages.
`public void AddMessagePacketEntry(const uint16_t * messageIds,int numMessageIds,uint16_t sequence)` | Add a packet entry for the set of messages included in a packet.
`public void ProcessPacketMessages(int numMessages,`[`Message`](#classyojimbo_1_1_message)` ** messages)` | Process messages included in a packet.
`public void UpdateOldestUnackedMessageId()` | Track the oldest unacked message id in the send queue.
`public bool SendingBlockMessage()` | True if we are currently sending a block message.
`public uint8_t * GetFragmentToSend(uint16_t & messageId,uint16_t & fragmentId,int & fragmentBytes,int & numFragments,int & messageType)` | Get the next block fragment to send.
`public int GetFragmentPacketData(`[`ChannelPacketData`](#structyojimbo_1_1_channel_packet_data)` & packetData,uint16_t messageId,uint16_t fragmentId,uint8_t * fragmentData,int fragmentSize,int numFragments,int messageType)` | Fill the packet data with block and fragment data.
`public void AddFragmentPacketEntry(uint16_t messageId,uint16_t fragmentId,uint16_t sequence)` | Adds a packet entry for the fragment.
`public void ProcessPacketFragment(int messageType,uint16_t messageId,int numFragments,uint16_t fragmentId,const uint8_t * fragmentData,int fragmentBytes,`[`BlockMessage`](#classyojimbo_1_1_block_message)` * blockMessage)` | Process a packet fragment.

## Members

#### `public  ReliableOrderedChannel(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,`[`MessageFactory`](#classyojimbo_1_1_message_factory)` & messageFactory,const `[`ChannelConfig`](#structyojimbo_1_1_channel_config)` & config,int channelId)` 

Reliable ordered channel constructor.

#### Parameters
* `allocator` The allocator to use. 


* `messageFactory` [Message](#classyojimbo_1_1_message) factory for creating and destroying messages. 


* `config` The configuration for this channel. 


* `channelId` The channel id in [0,numChannels-1].

#### `public  ~ReliableOrderedChannel()` 

Reliable ordered channel destructor.

Any messages still in the send or receive queues will be released.

#### `public virtual void Reset()` 

Reset the channel.



#### `public virtual bool CanSendMsg() const` 

Returns true if a message can be sent over this channel.



#### `public virtual void SendMsg(`[`Message`](#classyojimbo_1_1_message)` * message)` 

[Queue](#classyojimbo_1_1_queue) a message to be sent across this channel.

#### Parameters
* `message` The message to be sent.

#### `public virtual `[`Message`](#classyojimbo_1_1_message)` * ReceiveMsg()` 

Pops the next message off the receive queue if one is available.

#### Returns
A pointer to the received message, NULL if there are no messages to receive. The caller owns the message object returned by this function and is responsible for releasing it via [Message::Release](#classyojimbo_1_1_message_1a5979f1559786c1606dde8df944b24d26).

#### `public virtual void AdvanceTime(double time)` 

Advance channel time.

Called by [Connection::AdvanceTime](#classyojimbo_1_1_connection_1a268d70c87da4a4185bfe6b947b8e7b7a) for each channel configured on the connection.

#### `public virtual int GetPacketData(`[`ChannelPacketData`](#structyojimbo_1_1_channel_packet_data)` & packetData,uint16_t packetSequence,int availableBits)` 

Get channel packet data for this channel.

#### Parameters
* `packetData` The channel packet data to be filled [out] 


* `packetSequence` The sequence number of the packet being generated. 


* `availableBits` The maximum number of bits of packet data the channel is allowed to write.





#### Returns
The number of bits of packet data written by the channel.


**See also**: [ConnectionPacket](#structyojimbo_1_1_connection_packet)


**See also**: [Connection::GeneratePacket](#classyojimbo_1_1_connection_1aaf8a19bcdd9ad05ff598b6e975881290)

#### `public virtual void ProcessPacketData(const `[`ChannelPacketData`](#structyojimbo_1_1_channel_packet_data)` & packetData,uint16_t packetSequence)` 

Process packet data included in a connection packet.

#### Parameters
* `packetData` The channel packet data to process. 


* `packetSequence` The sequence number of the connection packet that contains the channel packet data.





**See also**: [ConnectionPacket](#structyojimbo_1_1_connection_packet)


**See also**: [Connection::ProcessPacket](#classyojimbo_1_1_connection_1a6118515c3a0f1764e5f27a378df87886)

#### `public virtual void ProcessAck(uint16_t sequence)` 

Process a connection packet ack.

Depending on the channel type: 1. Acks messages and block fragments so they stop being included in outgoing connection packets (reliable-ordered channel)

2. Does nothing at all (unreliable-unordered).



#### Parameters
* `sequence` The sequence number of the connection packet that was acked.

#### `public bool HasMessagesToSend() const` 

Are there any unacked messages in the send queue?

Messages are acked individually and remain in the send queue until acked.


#### Returns
True if there is at least one unacked message in the send queue.

#### `public int GetMessagesToSend(uint16_t * messageIds,int & numMessageIds,int remainingPacketBits)` 

Get messages to include in a packet.

Messages are measured to see how many bits they take, and only messages that fit within the channel packet budget will be included. See [ChannelConfig::packetBudget](#structyojimbo_1_1_channel_config_1a550d8d3ecde415a83a8c5c4ff092f872).

Takes care not to send messages too rapidly by respecting [ChannelConfig::messageResendTime](#structyojimbo_1_1_channel_config_1a4cee28ed7fe1b7df30c715ae9740db67) for each message, and to only include messages that that the receiver is able to buffer in their receive queue. In other words, won't run ahead of the receiver.


#### Parameters
* `messageIds` Array of message ids to be filled [out]. Fills up to [ChannelConfig::maxMessagesPerPacket](#structyojimbo_1_1_channel_config_1a9d2ddcfa107a7768a42da69419d72b56) messages, make sure your array is at least this size. 


* `numMessageIds` The number of message ids written to the array. 


* `remainingPacketBits` Number of bits remaining in the packet. Considers this as a hard limit when determining how many messages can fit into the packet.





#### Returns
Estimate of the number of bits required to serialize the messages (upper bound).


**See also**: [GetMessagePacketData](#classyojimbo_1_1_reliable_ordered_channel_1acf8d74071c1b3cba2924dd714a01646c)

#### `public void GetMessagePacketData(`[`ChannelPacketData`](#structyojimbo_1_1_channel_packet_data)` & packetData,const uint16_t * messageIds,int numMessageIds)` 

Fill channel packet data with messages.

This is the payload function to fill packet data while sending regular messages (without blocks attached).

Messages have references added to them when they are added to the packet. They also have a reference while they are stored in a send or receive queue. Messages are cleaned up when they are no longer in a queue, and no longer referenced by any packets.


#### Parameters
* `packetData` The packet data to fill [out] 


* `messageIds` Array of message ids identifying which messages to add to the packet from the message send queue. 


* `numMessageIds` The number of message ids in the array.





**See also**: [GetMessagesToSend](#classyojimbo_1_1_reliable_ordered_channel_1ad39d11496b412d7ef1952017d08d5159)

#### `public void AddMessagePacketEntry(const uint16_t * messageIds,int numMessageIds,uint16_t sequence)` 

Add a packet entry for the set of messages included in a packet.

This lets us look up the set of messages that were included in that packet later on when it is acked, so we can ack those messages individually.


#### Parameters
* `messageIds` The set of message ids that were included in the packet. 


* `numMessageIds` The number of message ids in the array. 


* `sequence` The sequence number of the connection packet the messages were included in.

#### `public void ProcessPacketMessages(int numMessages,`[`Message`](#classyojimbo_1_1_message)` ** messages)` 

Process messages included in a packet.

Any messages that have not already been received are added to the message receive queue. Messages that are added to the receive queue have a reference added. See [Message::AddRef](#classyojimbo_1_1_message_1a0bd869ee8e6b8392eb6108ca8e96e4d4).


#### Parameters
* `numMessages` The number of messages to process. 


* `messages` Array of pointers to messages.

#### `public void UpdateOldestUnackedMessageId()` 

Track the oldest unacked message id in the send queue.

Because messages are acked individually, the send queue is not a true queue and may have holes.

Because of this it is necessary to periodically walk forward from the previous oldest unacked message id, to find the current oldest unacked message id.

This lets us know our starting point for considering messages to include in the next packet we send.


**See also**: [GetMessagesToSend](#classyojimbo_1_1_reliable_ordered_channel_1ad39d11496b412d7ef1952017d08d5159)

#### `public bool SendingBlockMessage()` 

True if we are currently sending a block message.

Block messages are treated differently to regular messages.

Regular messages are small so we try to fit as many into the packet we can. See ReliableChannelData::GetMessagesToSend.

Blocks attached to block messages are usually larger than the maximum packet size or channel budget, so they are split up fragments.

While in the mode of sending a block message, each channel packet data generated has exactly one fragment from the current block in it. Fragments keep getting included in packets until all fragments of that block are acked.


#### Returns
True if currently sending a block message over the network, false otherwise.


**See also**: [BlockMessage](#classyojimbo_1_1_block_message)


**See also**: [GetFragmentToSend](#classyojimbo_1_1_reliable_ordered_channel_1a7a6858f645ecdaf3b359d481db24d05f)

#### `public uint8_t * GetFragmentToSend(uint16_t & messageId,uint16_t & fragmentId,int & fragmentBytes,int & numFragments,int & messageType)` 

Get the next block fragment to send.

The next block fragment is selected by scanning left to right over the set of fragments in the block, skipping over any fragments that have already been acked or have been sent within [ChannelConfig::fragmentResendTime](#structyojimbo_1_1_channel_config_1a63d507729312ce04a1ce553c67b57c8a).


#### Parameters
* `messageId` The id of the message that the block is attached to [out]. 


* `fragmentId` The id of the fragment to send [out]. 


* `fragmentBytes` The size of the fragment in bytes. 


* `numFragments` The total number of fragments in this block. 


* `messageType` The type of message the block is attached to. See [MessageFactory](#classyojimbo_1_1_message_factory).





#### Returns
Pointer to the fragment data.

#### `public int GetFragmentPacketData(`[`ChannelPacketData`](#structyojimbo_1_1_channel_packet_data)` & packetData,uint16_t messageId,uint16_t fragmentId,uint8_t * fragmentData,int fragmentSize,int numFragments,int messageType)` 

Fill the packet data with block and fragment data.

This is the payload function that fills the channel packet data while we are sending a block message.


#### Parameters
* `packetData` The packet data to fill [out] 


* `messageId` The id of the message that the block is attached to. 


* `fragmentId` The id of the block fragment being sent. 


* `fragmentData` The fragment data. 


* `fragmentSize` The size of the fragment data (bytes). 


* `numFragments` The number of fragments in the block. 


* `messageType` The type of message the block is attached to.





#### Returns
An estimate of the number of bits required to serialize the block message and fragment data (upper bound).

#### `public void AddFragmentPacketEntry(uint16_t messageId,uint16_t fragmentId,uint16_t sequence)` 

Adds a packet entry for the fragment.

This lets us look up the fragment that was in the packet later on when it is acked, so we can ack that block fragment.


#### Parameters
* `messageId` The message id that the block was attached to. 


* `fragmentId` The fragment id. 


* `sequence` The sequence number of the packet the fragment was included in.

#### `public void ProcessPacketFragment(int messageType,uint16_t messageId,int numFragments,uint16_t fragmentId,const uint8_t * fragmentData,int fragmentBytes,`[`BlockMessage`](#classyojimbo_1_1_block_message)` * blockMessage)` 

Process a packet fragment.

The fragment is added to the set of received fragments for the block. When all packet fragments are received, that block is reconstructed, attached to the block message and added to the message receive queue.


#### Parameters
* `messageType` The type of the message this block fragment is attached to. This is used to make sure this message type actually allows blocks to be attached to it. 


* `messageId` The id of the message the block fragment belongs to. 


* `numFragments` The number of fragments in the block. 


* `fragmentId` The id of the fragment in [0,numFragments-1]. 


* `fragmentData` The fragment data. 


* `fragmentBytes` The size of the fragment data in bytes. 


* `blockMessage` Pointer to the block message. Passed this in only with the first fragment (0), pass NULL for all other fragments.

# class `yojimbo::ReplayProtection` 


Provides protection against packets being sniffed and replayed.

Basically a ring buffer that stores packets indexed by packet sequence number modulo replay buffer size.

The logic is pretty much: 1. If a packet is older than what can be stored in the buffer, ignore it. 

2. If an entry in the reply buffer exists and matches the packet sequence, it's already been received, so ignore it.

3. Otherwise, this is the first time the packet has been received, so let it in.


The whole point is to avoid the possibility of an attacker capturing and replaying encrypted packets, in an attempt to break some internal protocol state, like packet level acks or reliable-messages.

Without this protection, that would be reasonably easy to do. Just capture a packet and then keep replaying it. Eventually the packet or message sequence number would wrap around and you'd corrupt the connection state for that client.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  ReplayProtection()` | Replay protection constructor.
`public inline void Reset(uint64_t mostRecentSequence)` | Reset the replay protection buffer at a particular sequence number.
`public inline bool PacketAlreadyReceived(uint64_t sequence)` | Has a packet already been received with this sequence?
`public inline uint64_t GetMostRecentSequence() const` | Get the most recent packet sequence number received.
`protected uint64_t m_mostRecentSequence` | The most recent sequence number received.
`protected uint64_t m_receivedPacket` | The ring buffer that stores packet sequence numbers at index sequence modulo buffer size.

## Members

#### `public inline  ReplayProtection()` 

Replay protection constructor.

Starts at packet sequence number 0.

#### `public inline void Reset(uint64_t mostRecentSequence)` 

Reset the replay protection buffer at a particular sequence number.

#### Parameters
* `mostRecentSequence` The sequence number to start at, defaulting to zero.

#### `public inline bool PacketAlreadyReceived(uint64_t sequence)` 

Has a packet already been received with this sequence?

IMPORTANT: Global packets sent by the server (packets that don't correspond to any connected client) have the high bit of the sequence number set to 1. These packets should NOT have replay protection applied to them.

This is not a problem in practice, as modification of the packet sequence number by an attacker would cause it to fail to decrypt (the sequence number is the nonce).


#### Parameters
* `sequence` The packet sequence number.





#### Returns
True if you should ignore this packet, because it's potentially a replay attack. False if it's OK to process the packet.

#### `public inline uint64_t GetMostRecentSequence() const` 

Get the most recent packet sequence number received.

Packets older than this sequence number minus [yojimbo::ReplayProtectionBufferSize](#namespaceyojimbo_1a28f842e78883c1086181638520a77045) are discarded. This way we can keep the replay protection buffer small (about one seconds worth).


#### Returns
The most recent packet sequence number.

#### `protected uint64_t m_mostRecentSequence` 

The most recent sequence number received.



#### `protected uint64_t m_receivedPacket` 

The ring buffer that stores packet sequence numbers at index sequence modulo buffer size.



# class `yojimbo::SequenceBuffer` 


Data structure that stores data indexed by sequence number.

Entries may or may not exist. If they don't exist the sequence value for the entry at that index is set to 0xFFFFFFFF.

This provides a constant time lookup for an entry by sequence number. If the entry at sequence modulo buffer size doesn't have the same sequence number, that sequence number is not stored.

This is incredibly useful and is used as the foundation of the packet level ack system and the reliable message send and receive queues.


**See also**: [Connection](#classyojimbo_1_1_connection)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  SequenceBuffer(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,int size)` | Sequence buffer constructor.
`public inline  ~SequenceBuffer()` | Sequence buffer destructor.
`public inline void Reset()` | Reset the sequence buffer.
`public inline T * Insert(uint16_t sequence)` | Insert an entry in the sequence buffer.
`public inline void Remove(uint16_t sequence)` | Remove an entry from the sequence buffer.
`public inline bool Available(uint16_t sequence) const` | Is the entry corresponding to the sequence number available? eg.
`public inline bool Exists(uint16_t sequence) const` | Does an entry exists for a sequence number?
`public inline T * Find(uint16_t sequence)` | Get the entry corresponding to a sequence number.
`public inline const T * Find(uint16_t sequence) const` | Get the entry corresponding to a sequence number (const version).
`public inline T * GetAtIndex(int index)` | Get the entry at the specified index.
`public inline uint16_t GetSequence() const` | Get the most recent sequence number added to the buffer.
`public inline int GetIndex(uint16_t sequence) const` | Get the entry index for a sequence number.
`public inline int GetSize() const` | Get the size of the sequence buffer.
`protected inline void RemoveEntries(int start_sequence,int finish_sequence)` | Helper function to remove entries.

## Members

#### `public inline  SequenceBuffer(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,int size)` 

Sequence buffer constructor.

#### Parameters
* `allocator` The allocator to use. 


* `size` The size of the sequence buffer.

#### `public inline  ~SequenceBuffer()` 

Sequence buffer destructor.



#### `public inline void Reset()` 

Reset the sequence buffer.

Removes all entries from the sequence buffer and restores it to initial state.

#### `public inline T * Insert(uint16_t sequence)` 

Insert an entry in the sequence buffer.

IMPORTANT: If another entry exists at the sequence modulo buffer size, it is overwritten.


#### Parameters
* `sequence` The sequence number.





#### Returns
The sequence buffer entry, which you must fill with your data. NULL if a sequence buffer entry could not be added for your sequence number (if the sequence number is too old for example).

#### `public inline void Remove(uint16_t sequence)` 

Remove an entry from the sequence buffer.

#### Parameters
* `sequence` The sequence number of the entry to remove.

#### `public inline bool Available(uint16_t sequence) const` 

Is the entry corresponding to the sequence number available? eg.

Currently unoccupied.

This works because older entries are automatically set back to unoccupied state as the sequence buffer advances forward.


#### Parameters
* `sequence` The sequence number.





#### Returns
True if the sequence buffer entry is available, false if it is already occupied.

#### `public inline bool Exists(uint16_t sequence) const` 

Does an entry exists for a sequence number?

#### Parameters
* `sequence` The sequence number.





#### Returns
True if an entry exists for this sequence number.

#### `public inline T * Find(uint16_t sequence)` 

Get the entry corresponding to a sequence number.

#### Parameters
* `sequence` The sequence number.





#### Returns
The entry if it exists. NULL if no entry is in the buffer for this sequence number.

#### `public inline const T * Find(uint16_t sequence) const` 

Get the entry corresponding to a sequence number (const version).

#### Parameters
* `sequence` The sequence number.





#### Returns
The entry if it exists. NULL if no entry is in the buffer for this sequence number.

#### `public inline T * GetAtIndex(int index)` 

Get the entry at the specified index.

Use this to iterate across entries in the sequence buffer.


#### Parameters
* `index` The entry index in [0,[GetSize()](#classyojimbo_1_1_sequence_buffer_1a84992bcc8abf980c3693be1a33f8e3e9)-1].





#### Returns
The entry if it exists. NULL if no entry is in the buffer at the specified index.

#### `public inline uint16_t GetSequence() const` 

Get the most recent sequence number added to the buffer.

This sequence number can wrap around, so if you are at 65535 and add an entry for sequence 0, then 0 becomes the new "most recent" sequence number.


#### Returns
The most recent sequence nmuber.


**See also**: [yojimbo::sequence_greater_than](#namespaceyojimbo_1ae1b7e3deef46222ecf7b3620838c7459)


**See also**: [yojimbo::sequence_less_than](#namespaceyojimbo_1ad45f4fee5c716e4e1e057eebb9597689)

#### `public inline int GetIndex(uint16_t sequence) const` 

Get the entry index for a sequence number.

This is simply the sequence number modulo the sequence buffer size.


#### Parameters
* `sequence` The sequence number.





#### Returns
The sequence buffer index corresponding of the sequence number.

#### `public inline int GetSize() const` 

Get the size of the sequence buffer.

#### Returns
The size of the sequence buffer (number of entries).

#### `protected inline void RemoveEntries(int start_sequence,int finish_sequence)` 

Helper function to remove entries.

This is used to remove old entries as we advance the sequence buffer forward.

Otherwise, if when entries are added with holes (eg. receive buffer for packets or messages, where not all sequence numbers are added to the buffer because we have high packet loss), and we are extremely unlucky, we can have old sequence buffer entries from the previous sequence # wrap around still in the buffer, which corrupts our internal connection state.

This actually happens in the soak test at high packet loss levels (>90%). It took me days to track it down :)

# class `yojimbo::Serializable` 


Interface for an object that knows how to read, write and measure how many bits it would take up in a bit stream.

IMPORTANT: Instead of overridding the serialize virtual methods method directly, use the YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS macro in your derived class to override and redirect them to your templated serialize method.

This way you can implement read and write for your messages in a single method and the C++ compiler takes care of generating specialized read, write and measure implementations for you.

See tests/shared.h for some examples of this.


**See also**: [ReadStream](#classyojimbo_1_1_read_stream)


**See also**: [WriteStream](#classyojimbo_1_1_write_stream)


**See also**: [MeasureStream](#classyojimbo_1_1_measure_stream)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline virtual  ~Serializable()` | 
`public bool SerializeInternal(class `[`ReadStream`](#classyojimbo_1_1_read_stream)` & stream)` | Virtual serialize function (read).
`public bool SerializeInternal(class `[`WriteStream`](#classyojimbo_1_1_write_stream)` & stream)` | Virtual serialize function (write).
`public bool SerializeInternal(class `[`MeasureStream`](#classyojimbo_1_1_measure_stream)` & stream)` | Virtual serialize function (measure).

## Members

#### `public inline virtual  ~Serializable()` 





#### `public bool SerializeInternal(class `[`ReadStream`](#classyojimbo_1_1_read_stream)` & stream)` 

Virtual serialize function (read).

Reads the object in from a bitstream.


#### Parameters
* `stream` The stream to read from.

#### `public bool SerializeInternal(class `[`WriteStream`](#classyojimbo_1_1_write_stream)` & stream)` 

Virtual serialize function (write).

Writes the object to a bitstream.


#### Parameters
* `stream` The stream to write to.

#### `public bool SerializeInternal(class `[`MeasureStream`](#classyojimbo_1_1_measure_stream)` & stream)` 

Virtual serialize function (measure).

Quickly measures how many bits the object would take if it were written to a bit stream.


#### Parameters
* `stream` The read stream.

# class `yojimbo::Server` 

```
class yojimbo::Server
  : public yojimbo::ConnectionListener
```  

A server with n slots for clients to connect to.

This class is designed to be inherited from to create your own server class.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  Server(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,`[`Transport`](#classyojimbo_1_1_transport)` & transport,const `[`ClientServerConfig`](#structyojimbo_1_1_client_server_config)` & config,double time)` | The server constructor.
`public virtual  ~Server()` | The server destructor.
`public void SetPrivateKey(const uint8_t * privateKey)` | Set the private key used to decrypt connect tokens.
`public void SetUserContext(void * context)` | Set the user context.
`public void SetServerAddress(const `[`Address`](#classyojimbo_1_1_address)` & address)` | Set the server IP address.
`public void Start(int maxClients)` | Start the server and allocate client slots.
`public void Stop()` | Stop the server and free client slots.
`public void DisconnectClient(int clientIndex,bool sendDisconnectPacket)` | Disconnect the client at the specified client index.
`public void DisconnectAllClients(bool sendDisconnectPacket)` | Disconnect all clients from the server.
`public void SendPackets()` | Send packets to connected clients.
`public void ReceivePackets()` | Receive packets sent from potential and connected clients.
`public void CheckForTimeOut()` | Check for timeouts.
`public void AdvanceTime(double time)` | Advance server time.
`public bool IsRunning() const` | Is the server running?
`public int GetMaxClients() const` | Get the maximum number of clients that can connect to the server.
`public bool IsClientConnected(int clientIndex) const` | Is a client connected to a client slot?
`public int GetNumConnectedClients() const` | Get the number of clients that are currently connected to the server.
`public int FindClientIndex(uint64_t clientId) const` | Find the client index for the client with the specified client id.
`public int FindClientIndex(const `[`Address`](#classyojimbo_1_1_address)` & address) const` | Find the client index for the client with the specified address.
`public uint64_t GetClientId(int clientIndex) const` | Get the client id for the client at the specified client index.
`public const `[`Address`](#classyojimbo_1_1_address)` & GetClientAddress(int clientIndex) const` | Get the address of the client at the specified client index.
`public const `[`Address`](#classyojimbo_1_1_address)` & GetServerAddress() const` | Get the server address.
`public void SetFlags(uint64_t flags)` | Set server flags.
`public uint64_t GetFlags() const` | Get the current server flags.
`public uint64_t GetCounter(int index) const` | Get a counter value.
`public void ResetCounters()` | Reset all counters to zero.
`public double GetTime() const` | Gets the current server time.
`public `[`Message`](#classyojimbo_1_1_message)` * CreateMsg(int clientIndex,int type)` | Create a message of the specified type.
`public bool CanSendMsg(int clientIndex,int channelId) const` | Check if there is room in the channel send queue to send one message to a client.
`public void SendMsg(int clientIndex,`[`Message`](#classyojimbo_1_1_message)` * message,int channelId)` | [Queue](#classyojimbo_1_1_queue) a message to be sent to a server.
`public `[`Message`](#classyojimbo_1_1_message)` * ReceiveMsg(int clientIndex,int channelId)` | Poll this method to receive messages sent from a client.
`public void ReleaseMsg(int clientIndex,`[`Message`](#classyojimbo_1_1_message)` * message)` | Release a message returned by [Server::ReceiveMsg](#classyojimbo_1_1_server_1a9740d45f24bb3879d6520e0f1248a97d).
`public `[`MessageFactory`](#classyojimbo_1_1_message_factory)` & GetMsgFactory(int clientIndex)` | Get the message factory instance belonging to a particular client.
`public inline `[`Allocator`](#classyojimbo_1_1_allocator)` & GetGlobalAllocator()` | Get the allocator used for global allocations.
`public inline `[`Allocator`](#classyojimbo_1_1_allocator)` & GetClientAllocator(int clientIndex)` | Get the allocator used for per-client.
`protected `[`Packet`](#classyojimbo_1_1_packet)` * CreateGlobalPacket(int type)` | Helper function to create a global packet by type.
`protected `[`Packet`](#classyojimbo_1_1_packet)` * CreateClientPacket(int clientIndex,int type)` | Helper function to create a client packet by type.
`protected virtual void OnStart(int maxClients)` | Override this method to get a callback when the server is started.
`protected virtual void OnStop()` | Override this method to get a callback when the server is stopped.
`protected virtual void OnConnectionRequest(`[`ServerConnectionRequestAction`](#namespaceyojimbo_1acf2e21e898444aa5236df8ab7c426992)` action,const `[`ConnectionRequestPacket`](#structyojimbo_1_1_connection_request_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address,const `[`ConnectToken`](#structyojimbo_1_1_connect_token)` & connectToken)` | Override this method to get a callback when the server processes a connection request packet.
`protected virtual void OnChallengeResponse(`[`ServerChallengeResponseAction`](#namespaceyojimbo_1a299c0cf5faa772f82b309af0c17b75f4)` action,const `[`ChallengeResponsePacket`](#structyojimbo_1_1_challenge_response_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address,const `[`ChallengeToken`](#structyojimbo_1_1_challenge_token)` & challengeToken)` | Override this method to get a callback when the server processes a challenge response packet.
`protected virtual void OnClientConnect(int clientIndex)` | Override this method to get a callback when a client connects to the server.
`protected virtual void OnClientDisconnect(int clientIndex)` | Override this method to get a callback when a client disconnects from the server.
`protected virtual void OnClientError(int clientIndex,`[`ServerClientError`](#namespaceyojimbo_1a74c07dfb41b33b1f637dc736417475f0)` error)` | Override this method to get a callback when an error occurs that will result in a client being disconnected from the server.
`protected virtual void OnPacketSent(int packetType,const `[`Address`](#classyojimbo_1_1_address)` & to,bool immediate)` | Override this method to get a callback when a packet is sent.
`protected virtual void OnPacketReceived(int packetType,const `[`Address`](#classyojimbo_1_1_address)` & from)` | Override this method to get a callback when a packet is received.
`protected virtual void OnConnectionPacketGenerated(`[`Connection`](#classyojimbo_1_1_connection)` * connection,uint16_t sequence)` | Override this method to get a callback when a connection packet is sent to a client.
`protected virtual void OnConnectionPacketAcked(`[`Connection`](#classyojimbo_1_1_connection)` * connection,uint16_t sequence)` | Override this method to get a callback when a connection packet is acked by the client (eg.
`protected virtual void OnConnectionPacketReceived(`[`Connection`](#classyojimbo_1_1_connection)` * connection,uint16_t sequence)` | Override this method to get a callback when a connection packet is received from a client.
`protected virtual void OnConnectionFragmentReceived(`[`Connection`](#classyojimbo_1_1_connection)` * connection,int channelId,uint16_t messageId,uint16_t fragmentId,int fragmentBytes,int numFragmentsReceived,int numFragmentsInBlock)` | Override this method to get a callback when a block fragment is received from a client.
`protected virtual bool ProcessUserPacket(int clientIndex,`[`Packet`](#classyojimbo_1_1_packet)` * packet)` | Override this method to process user packets sent from a client.
`protected virtual void SetEncryptedPacketTypes()` | 
`protected virtual void CreateAllocators()` | 
`protected virtual void DestroyAllocators()` | 
`protected virtual `[`Allocator`](#classyojimbo_1_1_allocator)` * CreateAllocator(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,void * memory,size_t bytes)` | 
`protected `[`Allocator`](#classyojimbo_1_1_allocator)` & GetAllocator(`[`ServerResourceType`](#namespaceyojimbo_1a953dcd2070d52623cc8b0fb706a562e8)` type,int clientIndex)` | 
`protected virtual `[`PacketFactory`](#classyojimbo_1_1_packet_factory)` * CreatePacketFactory(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,`[`ServerResourceType`](#namespaceyojimbo_1a953dcd2070d52623cc8b0fb706a562e8)` type,int clientIndex)` | 
`protected virtual `[`MessageFactory`](#classyojimbo_1_1_message_factory)` * CreateMessageFactory(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,`[`ServerResourceType`](#namespaceyojimbo_1a953dcd2070d52623cc8b0fb706a562e8)` type,int clientIndex)` | 
`protected virtual void ResetClientState(int clientIndex)` | 
`protected int FindFreeClientIndex() const` | 
`protected bool FindConnectTokenEntry(const uint8_t * mac)` | 
`protected bool FindOrAddConnectTokenEntry(const `[`Address`](#classyojimbo_1_1_address)` & address,const uint8_t * mac)` | 
`protected void ConnectClient(int clientIndex,const `[`Address`](#classyojimbo_1_1_address)` & clientAddress,uint64_t clientId)` | 
`protected void SendPacket(const `[`Address`](#classyojimbo_1_1_address)` & address,`[`Packet`](#classyojimbo_1_1_packet)` * packet,bool immediate)` | 
`protected void SendPacketToConnectedClient(int clientIndex,`[`Packet`](#classyojimbo_1_1_packet)` * packet,bool immediate)` | 
`protected void ProcessConnectionRequest(const `[`ConnectionRequestPacket`](#structyojimbo_1_1_connection_request_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address)` | 
`protected void ProcessChallengeResponse(const `[`ChallengeResponsePacket`](#structyojimbo_1_1_challenge_response_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address)` | 
`protected void ProcessKeepAlive(const `[`KeepAlivePacket`](#structyojimbo_1_1_keep_alive_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address)` | 
`protected void ProcessDisconnect(const `[`DisconnectPacket`](#structyojimbo_1_1_disconnect_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address)` | 
`protected void ProcessInsecureConnect(const `[`InsecureConnectPacket`](#structyojimbo_1_1_insecure_connect_packet)` &,const `[`Address`](#classyojimbo_1_1_address)` & address)` | 
`protected void ProcessConnectionPacket(`[`ConnectionPacket`](#structyojimbo_1_1_connection_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address)` | 
`protected void ProcessPacket(`[`Packet`](#classyojimbo_1_1_packet)` * packet,const `[`Address`](#classyojimbo_1_1_address)` & address,uint64_t sequence)` | 
`protected `[`KeepAlivePacket`](#structyojimbo_1_1_keep_alive_packet)` * CreateKeepAlivePacket(int clientIndex)` | 
`protected inline `[`Transport`](#classyojimbo_1_1_transport)` * GetTransport()` | 

## Members

#### `public  Server(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,`[`Transport`](#classyojimbo_1_1_transport)` & transport,const `[`ClientServerConfig`](#structyojimbo_1_1_client_server_config)` & config,double time)` 

The server constructor.

#### Parameters
* `allocator` The allocator for all memory used by the server. 


* `transport` The transport for sending and receiving packets. 


* `config` The client/server configuration. 


* `time` The current time in seconds. See [Server::AdvanceTime](#classyojimbo_1_1_server_1ae6e1b67b021d54ce967eae0b37a90238)

#### `public virtual  ~Server()` 

The server destructor.

IMPORTANT: Please call [Server::Stop](#classyojimbo_1_1_server_1a4f42d2c99693559c322cf2eb1eefbba9) before destroying the server. This is necessary because Stop is virtual and calling virtual methods from destructors does not give the expected behavior when you override that method.

#### `public void SetPrivateKey(const uint8_t * privateKey)` 

Set the private key used to decrypt connect tokens.

The private key must be known only to the dedicated server instance and the matchmaker backend that generates connect tokens.


#### Parameters
* `privateKey` The private key of size [yojimbo::KeyBytes](#namespaceyojimbo_1adaa08b5daf0fe9c4ff2c0c40867c21ba).

#### `public void SetUserContext(void * context)` 

Set the user context.

The user context is set on the stream when packets and read and written. It lets you pass in a pointer to some structure that you want to have available when reading and writing packets.

Typical use case is to pass in an array of min/max ranges for values determined by some data that is loaded from a toolchain vs. being known at compile time.

If you do use a user context, please make sure the data that contributes to the user context is checksummed and included in the protocol id, so clients and servers with incompatible data can't connect to each other.


**See also**: Stream::GetUserContext

#### `public void SetServerAddress(const `[`Address`](#classyojimbo_1_1_address)` & address)` 

Set the server IP address.

This should be a public IP address, eg. the address that a client would use to connect to the server.


#### Parameters
* `address` The server address.

#### `public void Start(int maxClients)` 

Start the server and allocate client slots.

Each client that connects to this server occupies one of the client slots allocated by this function.


#### Parameters
* `maxClients` The number of client slots to allocate. Must be in range [1,MaxClients]





**See also**: [Server::Stop](#classyojimbo_1_1_server_1a4f42d2c99693559c322cf2eb1eefbba9)

#### `public void Stop()` 

Stop the server and free client slots.

Any clients that are connected at the time you call stop will be disconnected.

When the server is stopped, clients cannot connect to the server.


**See also**: [Server::Start](#classyojimbo_1_1_server_1ac79505b75a954a3f5cbab63a4aaf3436).

#### `public void DisconnectClient(int clientIndex,bool sendDisconnectPacket)` 

Disconnect the client at the specified client index.

IMPORTANT: This function will assert if you attempt to disconnect a client that is not connected.


#### Parameters
* `clientIndex` The index of the client to disconnect in range [0,maxClients-1], where maxClients is the number of client slots allocated in [Server::Start](#classyojimbo_1_1_server_1ac79505b75a954a3f5cbab63a4aaf3436). 


* `sendDisconnectPacket` If true, disconnect packets are sent to the other side of the connection so it sees the disconnect as quickly as possible (rather than timing out).





**See also**: [Server::IsClientConnected](#classyojimbo_1_1_server_1a61a982c4ea83413605545da3b1ef9af0)

#### `public void DisconnectAllClients(bool sendDisconnectPacket)` 

Disconnect all clients from the server.

[Client](#classyojimbo_1_1_client) slots remain allocated as per the last call to [Server::Start](#classyojimbo_1_1_server_1ac79505b75a954a3f5cbab63a4aaf3436), they are simply made available for new clients to connect.


#### Parameters
* `sendDisconnectPacket` If true, disconnect packets are sent to the other side of the connection so it sees the disconnect as quickly as possible (rather than timing out).

#### `public void SendPackets()` 

Send packets to connected clients.

This function drives the sending of packets to clients such as keep-alives and connection packets to transmit messages from server to client.

Packets sent from this function are queued for sending on the transport, and are not actually serialized and sent to the network until you call [Transport::WritePackets](#classyojimbo_1_1_transport_1a903705df34df07acd44eccf3c6ccacff).

#### `public void ReceivePackets()` 

Receive packets sent from potential and connected clients.

This function receives and processes packets from the receive queue of the [Transport](#classyojimbo_1_1_transport). To minimize the latency of packet processing, make sure you call [Transport::ReadPackets](#classyojimbo_1_1_transport_1abbd058ddc3242bcfced01954c3d78dfa) shortly before calling this function.


**See also**: ProcessPackets

#### `public void CheckForTimeOut()` 

Check for timeouts.

Walks across the set of connected clients and compares the last time a packet was received from that client vs. the current time.

If no packet has been received within the timeout period, the client is disconnected and its client slot is made available for other clients to connect to.


**See also**: [Server::AdvanceTime](#classyojimbo_1_1_server_1ae6e1b67b021d54ce967eae0b37a90238)


**See also**: [ClientServerConfig::connectionTimeOut](#structyojimbo_1_1_client_server_config_1a71c455645f804ab75499e26ec728eb9c)


**See also**: [ClientServerConfig::connectionNegotiationTimeOut](#structyojimbo_1_1_client_server_config_1a30d025c12f2eac6c5b3ea9c069d86148)

#### `public void AdvanceTime(double time)` 

Advance server time.

Call this at the end of each frame to advance the server time forward.

IMPORTANT: Please use a double for your time value so it maintains sufficient accuracy as time increases.

#### `public bool IsRunning() const` 

Is the server running?

The server is running after you have called [Server::Start](#classyojimbo_1_1_server_1ac79505b75a954a3f5cbab63a4aaf3436). It is not running before the first server start, and after you call [Server::Stop](#classyojimbo_1_1_server_1a4f42d2c99693559c322cf2eb1eefbba9).

Clients can only connect to the server while it is running.


#### Returns
true if the server is currently running.

#### `public int GetMaxClients() const` 

Get the maximum number of clients that can connect to the server.

Corresponds to the maxClients parameter passed into the last call to [Server::Start](#classyojimbo_1_1_server_1ac79505b75a954a3f5cbab63a4aaf3436).


#### Returns
The maximum number of clients that can connect to the server. In other words, the number of client slots.

#### `public bool IsClientConnected(int clientIndex) const` 

Is a client connected to a client slot?

#### Parameters
* `clientIndex` the index of the client slot in [0,maxClients-1], where maxClients corresponds to the value passed into the last call to [Server::Start](#classyojimbo_1_1_server_1ac79505b75a954a3f5cbab63a4aaf3436).





#### Returns
True if the client is connected.

#### `public int GetNumConnectedClients() const` 

Get the number of clients that are currently connected to the server.

#### Returns
the number of connected clients.

#### `public int FindClientIndex(uint64_t clientId) const` 

Find the client index for the client with the specified client id.

#### Returns
The client index if a client with the client id is connected to the server, otherwise -1.

#### `public int FindClientIndex(const `[`Address`](#classyojimbo_1_1_address)` & address) const` 

Find the client index for the client with the specified address.

#### Returns
The client index if a client with the address is connected to the server, otherwise -1.

#### `public uint64_t GetClientId(int clientIndex) const` 

Get the client id for the client at the specified client index.

#### Returns
The client id of the client.

#### `public const `[`Address`](#classyojimbo_1_1_address)` & GetClientAddress(int clientIndex) const` 

Get the address of the client at the specified client index.

#### Returns
The address of the client.

#### `public const `[`Address`](#classyojimbo_1_1_address)` & GetServerAddress() const` 

Get the server address.

This is the public address that clients connect to.


#### Returns
The server address.


**See also**: [Server::SetServerAddress](#classyojimbo_1_1_server_1a7a99029745df7618f2ec45bc7a827414)

#### `public void SetFlags(uint64_t flags)` 

Set server flags.

Flags are used to enable and disable server functionality.


#### Parameters
* `flags` The server flags to set. See [yojimbo::ServerFlags](#namespaceyojimbo_1a57603dd733ee4081d0abd5245bbced79) for the set of server flags that can be passed in.





**See also**: [Server::GetFlags](#classyojimbo_1_1_server_1ad9c6ad33a36e2f145b8966ba5e0f46a4)

#### `public uint64_t GetFlags() const` 

Get the current server flags.

#### Returns
The server flags. See [yojimbo::ServerFlags](#namespaceyojimbo_1a57603dd733ee4081d0abd5245bbced79) for the set of server flags.


**See also**: [Server::SetFlags](#classyojimbo_1_1_server_1a16efe4045c3e291ebcc98769f9b26868)

#### `public uint64_t GetCounter(int index) const` 

Get a counter value.

Counters are used to track event and actions performed by the server. They are useful for debugging, testing and telemetry.


#### Returns
The counter value. See [yojimbo::ServerCounters](#namespaceyojimbo_1ab1523dc247c01d2f09d82c488cdf863d) for the set of server counters.

#### `public void ResetCounters()` 

Reset all counters to zero.

This is typically used with a telemetry application after uploading the current set of counters to the telemetry backend.

This way you can continue to accumulate events, and upload them at some frequency, like every 5 minutes to the telemetry backend, without double counting events.

#### `public double GetTime() const` 

Gets the current server time.

**See also**: [Server::AdvanceTime](#classyojimbo_1_1_server_1ae6e1b67b021d54ce967eae0b37a90238)

#### `public `[`Message`](#classyojimbo_1_1_message)` * CreateMsg(int clientIndex,int type)` 

Create a message of the specified type.

The message created by this function is typically passed to [Server::SendMsg](#classyojimbo_1_1_server_1ab89ae5e6b2f867043b2491da1e0bb14b). In this case, the send message function takes ownership of the message pointer and will release it for you.

If you are using the message in some other way, you are responsible for manually releasing it via [Server::ReleaseMsg](#classyojimbo_1_1_server_1ac902dac95ae08685348f3e2bcfc1aa1c).


#### Parameters
* `clientIndex` The index of the client the message will be sent to. This is necessary because each client has their own message factory and allocator. 


* `type` The message type. The set of message types depends on the message factory set on the client.





#### Returns
A pointer to the message created, or NULL if no message could be created.


**See also**: [MessageFactory](#classyojimbo_1_1_message_factory)

#### `public bool CanSendMsg(int clientIndex,int channelId) const` 

Check if there is room in the channel send queue to send one message to a client.

This function is useful in soak tests and unit tests where I want to send messages as quickly as possible, but don't want to overflow the send queue.

You don't need to call this function manually each time you call [Server::SendMsg](#classyojimbo_1_1_server_1ab89ae5e6b2f867043b2491da1e0bb14b). It's already asserted on in debug build and in release if it returns false it sets a runtime error that disconnects the client from the server.


#### Parameters
* `clientIndex` The index of the client the message will be sent to. 


* `channelId` The id of the channel in [0,numChannels-1].





#### Returns
True if the channel has room for one more message to be added to its send queue. False otherwise.

#### `public void SendMsg(int clientIndex,`[`Message`](#classyojimbo_1_1_message)` * message,int channelId)` 

[Queue](#classyojimbo_1_1_queue) a message to be sent to a server.

Adds a message to the send queue of the specified channel of the client connection.

The reliability and ordering guarantees of how the message will be received on the other side are determined by the configuration of the channel.

IMPORTANT: This function takes ownership of the message and ensures that the message is released when it finished being sent. This lets you create a message with [Server::CreateMsg](#classyojimbo_1_1_server_1a75ce42063083af8e534f4794e9c7060e) and pass it directly into this function. You don't need to manually release the message.


#### Parameters
* `clientIndex` The index of the client the message will be sent to. 


* `message` The message to be sent. It must be allocated from the message factory set on this client. 


* `channelId` The id of the channel to send the message across in [0,numChannels-1].





**See also**: [ChannelConfig](#structyojimbo_1_1_channel_config)


**See also**: [ClientServerConfig](#structyojimbo_1_1_client_server_config)

#### `public `[`Message`](#classyojimbo_1_1_message)` * ReceiveMsg(int clientIndex,int channelId)` 

Poll this method to receive messages sent from a client.

Typical usage is to iterate across the set of clients, iterate across the set of the channels, and poll this to receive messages until it returns NULL.

IMPORTANT: The message returned by this function has one reference. You are responsible for releasing this message via [Server::ReleaseMsg](#classyojimbo_1_1_server_1ac902dac95ae08685348f3e2bcfc1aa1c).


#### Parameters
* `clientIndex` The index of the client that we want to receive messages from. 


* `channelId` The id of the channel to try to receive a message from.





#### Returns
A pointer to the received message, NULL if there are no messages to receive.

#### `public void ReleaseMsg(int clientIndex,`[`Message`](#classyojimbo_1_1_message)` * message)` 

Release a message returned by [Server::ReceiveMsg](#classyojimbo_1_1_server_1a9740d45f24bb3879d6520e0f1248a97d).

This is a convenience function. It is equivalent to calling [MessageFactory::Release](#classyojimbo_1_1_message_factory_1a20529940091cb084c7876a4396954f50) on the message factory set on this client (see [Server::GetMsgFactory](#classyojimbo_1_1_server_1aefe868e935ac6bf80069c410d76ccb52)).


#### Parameters
* `clientIndex` The index of the client. This is necessary because each client has their own message factory, and messages must be released with the same message factory they were created with. 


* `message` The message to release. Must be non-NULL.





**See also**: [Server::ReceiveMsg](#classyojimbo_1_1_server_1a9740d45f24bb3879d6520e0f1248a97d)

#### `public `[`MessageFactory`](#classyojimbo_1_1_message_factory)` & GetMsgFactory(int clientIndex)` 

Get the message factory instance belonging to a particular client.

The message factory determines the set of messages exchanged between the client and server.


#### Parameters
* `clientIndex` The index of the client. Each client has their own message factory.





#### Returns
The message factory for the client.


**See also**: [YOJIMBO_SERVER_MESSAGE_FACTORY](#yojimbo__server_8h_1a344dbec77afed208ecec1788eef96c5d)

#### `public inline `[`Allocator`](#classyojimbo_1_1_allocator)` & GetGlobalAllocator()` 

Get the allocator used for global allocations.

Global allocations are allocations that aren't tied to any particular client.

Typically, this means data structures that correspond to connection negotiation, processing connection requests and so on.

The amount of memory backing this allocator is specified by [ClientServerConfig::serverGlobalMemory](#structyojimbo_1_1_client_server_config_1aaf57b9bcbc5ea1733451f087fcd6f144).


#### Returns
The global allocator.

#### `public inline `[`Allocator`](#classyojimbo_1_1_allocator)` & GetClientAllocator(int clientIndex)` 

Get the allocator used for per-client.

Per-client allocations are allocations that are tied to a particular client index. The idea is to silo each client on the server to their own set of resources, making it impossible for a client to exhaust resources shared with other clients connected to the server.

The amount of memory backing this allocator is specified by ClientServerConfig::perClientMemory. There is one allocator per-client slot allocated in [Server::Start](#classyojimbo_1_1_server_1ac79505b75a954a3f5cbab63a4aaf3436). These allocators are undefined outside of Start/Stop and will assert in that case.


#### Parameters
* `clientIndex` The index of the client.





#### Returns
The per-client allocator corresponding to the client index.

#### `protected `[`Packet`](#classyojimbo_1_1_packet)` * CreateGlobalPacket(int type)` 

Helper function to create a global packet by type.

Global packets don't belong to any particular client. These are packets send and received as part of the connection negotiation protocol.


#### Parameters
* `type` The type of packet to create.





#### Returns
The packet object that was created. NULL if a packet could be created. You *must* check this. It *will* happen when the packet factory runs out of memory to allocate packets!

#### `protected `[`Packet`](#classyojimbo_1_1_packet)` * CreateClientPacket(int clientIndex,int type)` 

Helper function to create a client packet by type.

[Client](#classyojimbo_1_1_client) packets belong to particular client index. These are packets sent after a client/server connection has been established.


#### Parameters
* `clientIndex` Index of the client that the packet will be sent to. 


* `type` The type of packet to create.





#### Returns
The packet object that was created. NULL if a packet could be created. You *must* check this. It *will* happen when the packet factory runs out of memory to allocate packets!

#### `protected virtual void OnStart(int maxClients)` 

Override this method to get a callback when the server is started.

#### Parameters
* `maxClients` The number of client slots are being allocated. eg. maximum number of clients that can connect to the server.

#### `protected virtual void OnStop()` 

Override this method to get a callback when the server is stopped.



#### `protected virtual void OnConnectionRequest(`[`ServerConnectionRequestAction`](#namespaceyojimbo_1acf2e21e898444aa5236df8ab7c426992)` action,const `[`ConnectionRequestPacket`](#structyojimbo_1_1_connection_request_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address,const `[`ConnectToken`](#structyojimbo_1_1_connect_token)` & connectToken)` 

Override this method to get a callback when the server processes a connection request packet.

#### Parameters
* `action` The action that the server took in response to the connection request packet. 


* `packet` The connection request packet being processed. 


* `address` The address the packet was sent from. 


* `connectToken` The decrypted connect token. Depending on the action, this may or may not be valid. See [yojimbo::ServerConnectionRequestAction](#namespaceyojimbo_1acf2e21e898444aa5236df8ab7c426992) for details.

#### `protected virtual void OnChallengeResponse(`[`ServerChallengeResponseAction`](#namespaceyojimbo_1a299c0cf5faa772f82b309af0c17b75f4)` action,const `[`ChallengeResponsePacket`](#structyojimbo_1_1_challenge_response_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address,const `[`ChallengeToken`](#structyojimbo_1_1_challenge_token)` & challengeToken)` 

Override this method to get a callback when the server processes a challenge response packet.

#### Parameters
* `action` The action that the server took in response to the challenge response packet. 


* `packet` The challenge response packet being processed. 


* `address` The address the packet was sent from. 


* `challengeToken` The decrypted challenge token. Depending on the action, this may or may not be valid. See [yojimbo::ServerChallengeResponseAction](#namespaceyojimbo_1a299c0cf5faa772f82b309af0c17b75f4) for details.

#### `protected virtual void OnClientConnect(int clientIndex)` 

Override this method to get a callback when a client connects to the server.

IMPORTANT: The client is fully connected at the point this callback is made, so all data structures are setup for this client, and query functions based around client index will work properly.


#### Parameters
* `clientIndex` The index of the slot that the client connected to.

#### `protected virtual void OnClientDisconnect(int clientIndex)` 

Override this method to get a callback when a client disconnects from the server.

IMPORTANT: The client is (still) fully connected at the point this callback is made, so all data structures are setup for the client, and query functions based around client index will work properly.


#### Parameters
* `clientIndex` The client slot index of the disconnecting client.

#### `protected virtual void OnClientError(int clientIndex,`[`ServerClientError`](#namespaceyojimbo_1a74c07dfb41b33b1f637dc736417475f0)` error)` 

Override this method to get a callback when an error occurs that will result in a client being disconnected from the server.

IMPORTANT: The client is (still) fully connected at the point this callback is made, so all data structures are setup for the client, and query functions based around client index will work properly.


#### Parameters
* `clientIndex` The client slot index of the client that is about to error out. 


* `error` The error that occured.

#### `protected virtual void OnPacketSent(int packetType,const `[`Address`](#classyojimbo_1_1_address)` & to,bool immediate)` 

Override this method to get a callback when a packet is sent.

#### Parameters
* `packetType` The type of packet being sent according to the packet factory that is set on the server. See [PacketFactory](#classyojimbo_1_1_packet_factory). 


* `to` The address the packet is being sent to. 


* `immediate` If true then this packet will be serialized and flushed to the network immediately. This is used for disconnect packets. See [Server::DisconnectClient](#classyojimbo_1_1_server_1a52e3f854f2236c4812f08cd123c3ef8c).

#### `protected virtual void OnPacketReceived(int packetType,const `[`Address`](#classyojimbo_1_1_address)` & from)` 

Override this method to get a callback when a packet is received.

#### Parameters
* `packetType` The type of packet being sent according to the packet factory that is set on the server. See [PacketFactory](#classyojimbo_1_1_packet_factory). 


* `from` The address the packet was received from.

#### `protected virtual void OnConnectionPacketGenerated(`[`Connection`](#classyojimbo_1_1_connection)` * connection,uint16_t sequence)` 

Override this method to get a callback when a connection packet is sent to a client.

[Connection](#classyojimbo_1_1_connection) packets are carrier packets that transmit messages between the client and server. They are only generated if you enabled messages in the [ClientServerConfig](#structyojimbo_1_1_client_server_config) (true by default).


#### Parameters
* `connection` The connection that the packet belongs to. To get the client index call [Connection::GetClientIndex](#classyojimbo_1_1_connection_1a97503e22bdb599176a64f6e70655e548). 


* `sequence` The sequence number of the connection packet being sent.





**See also**: [ClientServerConfig::enableMessages](#structyojimbo_1_1_client_server_config_1a6e1a7fd033a718b0a0d08e4b494585bf)


**See also**: [Connection::GetClientIndex](#classyojimbo_1_1_connection_1a97503e22bdb599176a64f6e70655e548)

#### `protected virtual void OnConnectionPacketAcked(`[`Connection`](#classyojimbo_1_1_connection)` * connection,uint16_t sequence)` 

Override this method to get a callback when a connection packet is acked by the client (eg.

the client notified the server that packet was received).

[Connection](#classyojimbo_1_1_connection) packets are carrier packets that transmit messages between the client and server. They are automatically generated when messages are enabled in [ClientServerConfig](#structyojimbo_1_1_client_server_config) (true by default).


#### Parameters
* `connection` The connection that the packet belongs to. To get the client index call [Connection::GetClientIndex](#classyojimbo_1_1_connection_1a97503e22bdb599176a64f6e70655e548). 


* `sequence` The packet sequence number of the connection packet that was acked by the client.





**See also**: [ClientServerConfig::enableMessages](#structyojimbo_1_1_client_server_config_1a6e1a7fd033a718b0a0d08e4b494585bf)


**See also**: [Connection::GetClientIndex](#classyojimbo_1_1_connection_1a97503e22bdb599176a64f6e70655e548)

#### `protected virtual void OnConnectionPacketReceived(`[`Connection`](#classyojimbo_1_1_connection)` * connection,uint16_t sequence)` 

Override this method to get a callback when a connection packet is received from a client.

[Connection](#classyojimbo_1_1_connection) packets are carrier packets that transmit messages between the client and server. They are automatically generated when messages are enabled in [ClientServerConfig](#structyojimbo_1_1_client_server_config) (true by default).


#### Parameters
* `connection` The connection that the packet belongs to. To get the client index call [Connection::GetClientIndex](#classyojimbo_1_1_connection_1a97503e22bdb599176a64f6e70655e548). 


* `sequence` The sequence number of the connection packet that was received.





**See also**: [ClientServerConfig::enableMessages](#structyojimbo_1_1_client_server_config_1a6e1a7fd033a718b0a0d08e4b494585bf)


**See also**: [Connection::GetClientIndex](#classyojimbo_1_1_connection_1a97503e22bdb599176a64f6e70655e548)

#### `protected virtual void OnConnectionFragmentReceived(`[`Connection`](#classyojimbo_1_1_connection)` * connection,int channelId,uint16_t messageId,uint16_t fragmentId,int fragmentBytes,int numFragmentsReceived,int numFragmentsInBlock)` 

Override this method to get a callback when a block fragment is received from a client.

This callback lets you implement a progress bar for large block transmissions.


#### Parameters
* `connection` The connection that the block fragment belongs to. To get the client index call [Connection::GetClientIndex](#classyojimbo_1_1_connection_1a97503e22bdb599176a64f6e70655e548). 


* `channelId` The channel the block is being sent over. 


* `messageId` The message id the block is attached to. 


* `fragmentId` The fragment id that is being processed. Fragment ids are in the range [0,numFragments-1]. 


* `fragmentBytes` The size of the fragment in bytes. 


* `numFragmentsReceived` The number of fragments received for this block so far (including this one). 


* `numFragmentsInBlock` The total number of fragments in this block. The block receive completes when all fragments are received.





**See also**: [Connection::GetClientIndex](#classyojimbo_1_1_connection_1a97503e22bdb599176a64f6e70655e548)


**See also**: [BlockMessage::AttachBlock](#classyojimbo_1_1_block_message_1af88bdd0b14d6ff6d73144dddb6581563)

#### `protected virtual bool ProcessUserPacket(int clientIndex,`[`Packet`](#classyojimbo_1_1_packet)` * packet)` 

Override this method to process user packets sent from a client.

User packets let you extend the yojimbo by adding your own packet types to be exchanged between client and server. See [PacketFactory](#classyojimbo_1_1_packet_factory).

Most users won't need to create custom packet types, and will extend the protocol by defining their own message types instead. See [MessageFactory](#classyojimbo_1_1_message_factory).


#### Parameters
* `clientIndex` Identifies which client this packet came from. 


* `packet` The user packet received from the client.





#### Returns
Return true if the user packet was processed successfully. Returning false if the packet could not be processed, or if is of a type you don't expect. This ensures that unexpected packet types don't keep the connection alive when it should time out.

#### `protected virtual void SetEncryptedPacketTypes()` 





#### `protected virtual void CreateAllocators()` 





#### `protected virtual void DestroyAllocators()` 





#### `protected virtual `[`Allocator`](#classyojimbo_1_1_allocator)` * CreateAllocator(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,void * memory,size_t bytes)` 





#### `protected `[`Allocator`](#classyojimbo_1_1_allocator)` & GetAllocator(`[`ServerResourceType`](#namespaceyojimbo_1a953dcd2070d52623cc8b0fb706a562e8)` type,int clientIndex)` 





#### `protected virtual `[`PacketFactory`](#classyojimbo_1_1_packet_factory)` * CreatePacketFactory(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,`[`ServerResourceType`](#namespaceyojimbo_1a953dcd2070d52623cc8b0fb706a562e8)` type,int clientIndex)` 





#### `protected virtual `[`MessageFactory`](#classyojimbo_1_1_message_factory)` * CreateMessageFactory(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,`[`ServerResourceType`](#namespaceyojimbo_1a953dcd2070d52623cc8b0fb706a562e8)` type,int clientIndex)` 





#### `protected virtual void ResetClientState(int clientIndex)` 





#### `protected int FindFreeClientIndex() const` 





#### `protected bool FindConnectTokenEntry(const uint8_t * mac)` 





#### `protected bool FindOrAddConnectTokenEntry(const `[`Address`](#classyojimbo_1_1_address)` & address,const uint8_t * mac)` 





#### `protected void ConnectClient(int clientIndex,const `[`Address`](#classyojimbo_1_1_address)` & clientAddress,uint64_t clientId)` 





#### `protected void SendPacket(const `[`Address`](#classyojimbo_1_1_address)` & address,`[`Packet`](#classyojimbo_1_1_packet)` * packet,bool immediate)` 





#### `protected void SendPacketToConnectedClient(int clientIndex,`[`Packet`](#classyojimbo_1_1_packet)` * packet,bool immediate)` 





#### `protected void ProcessConnectionRequest(const `[`ConnectionRequestPacket`](#structyojimbo_1_1_connection_request_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address)` 





#### `protected void ProcessChallengeResponse(const `[`ChallengeResponsePacket`](#structyojimbo_1_1_challenge_response_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address)` 





#### `protected void ProcessKeepAlive(const `[`KeepAlivePacket`](#structyojimbo_1_1_keep_alive_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address)` 





#### `protected void ProcessDisconnect(const `[`DisconnectPacket`](#structyojimbo_1_1_disconnect_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address)` 





#### `protected void ProcessInsecureConnect(const `[`InsecureConnectPacket`](#structyojimbo_1_1_insecure_connect_packet)` &,const `[`Address`](#classyojimbo_1_1_address)` & address)` 





#### `protected void ProcessConnectionPacket(`[`ConnectionPacket`](#structyojimbo_1_1_connection_packet)` & packet,const `[`Address`](#classyojimbo_1_1_address)` & address)` 





#### `protected void ProcessPacket(`[`Packet`](#classyojimbo_1_1_packet)` * packet,const `[`Address`](#classyojimbo_1_1_address)` & address,uint64_t sequence)` 





#### `protected `[`KeepAlivePacket`](#structyojimbo_1_1_keep_alive_packet)` * CreateKeepAlivePacket(int clientIndex)` 





#### `protected inline `[`Transport`](#classyojimbo_1_1_transport)` * GetTransport()` 





# class `yojimbo::Socket` 


Simple wrapper around a non-blocking UDP socket.

IMPORTANT: You must initialize the network layer before creating any sockets.


**See also**: [InitializeNetwork](#namespaceyojimbo_1aa3c09f4d564392b7e78774d5bd90d3b3)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  explicit Socket(const `[`Address`](#classyojimbo_1_1_address)` & address,int sendBufferSize,int receiveBufferSize)` | Creates a socket.
`public  ~Socket()` | [Socket](#classyojimbo_1_1_socket) destructor.
`public bool IsError() const` | Is the socket in an error state?
`public `[`SocketError`](#namespaceyojimbo_1a7d06104bf0ad6d55ace64fe1f1d03349)` GetError() const` | Get the socket error state.
`public void SendPacket(const `[`Address`](#classyojimbo_1_1_address)` & to,const void * packetData,size_t packetBytes)` | Send a packet to an address using this socket.
`public int ReceivePacket(`[`Address`](#classyojimbo_1_1_address)` & from,void * packetData,int maxPacketSize)` | Receive a packet from the network (non-blocking).
`public const `[`Address`](#classyojimbo_1_1_address)` & GetAddress() const` | Get the socket address including the dynamically assigned port # for sockets bound to port 0.

## Members

#### `public  explicit Socket(const `[`Address`](#classyojimbo_1_1_address)` & address,int sendBufferSize,int receiveBufferSize)` 

Creates a socket.

Please check [Socket::IsError](#classyojimbo_1_1_socket_1a0b0c3248f4e7d33d4cf29bb28ce66cc2) after creating a socket, as it could be in an error state.

IMPORTANT: You must initialize the network layer before creating sockets. See [yojimbo::InitializeNetwork](#namespaceyojimbo_1aa3c09f4d564392b7e78774d5bd90d3b3).


#### Parameters
* `address` The address to bind the socket to. 


* `sendBufferSize` The size of the send buffer to set on the socket (SO_SNDBUF). 


* `receiveBufferSize` The size of the receive buffer to set on the socket (SO_RCVBUF).

#### `public  ~Socket()` 

[Socket](#classyojimbo_1_1_socket) destructor.



#### `public bool IsError() const` 

Is the socket in an error state?

Corresponds to the socket error state being something other than [yojimbo::SOCKET_ERROR_NONE](#namespaceyojimbo_1a7d06104bf0ad6d55ace64fe1f1d03349a6b43243bee142fdf1120100116c63842).


#### Returns
True if the socket is in an error state, false otherwise.

#### `public `[`SocketError`](#namespaceyojimbo_1a7d06104bf0ad6d55ace64fe1f1d03349)` GetError() const` 

Get the socket error state.

These error states correspond to things that can go wrong in the socket constructor when it's created.

IMPORTANT: Please make sure you always check for error state after you create a socket.


#### Returns
The socket error state.


**See also**: [Socket::IsError](#classyojimbo_1_1_socket_1a0b0c3248f4e7d33d4cf29bb28ce66cc2)

#### `public void SendPacket(const `[`Address`](#classyojimbo_1_1_address)` & to,const void * packetData,size_t packetBytes)` 

Send a packet to an address using this socket.

The packet is sent unreliably over UDP. It may arrive out of order, in duplicate or not at all.


#### Parameters
* `to` The address to send the packet to. 


* `packetData` The packet data to send. 


* `packetBytes` The size of the packet data to send (bytes).

#### `public int ReceivePacket(`[`Address`](#classyojimbo_1_1_address)` & from,void * packetData,int maxPacketSize)` 

Receive a packet from the network (non-blocking).

#### Parameters
* `from` The address that sent the packet [out] 


* `packetData` The buffer where the packet data will be copied to. Must be at least maxPacketSize large in bytes. 


* `maxPacketSize` The maximum packet size to read in bytes. Any packets received larger than this are discarded.





#### Returns
The size of the packet received in [1,maxPacketSize], or 0 if no packet is available to read.

#### `public const `[`Address`](#classyojimbo_1_1_address)` & GetAddress() const` 

Get the socket address including the dynamically assigned port # for sockets bound to port 0.

#### Returns
The socket address. Port is guaranteed to be non-zero, provided the socket is not in an error state.

# class `yojimbo::TLSF_Allocator` 

```
class yojimbo::TLSF_Allocator
  : public yojimbo::Allocator
```  

[Allocator](#classyojimbo_1_1_allocator) built on the TLSF allocator implementation by Matt Conte.

Thanks Matt!

This is a fast allocator that supports multiple heaps. It's used inside the yojimbo server to silo allocations for each client to their own heap.

See [https://github.com/mattconte/tlsf](https://github.com/mattconte/tlsf) for details on this allocator implementation.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  TLSF_Allocator(void * memory,size_t bytes)` | TLSF allocator constructor.
`public  ~TLSF_Allocator()` | TLSF allocator destructor.
`public virtual void * Allocate(size_t size,const char * file,int line)` | Allocates a block of memory using TLSF.
`public virtual void Free(void * p,const char * file,int line)` | Free a block of memory using TLSF.

## Members

#### `public  TLSF_Allocator(void * memory,size_t bytes)` 

TLSF allocator constructor.

If you want to integrate your own allocator with yojimbo for use with the client and server, this class is a good template to start from.

Make sure your constructor has the same signature as this one, and it will work with the YOJIMBO_SERVER_ALLOCATOR and YOJIMBO_CLIENT_ALLOCATOR helper macros.


#### Parameters
* `memory` Block of memory in which the allocator will work. This block must remain valid while this allocator exists. The allocator does not assume ownership of it, you must free it elsewhere, if necessary. 


* `bytes` The size of the block of memory (bytes). Actual amount of memor you can allocate will be less than this, due to allocator overhead.

#### `public  ~TLSF_Allocator()` 

TLSF allocator destructor.

Checks for memory leaks in debug build. Free all memory allocated by this allocator before destroying.

#### `public virtual void * Allocate(size_t size,const char * file,int line)` 

Allocates a block of memory using TLSF.

IMPORTANT: Don't call this directly. Use the YOJIMBO_NEW or YOJIMBO_ALLOCATE macros instead, because they automatically pass in the source filename and line number for you.


#### Parameters
* `size` The size of the block of memory to allocate (bytes). 


* `file` The source code filename that is performing the allocation. Used for tracking allocations and reporting on memory leaks. 


* `line` The line number in the source code file that is performing the allocation.





#### Returns
A block of memory of the requested size, or NULL if the allocation could not be performed. If NULL is returned, the error level is set to ALLOCATION_ERROR_FAILED_TO_ALLOCATE.

#### `public virtual void Free(void * p,const char * file,int line)` 

Free a block of memory using TLSF.

IMPORTANT: Don't call this directly. Use the YOJIMBO_DELETE or YOJIMBO_FREE macros instead, because they automatically pass in the source filename and line number for you.


#### Parameters
* `p` Pointer to the block of memory to free. Must be non-NULL block of memory that was allocated with this allocator. Will assert otherwise. 


* `file` The source code filename that is performing the free. Used for tracking allocations and reporting on memory leaks. 


* `line` The line number in the source code file that is performing the free.





**See also**: [Allocator::Allocate](#classyojimbo_1_1_allocator_1ab33a56067c7b815481e3dd091b701f9c)


**See also**: [Allocator::GetError](#classyojimbo_1_1_allocator_1a6418ce2bf0cf777755b2d06cca6ff522)

# class `yojimbo::Transport` 


Interface for sending and receiving packets.

This is the common interface shared by all transport implementations.

Transports provide send and receive queues for high level packets objects, taking care of the details of reading and writing binary packets to the network.

It is intended to allow mixing and matching of different transport implementations with protocol classes like [Client](#classyojimbo_1_1_client), [Server](#classyojimbo_1_1_server) and [Connection](#classyojimbo_1_1_connection). This way a new transport implementation can be swapped in without any change to protocol level code.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline virtual  ~Transport()` | 
`public void Reset()` | Reset the transport.
`public void SetContext(const `[`TransportContext`](#structyojimbo_1_1_transport_context)` & context)` | Set the transport context.
`public void ClearContext()` | Clear context.
`public void SendPacket(const `[`Address`](#classyojimbo_1_1_address)` & address,`[`Packet`](#classyojimbo_1_1_packet)` * packet,uint64_t sequence,bool immediate)` | [Queue](#classyojimbo_1_1_queue) a packet to be sent.
`public `[`Packet`](#classyojimbo_1_1_packet)` * ReceivePacket(`[`Address`](#classyojimbo_1_1_address)` & from,uint64_t * sequence)` | Receive a packet.
`public void WritePackets()` | Iterates across all packets in the send queue and writes them to the network.
`public void ReadPackets()` | Reads packets from the network and adds them to the packet receive queue.
`public int GetMaxPacketSize() const` | Returns the maximum packet size supported by this transport.
`public void SetNetworkConditions(float latency,float jitter,float packetLoss,float duplicate)` | Add simulated network conditions on top of this transport.
`public void ClearNetworkConditions()` | Clear network conditions back to defaults.
`public void EnablePacketEncryption()` | Turns on packet encryption and enables it for all packet types.
`public void DisablePacketEncryption()` | Disables encryption for all packet types.
`public void DisableEncryptionForPacketType(int type)` | Disables encryption for a specific packet type.
`public bool IsEncryptedPacketType(int type) const` | Is a packet type encrypted?
`public bool AddEncryptionMapping(const `[`Address`](#classyojimbo_1_1_address)` & address,const uint8_t * sendKey,const uint8_t * receiveKey,double timeout)` | Associates an address with keys for packet encryption.
`public bool RemoveEncryptionMapping(const `[`Address`](#classyojimbo_1_1_address)` & address)` | Remove the encryption mapping for an address.
`public int FindEncryptionMapping(const `[`Address`](#classyojimbo_1_1_address)` & address)` | Find the index of an encryption mapping by address.
`public void ResetEncryptionMappings()` | Reset all encryption mappings.
`public bool AddContextMapping(const `[`Address`](#classyojimbo_1_1_address)` & address,const `[`TransportContext`](#structyojimbo_1_1_transport_context)` & context)` | Add a transport context that is specific for an address.
`public bool RemoveContextMapping(const `[`Address`](#classyojimbo_1_1_address)` & address)` | Remove a context mapping.
`public void ResetContextMappings()` | Reset all context mappings.
`public void AdvanceTime(double time)` | Advance transport time.
`public double GetTime() const` | Gets the current transport time.
`public uint64_t GetCounter(int index) const` | Get a counter value.
`public void ResetCounters()` | Reset all counters to zero.
`public void SetFlags(uint64_t flags)` | Set transport flags.
`public uint64_t GetFlags() const` | Get the current transport flags.
`public const `[`Address`](#classyojimbo_1_1_address)` & GetAddress() const` | Get the address of the transport.
`public uint64_t GetProtocolId() const` | Get the protocol id for the transport.

## Members

#### `public inline virtual  ~Transport()` 





#### `public void Reset()` 

Reset the transport.

This function completely resets the transport, clearing all packet send and receive queues and resetting all contexts and encryption mappings.

This gets the transport back into a pristine state, ready to be used for some other purpose, without needing to destroy and recreate the transport object.

#### `public void SetContext(const `[`TransportContext`](#structyojimbo_1_1_transport_context)` & context)` 

Set the transport context.

This is the default context to be used for reading and writing packets. See [TransportContext](#structyojimbo_1_1_transport_context).

The user can also call [Transport::AddContextMapping](#classyojimbo_1_1_transport_1a4d0e96d87d6a7b3890ca077614e7a4e8) to associate up to [yojimbo::MaxContextMappings](#namespaceyojimbo_1a3eb912b430ab10ab4afc8737d3ea883a) contexts with particular addresses. When a context is associated with an address, that context is when reading/writing packets from/to that address instead of this one.


#### Parameters
* `context` The default context for reading and writing packets.

#### `public void ClearContext()` 

Clear context.

This function clears the default context set on the transport.

This returns the transport to the state before a context was set on it. While in this state, the transport cannot read or write packets and will early out of [Transport::ReadPackets](#classyojimbo_1_1_transport_1abbd058ddc3242bcfced01954c3d78dfa) and [Transport::WritePackets](#classyojimbo_1_1_transport_1a903705df34df07acd44eccf3c6ccacff).

If the transport is set on a client, it is returned to this state after the client disconnects. On the server, the transport is returned to this state when the server stops.


**See also**: [Client::Disconnect](#classyojimbo_1_1_client_1a9eb150732daeadb0be17e1b16f3b7658)


**See also**: [Server::Stop](#classyojimbo_1_1_server_1a4f42d2c99693559c322cf2eb1eefbba9)

#### `public void SendPacket(const `[`Address`](#classyojimbo_1_1_address)` & address,`[`Packet`](#classyojimbo_1_1_packet)` * packet,uint64_t sequence,bool immediate)` 

[Queue](#classyojimbo_1_1_queue) a packet to be sent.

Ownership of the packet object is transferred to the transport. Don't call [Packet::Destroy](#classyojimbo_1_1_packet_1a597f853846f3d75213281625b25c9d16) on the packet after you send it with this function, the transport will do that for you automatically.

IMPORTANT: The packet will be sent over a UDP-equivalent network. It may arrive out of order, in duplicate or not at all.


#### Parameters
* `address` The address the packet should be sent to. 


* `packet` The packet that is being sent. 


* `sequence` The 64 bit sequence number of the packet used as a nonce for packet encryption. Should increase with each packet sent per-encryption mapping, but this is up to the caller. 


* `immediate` If true the the packet is written and flushed to the network immediately, rather than in the next call to [Transport::WritePackets](#classyojimbo_1_1_transport_1a903705df34df07acd44eccf3c6ccacff).

#### `public `[`Packet`](#classyojimbo_1_1_packet)` * ReceivePacket(`[`Address`](#classyojimbo_1_1_address)` & from,uint64_t * sequence)` 

Receive a packet.

This function pops a packet off the receive queue, if any are available to be received.

This function transfers ownership of the packet pointer to the caller. It is the callers responsibility to call [Packet::Destroy](#classyojimbo_1_1_packet_1a597f853846f3d75213281625b25c9d16) after processing the packet.

To make sure packet latency is minimized call Transport::ReceivePackets just before looping and calling this function until it returns NULL.


#### Parameters
* `from` The address that the packet was received from. 


* `sequence` Pointer to an unsigned 64bit sequence number (optional). If the pointer is not NULL, it will be dereferenced to store the sequence number of the received packet. Only encrypted packets have these sequence numbers. 





#### Returns
The next packet in the receive queue, or NULL if no packets are left in the receive queue.

#### `public void WritePackets()` 

Iterates across all packets in the send queue and writes them to the network.

To minimize packet latency, call this function shortly after you have finished queueing up packets to send via [Transport::SendPacket](#classyojimbo_1_1_transport_1aa9c2dec77b36ddd19296c35533173b5f).

#### `public void ReadPackets()` 

Reads packets from the network and adds them to the packet receive queue.

To minimize packet latency call this function right before you loop calling [Transport::ReceivePacket](#classyojimbo_1_1_transport_1affa39a75fc878fde8641dfd888a21137) until it returns NULL.

#### `public int GetMaxPacketSize() const` 

Returns the maximum packet size supported by this transport.

This is typically configured in the constructor of the transport implementation.

It's added here so callers using the transport interface know the maximum packet size that can be sent over the transport.


#### Returns
The maximum packet size that can be sent over this transport (bytes).

#### `public void SetNetworkConditions(float latency,float jitter,float packetLoss,float duplicate)` 

Add simulated network conditions on top of this transport.

By default no latency, jitter, packet loss, or duplicates are added on top of the network.

However, during development you often want to simulate some latency and packet loss while the game is being played over the LAN, so people developing your game do so under conditions that you can expect to encounter when it's being played over the Internet.

I recommend adding around 125ms of latency and somewhere between 2-5% packet loss, and +/- one frame of jitter @ 60HZ (eg. 20ms or so).

IMPORTANT: Take care when sitting simulated network conditions because they are implemented on packet send only, so you usually want to set HALF of the latency you want on client, and HALF of the latency on the server transport. So for 100ms of latency in total, you'd set 50ms latency on the client transport, and 50ms latency on the server transport, which adds up to of 100ms extra round trip delay.

The network simulator allocated here can take up a significant amount of memory. If you want to save memory, you might want to disable the network simulator. You can do this in the constructor of your transport implementation. See [NetworkTransport::NetworkTransport](#classyojimbo_1_1_network_transport_1add26cbdcc1e3584a892004f3d134cdf6) for details.


#### Parameters
* `latency` The amount of latency to add to each packet sent (milliseconds). 


* `jitter` The amount of jitter to add to each packet sent (milliseconds). The packet delivery time is adjusted by some random amount within +/- jitter. This is best used in combination with some amount of latency, otherwise the jitter is not truly +/-. 


* `packetLoss` The percentage of packets to drop on send. 100% drops all packets. 0% drops no packets. 


* `duplicate` The percentage of packets to be duplicated. Duplicate packets are scheduled to be sent at some random time up to 1 second in the future, to grossly approximate duplicate packets that occur from IP route changes.

#### `public void ClearNetworkConditions()` 

Clear network conditions back to defaults.

After this function latency, jitter, packet loss and duplicate packets are all set back to zero.

#### `public void EnablePacketEncryption()` 

Turns on packet encryption and enables it for all packet types.

When a packet is sent the transport checks if that packet type is encrypted. If it's an encrypted, the transport looks for an encryption mapping for the destination address, and encrypts the packet with that key. Otherwise, the packet is sent out unencrypted.

When a packet is received, the transport checks if that packetet type should be encrypted. If it should be, but the packet itself is not encrypted, the packet is discarded. Otherwise, the server looks for a decryption key for the packet sender address and decrypts the packet before adding it to the packet receive queue.

The exception to this rule is when you enable TRANSPORT_FLAG_INSECURE_MODE via [Transport::SetFlags](#classyojimbo_1_1_transport_1aa7cd42ebe2b149a71ed05438c76d3417), which makes encryption optional for encrypted packet types.

This allows a mixture of secure and insecure client connections, which is convenient for development over a LAN, but should NEVER be enabled in production.

Please make sure you #define YOJIMBO_SECURE_MODE 1 in your production build!


**See also**: [Transport::DisablePacketEncryption](#classyojimbo_1_1_transport_1ab319325cba1edddf68ebf2f4fa593acb)


**See also**: [Transport::DisableEncryptionForPacketType](#classyojimbo_1_1_transport_1aa102f26f89f0b48bf8de436344d924e0)

#### `public void DisablePacketEncryption()` 

Disables encryption for all packet types.

**See also**: [Transport::DisablePacketEncryption](#classyojimbo_1_1_transport_1ab319325cba1edddf68ebf2f4fa593acb)


**See also**: [Transport::DisableEncryptionForPacketType](#classyojimbo_1_1_transport_1aa102f26f89f0b48bf8de436344d924e0)

#### `public void DisableEncryptionForPacketType(int type)` 

Disables encryption for a specific packet type.

Typical usage is to enable packet encryption (for all types) via [Transport::EnablePacketEncryption](#classyojimbo_1_1_transport_1ac3c1c9cd04e06ce14af28c2097d960e8), and then selectively disable it for packet types that you don't want to be encrypted.

For example, the client/server protocol sends connection request packets as unencrypted, because they contain connect token data which is already encrypted, and every other packet sent is encrypted.


#### Parameters
* `type` The packet type that should be set as not encrypted.

#### `public bool IsEncryptedPacketType(int type) const` 

Is a packet type encrypted?

#### Returns
True if the packet type is an encrypted packet, false otherwise.


**See also**: [EnablePacketEncryption](#classyojimbo_1_1_transport_1ac3c1c9cd04e06ce14af28c2097d960e8)


**See also**: [DisablePacketEncryption](#classyojimbo_1_1_transport_1ab319325cba1edddf68ebf2f4fa593acb)


**See also**: [DisableEncryptionForPacketType](#classyojimbo_1_1_transport_1aa102f26f89f0b48bf8de436344d924e0)


#### Returns
True if the packet type is encrypted, false otherwise.

#### `public bool AddEncryptionMapping(const `[`Address`](#classyojimbo_1_1_address)` & address,const uint8_t * sendKey,const uint8_t * receiveKey,double timeout)` 

Associates an address with keys for packet encryption.

This mapping is used by the transport on packet send and receive determine what key should be used when sending a packet to an address, and what key should be used to decrypt a packet received from an address.

For example, the server adds an encryption mapping for clients when it receives a connection request packet with a valid connect token, enabling encrypted packets to be exchanged between the server and that client past that point. The encryption mapping is removed when the client disconnects from the server.

Encryption mappings also time out, making them ideal for pending clients, who may never reply back and complete the connection negotiation. As a result, there are more encryption mappings than client slots. See [yojimbo::MaxEncryptionMappings](#namespaceyojimbo_1ac788010bac7b62c964e84bbfd35fa5a4).

See [EncryptionManager](#classyojimbo_1_1_encryption_manager) for further details.


#### Parameters
* `address` The address to associate with encryption keys. 


* `sendKey` The key used to encrypt packets sent to this address. 


* `receiveKey` The key used to decrypt packets received from this address. 


* `timeout` The timeout value in seconds for this encryption mapping (seconds). Encyrption mapping times out if no packets are sent to or received from this address in the timeout period.





#### Returns
True if the encryption mapping was added successfully, false otherwise.

#### `public bool RemoveEncryptionMapping(const `[`Address`](#classyojimbo_1_1_address)` & address)` 

Remove the encryption mapping for an address.

#### Parameters
* `address` The address of the encryption mapping to remove.





#### Returns
True if an encryption mapping for the address exists and was removed, false if no encryption mapping could be found for the address.

#### `public int FindEncryptionMapping(const `[`Address`](#classyojimbo_1_1_address)` & address)` 

Find the index of an encryption mapping by address.

#### Returns
The index of the encryption mapping in the range [0,MaxEncryptionMappings-1], or -1 if no encryption mapping exists for the specified address.

#### `public void ResetEncryptionMappings()` 

Reset all encryption mappings.

All encryption mappings set on the transport are removed.


**See also**: [Transport::Reset](#classyojimbo_1_1_transport_1a3e113502a15c39867026864b72b2d125)

#### `public bool AddContextMapping(const `[`Address`](#classyojimbo_1_1_address)` & address,const `[`TransportContext`](#structyojimbo_1_1_transport_context)` & context)` 

Add a transport context that is specific for an address.

Context mappings are used to give each connected client on the server its own set of resources, so malicious clients can't exhaust resources shared with other clients.

When a client establishes connection with the server, a context mapping is added for that client. When the client disconnects from the server, the context mapping is removed.


#### Parameters
* `address` The address to associate with a transport context. 


* `context` The transport context to be copied across. All data in the context must remain valid while it is set on the transport.





**See also**: [TransportContextManager](#classyojimbo_1_1_transport_context_manager)


**See also**: [yojimbo::MaxContextMappings](#namespaceyojimbo_1a3eb912b430ab10ab4afc8737d3ea883a)

#### `public bool RemoveContextMapping(const `[`Address`](#classyojimbo_1_1_address)` & address)` 

Remove a context mapping.

Context mappings are used to give each connected client on the server its own set of resources, so malicious clients can't exhaust resources shared with other clients.

When a client establishes connection with the server, a context mapping is added for that client. When the client disconnects from the server, the context mapping is removed.


#### Parameters
* `address` The address of the context to remove. 





#### Returns
True if a context mapping was found at the address and removed, false if no context mapping could be found for the address.

#### `public void ResetContextMappings()` 

Reset all context mappings.

After this function is called all context mappings are removed.

#### `public void AdvanceTime(double time)` 

Advance transport time.

Call this at the end of each frame to advance the transport time forward.

IMPORTANT: Please use a double for your time value so it maintains sufficient accuracy as time increases.

#### `public double GetTime() const` 

Gets the current transport time.

**See also**: [Transport::AdvanceTime](#classyojimbo_1_1_transport_1afbc173c15dc6a2d2eee3ac1f93d47d90)

#### `public uint64_t GetCounter(int index) const` 

Get a counter value.

Counters are used to track event and actions performed by the transport. They are useful for debugging, testing and telemetry.


#### Returns
The counter value. See [yojimbo::TransportCounters](#namespaceyojimbo_1a2d157756c1035b5c7f088771cd9aca07) for the set of transport counters.

#### `public void ResetCounters()` 

Reset all counters to zero.

This is typically used with a telemetry application after uploading the current set of counters to the telemetry backend.

This way you can continue to accumulate events, and upload them at some frequency, like every 5 minutes to the telemetry backend, without double counting events.

#### `public void SetFlags(uint64_t flags)` 

Set transport flags.

Flags are used to enable and disable transport functionality.


#### Parameters
* `flags` The transport flags to set. See [yojimbo::TransportFlags](#namespaceyojimbo_1a597f4f1f57cad3cfc85f0d89f74cbd6d) for the set of transport flags that can be passed in.





**See also**: [Transport::GetFlags](#classyojimbo_1_1_transport_1aa57ac8bdf036a39e4cf9952bb1f9753d)

#### `public uint64_t GetFlags() const` 

Get the current transport flags.

#### Returns
The transport flags. See [yojimbo::TransportFlags](#namespaceyojimbo_1a597f4f1f57cad3cfc85f0d89f74cbd6d) for the set of transport flags.


**See also**: [Transport::SetFlags](#classyojimbo_1_1_transport_1aa7cd42ebe2b149a71ed05438c76d3417)

#### `public const `[`Address`](#classyojimbo_1_1_address)` & GetAddress() const` 

Get the address of the transport.

This is the address that packet should be sent to to be received by this transport.


#### Returns
The transport address.

#### `public uint64_t GetProtocolId() const` 

Get the protocol id for the transport.

Protocol id is used to enable multiple versions of your protocol at the same time, by excluding communication between protocols with different ids.

# class `yojimbo::TransportContextManager` 


Maps addresses to transport contexts so each client on the server can have its own set of resources.

Typically, one context mapping is added to the transport per-connected client, allowing the server to silo each client to its own set of resources, eliminating the risk of malicious clients depleting shared resources on the server.

This is the implementation class for the [Transport::AddContextMapping](#classyojimbo_1_1_transport_1a4d0e96d87d6a7b3890ca077614e7a4e8), [Transport::RemoveContextMapping](#classyojimbo_1_1_transport_1a1faec80840144c109f266b403a186a13) and [Transport::ResetContextMappings](#classyojimbo_1_1_transport_1ad7710e1146051a542f083b2894ecee8b) set of functions. It allows different transport implementations to reuse the same context mappping implementation.


**See also**: [TransportContext](#structyojimbo_1_1_transport_context)


**See also**: [Transport::AddContextMapping](#classyojimbo_1_1_transport_1a4d0e96d87d6a7b3890ca077614e7a4e8)


**See also**: [Transport::RemoveContextMapping](#classyojimbo_1_1_transport_1a1faec80840144c109f266b403a186a13)


**See also**: [Transport::ResetContextMappings](#classyojimbo_1_1_transport_1ad7710e1146051a542f083b2894ecee8b)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  TransportContextManager()` | 
`public bool AddContextMapping(const `[`Address`](#classyojimbo_1_1_address)` & address,const `[`TransportContext`](#structyojimbo_1_1_transport_context)` & context)` | Add a context mapping.
`public bool RemoveContextMapping(const `[`Address`](#classyojimbo_1_1_address)` & address)` | Remove a context mapping.
`public void ResetContextMappings()` | Reset all context mappings.
`public const `[`TransportContext`](#structyojimbo_1_1_transport_context)` * GetContext(const `[`Address`](#classyojimbo_1_1_address)` & address) const` | Find a context by address.

## Members

#### `public  TransportContextManager()` 





#### `public bool AddContextMapping(const `[`Address`](#classyojimbo_1_1_address)` & address,const `[`TransportContext`](#structyojimbo_1_1_transport_context)` & context)` 

Add a context mapping.

Maps the specified address to the context mapping. If a context mapping already exists for this address, it is updated with the new context.

IMPORTANT: The context data is copied across by value and not stored by pointer. All pointers and data inside the context are expected to stay valid while set on the transport.


#### Parameters
* `address` The address that maps to the context. 


* `context` A reference to the context data to be copied across. 





#### Returns
True if the context mapping was added successfully. False if the context is already added context mapping slots are avalable. See [yojimbo::MaxContextMappings](#namespaceyojimbo_1a3eb912b430ab10ab4afc8737d3ea883a).

#### `public bool RemoveContextMapping(const `[`Address`](#classyojimbo_1_1_address)` & address)` 

Remove a context mapping.

Searches for a context mapping for the address passed in, and if it finds one, removes it.


#### Returns
True if a context mapping was found and removed, false otherwise.

#### `public void ResetContextMappings()` 

Reset all context mappings.

After this function all context mappings are removed.

Call this if you want to completely reset the context mappings and start fresh.

#### `public const `[`TransportContext`](#structyojimbo_1_1_transport_context)` * GetContext(const `[`Address`](#classyojimbo_1_1_address)` & address) const` 

Find a context by address.

This is called by the transport when a packet is sent or received, to find the appropriate context to use given the address the packet is being sent to, or received from.


#### Parameters
* `address` The address corresponding to the context we want. 





#### Returns
A const pointer to the context if one exists for the address passed in, NULL otherwise.

# class `yojimbo::UnreliableUnorderedChannel` 

```
class yojimbo::UnreliableUnorderedChannel
  : public yojimbo::Channel
```  

Messages sent across this channel are not guaranteed to arrive, and may be received in a different order than they were sent.

This channel type is best used for time critical data like snapshots and object state.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  UnreliableUnorderedChannel(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,`[`MessageFactory`](#classyojimbo_1_1_message_factory)` & messageFactory,const `[`ChannelConfig`](#structyojimbo_1_1_channel_config)` & config,int channelId)` | Reliable ordered channel constructor.
`public  ~UnreliableUnorderedChannel()` | Unreliable unordered channel destructor.
`public virtual void Reset()` | Reset the channel.
`public virtual bool CanSendMsg() const` | Returns true if a message can be sent over this channel.
`public virtual void SendMsg(`[`Message`](#classyojimbo_1_1_message)` * message)` | [Queue](#classyojimbo_1_1_queue) a message to be sent across this channel.
`public virtual `[`Message`](#classyojimbo_1_1_message)` * ReceiveMsg()` | Pops the next message off the receive queue if one is available.
`public virtual void AdvanceTime(double time)` | Advance channel time.
`public virtual int GetPacketData(`[`ChannelPacketData`](#structyojimbo_1_1_channel_packet_data)` & packetData,uint16_t packetSequence,int availableBits)` | Get channel packet data for this channel.
`public virtual void ProcessPacketData(const `[`ChannelPacketData`](#structyojimbo_1_1_channel_packet_data)` & packetData,uint16_t packetSequence)` | Process packet data included in a connection packet.
`public virtual void ProcessAck(uint16_t sequence)` | Process a connection packet ack.
`protected `[`Queue](#classyojimbo_1_1_queue)< [Message`](#classyojimbo_1_1_message)` * > * m_messageSendQueue` | [Message](#classyojimbo_1_1_message) send queue.
`protected `[`Queue](#classyojimbo_1_1_queue)< [Message`](#classyojimbo_1_1_message)` * > * m_messageReceiveQueue` | [Message](#classyojimbo_1_1_message) receive queue.

## Members

#### `public  UnreliableUnorderedChannel(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,`[`MessageFactory`](#classyojimbo_1_1_message_factory)` & messageFactory,const `[`ChannelConfig`](#structyojimbo_1_1_channel_config)` & config,int channelId)` 

Reliable ordered channel constructor.

#### Parameters
* `allocator` The allocator to use. 


* `messageFactory` [Message](#classyojimbo_1_1_message) factory for creating and destroying messages. 


* `config` The configuration for this channel. 


* `channelId` The channel id in [0,numChannels-1].

#### `public  ~UnreliableUnorderedChannel()` 

Unreliable unordered channel destructor.

Any messages still in the send or receive queues will be released.

#### `public virtual void Reset()` 

Reset the channel.



#### `public virtual bool CanSendMsg() const` 

Returns true if a message can be sent over this channel.



#### `public virtual void SendMsg(`[`Message`](#classyojimbo_1_1_message)` * message)` 

[Queue](#classyojimbo_1_1_queue) a message to be sent across this channel.

#### Parameters
* `message` The message to be sent.

#### `public virtual `[`Message`](#classyojimbo_1_1_message)` * ReceiveMsg()` 

Pops the next message off the receive queue if one is available.

#### Returns
A pointer to the received message, NULL if there are no messages to receive. The caller owns the message object returned by this function and is responsible for releasing it via [Message::Release](#classyojimbo_1_1_message_1a5979f1559786c1606dde8df944b24d26).

#### `public virtual void AdvanceTime(double time)` 

Advance channel time.

Called by [Connection::AdvanceTime](#classyojimbo_1_1_connection_1a268d70c87da4a4185bfe6b947b8e7b7a) for each channel configured on the connection.

#### `public virtual int GetPacketData(`[`ChannelPacketData`](#structyojimbo_1_1_channel_packet_data)` & packetData,uint16_t packetSequence,int availableBits)` 

Get channel packet data for this channel.

#### Parameters
* `packetData` The channel packet data to be filled [out] 


* `packetSequence` The sequence number of the packet being generated. 


* `availableBits` The maximum number of bits of packet data the channel is allowed to write.





#### Returns
The number of bits of packet data written by the channel.


**See also**: [ConnectionPacket](#structyojimbo_1_1_connection_packet)


**See also**: [Connection::GeneratePacket](#classyojimbo_1_1_connection_1aaf8a19bcdd9ad05ff598b6e975881290)

#### `public virtual void ProcessPacketData(const `[`ChannelPacketData`](#structyojimbo_1_1_channel_packet_data)` & packetData,uint16_t packetSequence)` 

Process packet data included in a connection packet.

#### Parameters
* `packetData` The channel packet data to process. 


* `packetSequence` The sequence number of the connection packet that contains the channel packet data.





**See also**: [ConnectionPacket](#structyojimbo_1_1_connection_packet)


**See also**: [Connection::ProcessPacket](#classyojimbo_1_1_connection_1a6118515c3a0f1764e5f27a378df87886)

#### `public virtual void ProcessAck(uint16_t sequence)` 

Process a connection packet ack.

Depending on the channel type: 1. Acks messages and block fragments so they stop being included in outgoing connection packets (reliable-ordered channel)

2. Does nothing at all (unreliable-unordered).



#### Parameters
* `sequence` The sequence number of the connection packet that was acked.

#### `protected `[`Queue](#classyojimbo_1_1_queue)< [Message`](#classyojimbo_1_1_message)` * > * m_messageSendQueue` 

[Message](#classyojimbo_1_1_message) send queue.



#### `protected `[`Queue](#classyojimbo_1_1_queue)< [Message`](#classyojimbo_1_1_message)` * > * m_messageReceiveQueue` 

[Message](#classyojimbo_1_1_message) receive queue.



# class `yojimbo::WriteStream` 

```
class yojimbo::WriteStream
  : public yojimbo::BaseStream
```  

Stream class for writing bitpacked data.

This class is a wrapper around the bit writer class. Its purpose is to provide unified interface for reading and writing.

You can determine if you are writing to a stream by calling Stream::IsWriting inside your templated serialize method.

This is evaluated at compile time, letting the compiler generate optimized serialize functions without the hassle of maintaining separate read and write functions.

IMPORTANT: Generally, you don't call methods on this class directly. Use the serialize_* macros instead. See test/shared.h for some examples.


**See also**: [BitWriter](#classyojimbo_1_1_bit_writer)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  WriteStream(uint8_t * buffer,int bytes,`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator)` | Write stream constructor.
`public inline bool SerializeInteger(int32_t value,int32_t min,int32_t max)` | Serialize an integer (write).
`public inline bool SerializeBits(uint32_t value,int bits)` | Serialize a number of bits (write).
`public inline bool SerializeBytes(const uint8_t * data,int bytes)` | Serialize an array of bytes (write).
`public inline bool SerializeAlign()` | Serialize an align (write).
`public inline int GetAlignBits() const` | If we were to write an align right now, how many bits would be required?
`public inline bool SerializeCheck()` | Serialize a safety check to the stream (write).
`public inline void Flush()` | Flush the stream to memory after you finish writing.
`public inline const uint8_t * GetData() const` | Get a pointer to the data written by the stream.
`public inline int GetBytesProcessed() const` | How many bytes have been written so far?
`public inline int GetBitsProcessed() const` | Get number of bits written so far.

## Members

#### `public inline  WriteStream(uint8_t * buffer,int bytes,`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator)` 

Write stream constructor.

#### Parameters
* `buffer` The buffer to write to. 


* `bytes` The number of bytes in the buffer. Must be a multiple of four. 


* `allocator` The allocator to use for stream allocations. This lets you dynamically allocate memory as you read and write packets.

#### `public inline bool SerializeInteger(int32_t value,int32_t min,int32_t max)` 

Serialize an integer (write).

#### Parameters
* `value` The integer value in [min,max]. 


* `min` The minimum value. 


* `max` The maximum value.





#### Returns
Always returns true. All checking is performed by debug asserts only on write.

#### `public inline bool SerializeBits(uint32_t value,int bits)` 

Serialize a number of bits (write).

#### Parameters
* `value` The unsigned integer value to serialize. Must be in range [0,(1<<bits)-1]. 


* `bits` The number of bits to write in [1,32].





#### Returns
Always returns true. All checking is performed by debug asserts on write.

#### `public inline bool SerializeBytes(const uint8_t * data,int bytes)` 

Serialize an array of bytes (write).

#### Parameters
* `data` Array of bytes to be written. 


* `bytes` The number of bytes to write.





#### Returns
Always returns true. All checking is performed by debug asserts on write.

#### `public inline bool SerializeAlign()` 

Serialize an align (write).

#### Returns
Always returns true. All checking is performed by debug asserts on write.

#### `public inline int GetAlignBits() const` 

If we were to write an align right now, how many bits would be required?

#### Returns
The number of zero pad bits required to achieve byte alignment in [0,7].

#### `public inline bool SerializeCheck()` 

Serialize a safety check to the stream (write).

Safety checks help track down desyncs. A check is written to the stream, and on the other side if the check is not present it asserts and fails the serialize.


#### Returns
Always returns true. All checking is performed by debug asserts on write.

#### `public inline void Flush()` 

Flush the stream to memory after you finish writing.

Always call this after you finish writing and before you call [WriteStream::GetData](#classyojimbo_1_1_write_stream_1af56a1ac333a186c8b423f8ed01f351e3), or you'll potentially truncate the last dword of data you wrote.


**See also**: [BitWriter::FlushBits](#classyojimbo_1_1_bit_writer_1a03fb30b2bb170ace192f883334995cd3)

#### `public inline const uint8_t * GetData() const` 

Get a pointer to the data written by the stream.

IMPORTANT: Call [WriteStream::Flush](#classyojimbo_1_1_write_stream_1af0580123440cb180506029dd150f7ca9) before you call this function!


#### Returns
A pointer to the data written by the stream

#### `public inline int GetBytesProcessed() const` 

How many bytes have been written so far?

#### Returns
Number of bytes written. This is effectively the packet size.

#### `public inline int GetBitsProcessed() const` 

Get number of bits written so far.

#### Returns
Number of bits written.

# struct `yojimbo::BitsRequired` 


Calculates the number of bits required to serialize an integer value in [min,max] at compile time.

**See also**: [Log2](#structyojimbo_1_1_log2)


**See also**: [PopCount](#structyojimbo_1_1_pop_count)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

## Members

# struct `yojimbo::ChallengePacket` 

```
struct yojimbo::ChallengePacket
  : public yojimbo::Packet
```  

Sent from server to client in response to valid connection request packets, provided that the server is not full.

This challenge/response is done so the server can trust that the client is actually at the packet source address it says it is in the connection request packet.

The server only completes connection once the client responds with data that the client cannot possibly know, unless it receives this packet sent to it.

IMPORTANT: Intentially smaller than the connection request packet, to make DDoS amplification attacks impossible.


**See also**: [ChallengeToken](#structyojimbo_1_1_challenge_token)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public uint8_t challengeTokenData` | Encrypted challenge token data generated be server in response to a connection request.
`public uint8_t challengeTokenNonce` | Nonce required to decrypt the challenge token on the server. Effectively a sequence number which is incremented each time a new challenge token is generated by the server.
`public inline  ChallengePacket()` | 
`public template<typename Stream>`  <br/>`inline bool Serialize(Stream & stream)` | 
`public  YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()` | 

## Members

#### `public uint8_t challengeTokenData` 

Encrypted challenge token data generated be server in response to a connection request.



#### `public uint8_t challengeTokenNonce` 

Nonce required to decrypt the challenge token on the server. Effectively a sequence number which is incremented each time a new challenge token is generated by the server.



#### `public inline  ChallengePacket()` 





#### `public template<typename Stream>`  <br/>`inline bool Serialize(Stream & stream)` 





#### `public  YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()` 





# struct `yojimbo::ChallengeResponsePacket` 

```
struct yojimbo::ChallengeResponsePacket
  : public yojimbo::Packet
```  

Sent from client to server in response to a challenge packet.

This packet is sent back to the server, so the server knows that the client is really at the address they said they were in the connection packet.


**See also**: [ChallengeToken](#structyojimbo_1_1_challenge_token)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public uint8_t challengeTokenData` | Encrypted challenge token data generated by server.
`public uint8_t challengeTokenNonce` | Nonce required to decrypt the challenge token on the server.
`public inline  ChallengeResponsePacket()` | 
`public template<typename Stream>`  <br/>`inline bool Serialize(Stream & stream)` | 
`public  YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()` | 

## Members

#### `public uint8_t challengeTokenData` 

Encrypted challenge token data generated by server.



#### `public uint8_t challengeTokenNonce` 

Nonce required to decrypt the challenge token on the server.



#### `public inline  ChallengeResponsePacket()` 





#### `public template<typename Stream>`  <br/>`inline bool Serialize(Stream & stream)` 





#### `public  YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()` 





# struct `yojimbo::ChallengeToken` 


Data stored inside the encrypted challenge token.

Sent from server to client in response to a connection request with a valid connect token.

The purpose of this challenge/response does is ensure that a potential client is able to receive packets on the source packet address for the connection request packet.

This stops clients from connecting with spoofed packet source addresses.


**See also**: [ConnectionRequestPacket](#structyojimbo_1_1_connection_request_packet)


**See also**: [ChallengeResponsePacket](#structyojimbo_1_1_challenge_response_packet)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public uint64_t clientId` | The unique client id. Maximum of one connection per-client id, per-server at any time.
`public uint8_t connectTokenMac` | Mac of the initial connect token this challenge corresponds to. Used to quickly map the challenge response from a client to a pending connection entry on the server.
`public uint8_t clientToServerKey` | The key for encrypted communication from client -> server.
`public uint8_t serverToClientKey` | The key for encrypted communication from server -> client.
`public inline  ChallengeToken()` | 
`public template<typename Stream>`  <br/>`inline bool Serialize(Stream & stream)` | 

## Members

#### `public uint64_t clientId` 

The unique client id. Maximum of one connection per-client id, per-server at any time.



#### `public uint8_t connectTokenMac` 

Mac of the initial connect token this challenge corresponds to. Used to quickly map the challenge response from a client to a pending connection entry on the server.



#### `public uint8_t clientToServerKey` 

The key for encrypted communication from client -> server.



#### `public uint8_t serverToClientKey` 

The key for encrypted communication from server -> client.



#### `public inline  ChallengeToken()` 





#### `public template<typename Stream>`  <br/>`inline bool Serialize(Stream & stream)` 





# struct `yojimbo::ChannelConfig` 


Configuration properties for a message channel.

Channels let you specify different reliability and ordering guarantees for messages sent across a connection.

They may be configured as one of two types: reliable-ordered or unreliable-unordered.

Reliable ordered channels guarantee that messages (see [Message](#classyojimbo_1_1_message)) are received reliably and in the same order they were sent. This channel type is designed for control messages and RPCs sent between the client and server.

Unreliable unordered channels are like UDP. There is no guarantee that messages will arrive, and messages may arrive out of order. This channel type is designed for data that is time critical and should not be resent if dropped, like snapshots of world state sent rapidly from server to client, or cosmetic events such as effects and sounds.

Both channel types support blocks of data attached to messages (see [BlockMessage](#classyojimbo_1_1_block_message)), but their treatment of blocks is quite different.

Reliable ordered channels are designed for blocks that must be received reliably and in-order with the rest of the messages sent over the channel. Examples of these sort of blocks include the initial state of a level, or server configuration data sent down to a client on connect. These blocks are sent by splitting them into fragments and resending each fragment until the other side has received the entire block. This allows for sending blocks of data larger that maximum packet size quickly and reliably even under packet loss.

Unreliable-unordered channels send blocks as-is without splitting them up into fragments. The idea is that transport level packet fragmentation should be used on top of the generated packet to split it up into into smaller packets that can be sent across typical internet MTU (<1500 bytes). Because of this, you need to make sure that the maximum block size for an unreliable-unordered channel fits within the maximum packet size.

Channels are typically configured as part of a [ConnectionConfig](#structyojimbo_1_1_connection_config), which is included inside the [ClientServerConfig](#structyojimbo_1_1_client_server_config) that is passed into the [Client](#classyojimbo_1_1_client) and [Server](#classyojimbo_1_1_server) constructors.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public `[`ChannelType`](#namespaceyojimbo_1af4ea754d67a17488294fd1731e1acd35)` type` | [Channel](#classyojimbo_1_1_channel) type: reliable-ordered or unreliable-unordered.
`public bool disableBlocks` | Disables blocks being sent across this channel.
`public int sendQueueSize` | Number of messages in the send queue for this channel.
`public int receiveQueueSize` | Number of messages in the receive queue for this channel.
`public int sentPacketBufferSize` | Maps packet level acks to individual messages & fragments. Please consider your packet send rate and make sure you have at least a few seconds worth of entries in this buffer.
`public int maxMessagesPerPacket` | Maximum number of messages to include in each packet. Will write up to this many messages, provided the messages fit into the channel packet budget and the number of bytes remaining in the packet.
`public int packetBudget` | Maximum amount of message data to write to the packet for this channel (bytes). Specifying -1 means the channel can use up to the rest of the bytes remaining in the packet.
`public int maxBlockSize` | The size of the largest block that can be sent across this channel (bytes).
`public int fragmentSize` | Blocks are split up into fragments of this size when sent over a reliabled-ordered channel (bytes).
`public float messageResendTime` | Minimum delay between message resends (seconds). Avoids sending the same message too frequently.
`public float fragmentResendTime` | Minimum delay between fragment resends (seconds). Avoids sending the same fragment too frequently.
`public inline  ChannelConfig()` | 
`public inline int GetMaxFragmentsPerBlock() const` | 

## Members

#### `public `[`ChannelType`](#namespaceyojimbo_1af4ea754d67a17488294fd1731e1acd35)` type` 

[Channel](#classyojimbo_1_1_channel) type: reliable-ordered or unreliable-unordered.



#### `public bool disableBlocks` 

Disables blocks being sent across this channel.



#### `public int sendQueueSize` 

Number of messages in the send queue for this channel.



#### `public int receiveQueueSize` 

Number of messages in the receive queue for this channel.



#### `public int sentPacketBufferSize` 

Maps packet level acks to individual messages & fragments. Please consider your packet send rate and make sure you have at least a few seconds worth of entries in this buffer.



#### `public int maxMessagesPerPacket` 

Maximum number of messages to include in each packet. Will write up to this many messages, provided the messages fit into the channel packet budget and the number of bytes remaining in the packet.



#### `public int packetBudget` 

Maximum amount of message data to write to the packet for this channel (bytes). Specifying -1 means the channel can use up to the rest of the bytes remaining in the packet.



#### `public int maxBlockSize` 

The size of the largest block that can be sent across this channel (bytes).



#### `public int fragmentSize` 

Blocks are split up into fragments of this size when sent over a reliabled-ordered channel (bytes).



#### `public float messageResendTime` 

Minimum delay between message resends (seconds). Avoids sending the same message too frequently.



#### `public float fragmentResendTime` 

Minimum delay between fragment resends (seconds). Avoids sending the same fragment too frequently.



#### `public inline  ChannelConfig()` 





#### `public inline int GetMaxFragmentsPerBlock() const` 





# struct `yojimbo::ChannelPacketData` 


Per-channel data inside a connection packet.

**See also**: [ConnectionPacket](#structyojimbo_1_1_connection_packet)


**See also**: [Connection::GeneratePacket](#classyojimbo_1_1_connection_1aaf8a19bcdd9ad05ff598b6e975881290)


**See also**: Channel::GeneratePacketData 


**See also**: [Channel::ProcessPacketData](#classyojimbo_1_1_channel_1a89231620ae60d7addb924edb80562a75)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public uint32_t channelId` | The id of the channel this data belongs to in [0,numChannels-1].
`public uint32_t initialized` | 1 if this channel packet data was properly initialized, 0 otherwise. This is a safety measure to make sure [ChannelPacketData::Initialize](#structyojimbo_1_1_channel_packet_data_1a7b5b9aa863200e2d276ff0a9e209cd5a) gets called.
`public uint32_t blockMessage` | 1 if this channel data contains data for a block (eg. a fragment of that block), 0 if this channel data contains messages.
`public uint32_t messageFailedToSerialize` | Set to 1 if a message for this channel fails to serialized. Used to set CHANNEL_ERROR_FAILED_TO_SERIALIZE on the [Channel](#classyojimbo_1_1_channel) object.
`public `[`MessageData`](#structyojimbo_1_1_channel_packet_data_1_1_message_data)` message` | Data for sending messages.
`public `[`BlockData`](#structyojimbo_1_1_channel_packet_data_1_1_block_data)` block` | Data for sending a block fragment.
`public union yojimbo::ChannelPacketData::@1 @2` | 
`public void Initialize()` | Initialize the channel packet data to default values.
`public void Free(`[`MessageFactory`](#classyojimbo_1_1_message_factory)` & messageFactory)` | Release messages stored in channel packet data and free allocations.
`public template<typename Stream>`  <br/>`bool Serialize(Stream & stream,`[`MessageFactory`](#classyojimbo_1_1_message_factory)` & messageFactory,const `[`ChannelConfig`](#structyojimbo_1_1_channel_config)` * channelConfigs,int numChannels)` | Templated serialize function for the channel packet data.
`public bool SerializeInternal(`[`ReadStream`](#classyojimbo_1_1_read_stream)` & stream,`[`MessageFactory`](#classyojimbo_1_1_message_factory)` & messageFactory,const `[`ChannelConfig`](#structyojimbo_1_1_channel_config)` * channelConfigs,int numChannels)` | Implements serialize read by a calling into [ChannelPacketData::Serialize](#structyojimbo_1_1_channel_packet_data_1a17f36a5415fd6335feaccf5a28f2d198) with a [ReadStream](#classyojimbo_1_1_read_stream).
`public bool SerializeInternal(`[`WriteStream`](#classyojimbo_1_1_write_stream)` & stream,`[`MessageFactory`](#classyojimbo_1_1_message_factory)` & messageFactory,const `[`ChannelConfig`](#structyojimbo_1_1_channel_config)` * channelConfigs,int numChannels)` | Implements serialize write by a calling into [ChannelPacketData::Serialize](#structyojimbo_1_1_channel_packet_data_1a17f36a5415fd6335feaccf5a28f2d198) with a [WriteStream](#classyojimbo_1_1_write_stream).
`public bool SerializeInternal(`[`MeasureStream`](#classyojimbo_1_1_measure_stream)` & stream,`[`MessageFactory`](#classyojimbo_1_1_message_factory)` & messageFactory,const `[`ChannelConfig`](#structyojimbo_1_1_channel_config)` * channelConfigs,int numChannels)` | Implements serialize measure by a calling into [ChannelPacketData::Serialize](#structyojimbo_1_1_channel_packet_data_1a17f36a5415fd6335feaccf5a28f2d198) with a [MeasureStream](#classyojimbo_1_1_measure_stream).

## Members

#### `public uint32_t channelId` 

The id of the channel this data belongs to in [0,numChannels-1].



#### `public uint32_t initialized` 

1 if this channel packet data was properly initialized, 0 otherwise. This is a safety measure to make sure [ChannelPacketData::Initialize](#structyojimbo_1_1_channel_packet_data_1a7b5b9aa863200e2d276ff0a9e209cd5a) gets called.



#### `public uint32_t blockMessage` 

1 if this channel data contains data for a block (eg. a fragment of that block), 0 if this channel data contains messages.



#### `public uint32_t messageFailedToSerialize` 

Set to 1 if a message for this channel fails to serialized. Used to set CHANNEL_ERROR_FAILED_TO_SERIALIZE on the [Channel](#classyojimbo_1_1_channel) object.



#### `public `[`MessageData`](#structyojimbo_1_1_channel_packet_data_1_1_message_data)` message` 

Data for sending messages.



#### `public `[`BlockData`](#structyojimbo_1_1_channel_packet_data_1_1_block_data)` block` 

Data for sending a block fragment.



#### `public union yojimbo::ChannelPacketData::@1 @2` 





#### `public void Initialize()` 

Initialize the channel packet data to default values.



#### `public void Free(`[`MessageFactory`](#classyojimbo_1_1_message_factory)` & messageFactory)` 

Release messages stored in channel packet data and free allocations.

#### Parameters
* `messageFactory` The message factory used to release messages in this packet data.

#### `public template<typename Stream>`  <br/>`bool Serialize(Stream & stream,`[`MessageFactory`](#classyojimbo_1_1_message_factory)` & messageFactory,const `[`ChannelConfig`](#structyojimbo_1_1_channel_config)` * channelConfigs,int numChannels)` 

Templated serialize function for the channel packet data.

Unifies packet read and write, making it harder to accidentally desync.


#### Parameters
* `stream` The stream used for serialization. 


* `messageFactory` The message factory used to create message objects on serialize read. 


* `channelConfigs` Array of channel configs, indexed by channel id in [0,numChannels-1]. 


* `numChannels` The number of channels configured on the connection.

#### `public bool SerializeInternal(`[`ReadStream`](#classyojimbo_1_1_read_stream)` & stream,`[`MessageFactory`](#classyojimbo_1_1_message_factory)` & messageFactory,const `[`ChannelConfig`](#structyojimbo_1_1_channel_config)` * channelConfigs,int numChannels)` 

Implements serialize read by a calling into [ChannelPacketData::Serialize](#structyojimbo_1_1_channel_packet_data_1a17f36a5415fd6335feaccf5a28f2d198) with a [ReadStream](#classyojimbo_1_1_read_stream).



#### `public bool SerializeInternal(`[`WriteStream`](#classyojimbo_1_1_write_stream)` & stream,`[`MessageFactory`](#classyojimbo_1_1_message_factory)` & messageFactory,const `[`ChannelConfig`](#structyojimbo_1_1_channel_config)` * channelConfigs,int numChannels)` 

Implements serialize write by a calling into [ChannelPacketData::Serialize](#structyojimbo_1_1_channel_packet_data_1a17f36a5415fd6335feaccf5a28f2d198) with a [WriteStream](#classyojimbo_1_1_write_stream).



#### `public bool SerializeInternal(`[`MeasureStream`](#classyojimbo_1_1_measure_stream)` & stream,`[`MessageFactory`](#classyojimbo_1_1_message_factory)` & messageFactory,const `[`ChannelConfig`](#structyojimbo_1_1_channel_config)` * channelConfigs,int numChannels)` 

Implements serialize measure by a calling into [ChannelPacketData::Serialize](#structyojimbo_1_1_channel_packet_data_1a17f36a5415fd6335feaccf5a28f2d198) with a [MeasureStream](#classyojimbo_1_1_measure_stream).



# struct `yojimbo::ClientServerConfig` 


Configuration shared between client and server.

Passed to [Client](#classyojimbo_1_1_client) and [Server](#classyojimbo_1_1_server) constructors to configure their behavior.

Please make sure that the message configuration is identical between client and server or it will not work.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public int clientMemory` | Memory allocated inside [Client](#classyojimbo_1_1_client) for packets, messages and stream allocations (bytes)
`public int serverGlobalMemory` | Memory allocated inside [Server](#classyojimbo_1_1_server) for global connection request and challenge response packets (bytes)
`public int serverPerClientMemory` | Memory allocated inside [Server](#classyojimbo_1_1_server) for packets, messages and stream allocations per-client (bytes)
`public int numDisconnectPackets` | Number of disconnect packets to send on clean disconnect. Make sure the other side of the connection receives a disconnect packet and disconnects cleanly, even under packet loss. Without this, the other side times out and this ties up that slot on the server for an extended period.
`public float connectionNegotiationSendRate` | Send rate for packets sent during connection negotiation process. eg. connection request and challenge response packets (packets per-second).
`public float connectionNegotiationTimeOut` | [Connection](#classyojimbo_1_1_connection) negotiation times out if no response is received from the other side in this amount of time (seconds).
`public float connectionKeepAliveSendRate` | Keep alive packets are sent at this rate between client and server if no other packets are sent by the client or server. Avoids timeout in situations where you are not sending packets at a steady rate (packets per-second).
`public float connectionTimeOut` | Once a connection is established, it times out if it hasn't received any packets from the other side in this amount of time (seconds).
`public bool enableMessages` | If this is true then you can send messages between client and server. Set to false if you don't want to use messages and you want to extend the protocol by adding new packet types instead.
`public `[`ConnectionConfig`](#structyojimbo_1_1_connection_config)` connectionConfig` | Configures connection properties and message channels between client and server. Must be identical between client and server to work properly. Only used if enableMessages is true.
`public inline  ClientServerConfig()` | 

## Members

#### `public int clientMemory` 

Memory allocated inside [Client](#classyojimbo_1_1_client) for packets, messages and stream allocations (bytes)



#### `public int serverGlobalMemory` 

Memory allocated inside [Server](#classyojimbo_1_1_server) for global connection request and challenge response packets (bytes)



#### `public int serverPerClientMemory` 

Memory allocated inside [Server](#classyojimbo_1_1_server) for packets, messages and stream allocations per-client (bytes)



#### `public int numDisconnectPackets` 

Number of disconnect packets to send on clean disconnect. Make sure the other side of the connection receives a disconnect packet and disconnects cleanly, even under packet loss. Without this, the other side times out and this ties up that slot on the server for an extended period.



#### `public float connectionNegotiationSendRate` 

Send rate for packets sent during connection negotiation process. eg. connection request and challenge response packets (packets per-second).



#### `public float connectionNegotiationTimeOut` 

[Connection](#classyojimbo_1_1_connection) negotiation times out if no response is received from the other side in this amount of time (seconds).



#### `public float connectionKeepAliveSendRate` 

Keep alive packets are sent at this rate between client and server if no other packets are sent by the client or server. Avoids timeout in situations where you are not sending packets at a steady rate (packets per-second).



#### `public float connectionTimeOut` 

Once a connection is established, it times out if it hasn't received any packets from the other side in this amount of time (seconds).



#### `public bool enableMessages` 

If this is true then you can send messages between client and server. Set to false if you don't want to use messages and you want to extend the protocol by adding new packet types instead.



#### `public `[`ConnectionConfig`](#structyojimbo_1_1_connection_config)` connectionConfig` 

Configures connection properties and message channels between client and server. Must be identical between client and server to work properly. Only used if enableMessages is true.



#### `public inline  ClientServerConfig()` 





# struct `yojimbo::ConnectionConfig` 


Configures connection properties and the set of channels for sending and receiving messages.

Specifies the maximum packet size to generate, and the number of message channels, and the per-channel configuration data. See [ChannelConfig](#structyojimbo_1_1_channel_config) for details.

Typically configured as part of a [ClientServerConfig](#structyojimbo_1_1_client_server_config) which is passed into [Client](#classyojimbo_1_1_client) and [Server](#classyojimbo_1_1_server) constructors.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public int connectionPacketType` | [Connection](#classyojimbo_1_1_connection) packet type (so you can override it). Only necessary to set this if you are using [Connection](#classyojimbo_1_1_connection) directly. Not necessary to set when using client/server as it overrides it to CLIENT_SERVER_PACKET_CONNECTION for you automatically.
`public int slidingWindowSize` | The size of the sliding window used for packet acks (# of packets in history). Depending on your packet send rate, you should make sure this buffer is large enough to cover at least a few seconds worth of packets.
`public int maxPacketSize` | The maximum size of packets generated to transmit messages between client and server (bytes).
`public int numChannels` | Number of message channels in [1,MaxChannels]. Each message channel must have a corresponding configuration below.
`public `[`ChannelConfig`](#structyojimbo_1_1_channel_config)` channel` | Per-channel configuration. See [ChannelConfig](#structyojimbo_1_1_channel_config) for details.
`public inline  ConnectionConfig()` | 

## Members

#### `public int connectionPacketType` 

[Connection](#classyojimbo_1_1_connection) packet type (so you can override it). Only necessary to set this if you are using [Connection](#classyojimbo_1_1_connection) directly. Not necessary to set when using client/server as it overrides it to CLIENT_SERVER_PACKET_CONNECTION for you automatically.



#### `public int slidingWindowSize` 

The size of the sliding window used for packet acks (# of packets in history). Depending on your packet send rate, you should make sure this buffer is large enough to cover at least a few seconds worth of packets.



#### `public int maxPacketSize` 

The maximum size of packets generated to transmit messages between client and server (bytes).



#### `public int numChannels` 

Number of message channels in [1,MaxChannels]. Each message channel must have a corresponding configuration below.



#### `public `[`ChannelConfig`](#structyojimbo_1_1_channel_config)` channel` 

Per-channel configuration. See [ChannelConfig](#structyojimbo_1_1_channel_config) for details.



#### `public inline  ConnectionConfig()` 





# struct `yojimbo::ConnectionContext` 


Provides information required to read and write connection packets.

**See also**: [ConnectionPacket](#structyojimbo_1_1_connection_packet)


**See also**: Stream::SetContext 


**See also**: Stream::GetContext

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public uint32_t magic` | The magic number for safety checks. Set to [yojimbo::ConnectionContextMagic](#namespaceyojimbo_1a90165bd245a90a781bf9ca6b7b552613).
`public const `[`ConnectionConfig`](#structyojimbo_1_1_connection_config)` * connectionConfig` | The connection config. So we know the number of channels and how they are setup.
`public class `[`MessageFactory`](#classyojimbo_1_1_message_factory)` * messageFactory` | The message factory used for creating and destroying messages.
`public inline  ConnectionContext()` | 

## Members

#### `public uint32_t magic` 

The magic number for safety checks. Set to [yojimbo::ConnectionContextMagic](#namespaceyojimbo_1a90165bd245a90a781bf9ca6b7b552613).



#### `public const `[`ConnectionConfig`](#structyojimbo_1_1_connection_config)` * connectionConfig` 

The connection config. So we know the number of channels and how they are setup.



#### `public class `[`MessageFactory`](#classyojimbo_1_1_message_factory)` * messageFactory` 

The message factory used for creating and destroying messages.



#### `public inline  ConnectionContext()` 





# struct `yojimbo::ConnectionDeniedPacket` 

```
struct yojimbo::ConnectionDeniedPacket
  : public yojimbo::Packet
```  

Sent from server to client to deny a client connection.

This is only sent in when the server is full, in response to clients with valid client connect tokens who would otherwise be able to connect.

This lets clients get quickly notified that a server is full, so they can try the next server in their list rather than waiting for a potentially long timeout period (5-10 seconds).

All other situations where the client cannot connect (eg. invalid connect token, connect token timed out), will not get any response from the server. They will just be ignored.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public template<typename Stream>`  <br/>`inline bool Serialize(Stream &)` | 
`public  YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()` | 

## Members

#### `public template<typename Stream>`  <br/>`inline bool Serialize(Stream &)` 





#### `public  YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()` 





# struct `yojimbo::ConnectionPacket` 

```
struct yojimbo::ConnectionPacket
  : public yojimbo::Packet
```  

Implements packet level acks and carries messages across a connection.

[Connection](#classyojimbo_1_1_connection) packets should be generated and sent at a steady rate like 10, 20 or 30 times per-second in both directions across a connection.

The packet ack system is designed around this assumption. There are no separate ack packets!


**See also**: [GenerateAckBits](#namespaceyojimbo_1a0d9af22df17066a4705a81d1da4617de)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public uint16_t sequence` | The connection packet sequence number. Wraps around and keeps working. See [yojimbo::sequence_greater_than](#namespaceyojimbo_1ae1b7e3deef46222ecf7b3620838c7459) and [yojimbo::sequence_less_than](#namespaceyojimbo_1ad45f4fee5c716e4e1e057eebb9597689).
`public uint16_t ack` | The sequence number of the most recent packet received from the other side of the connection.
`public uint32_t ack_bits` | Bit n is set if packet ack - n was received from the other side of the connection. See [yojimbo::GenerateAckBits](#namespaceyojimbo_1a0d9af22df17066a4705a81d1da4617de).
`public int numChannelEntries` | The number of channel entries in this packet. Each channel entry corresponds to message data for a particular channel.
`public `[`ChannelPacketData`](#structyojimbo_1_1_channel_packet_data)` * channelEntry` | Per-channel message data that was included in this packet.
`public  ConnectionPacket()` | 
`public  ~ConnectionPacket()` | [Connection](#classyojimbo_1_1_connection) packet destructor.
`public bool AllocateChannelData(`[`MessageFactory`](#classyojimbo_1_1_message_factory)` & messageFactory,int numEntries)` | Allocate channel data in this packet.
`public template<typename Stream>`  <br/>`bool Serialize(Stream & stream)` | The template function for serializing the connection packet.
`public virtual bool SerializeInternal(`[`ReadStream`](#classyojimbo_1_1_read_stream)` & stream)` | Implements serialize read by calling into [ConnectionPacket::Serialize](#structyojimbo_1_1_connection_packet_1ae966870eedb3dfaa11692e4d93e110cd) with a [ReadStream](#classyojimbo_1_1_read_stream).
`public virtual bool SerializeInternal(`[`WriteStream`](#classyojimbo_1_1_write_stream)` & stream)` | Implements serialize write by calling into [ConnectionPacket::Serialize](#structyojimbo_1_1_connection_packet_1ae966870eedb3dfaa11692e4d93e110cd) with a [WriteStream](#classyojimbo_1_1_write_stream).
`public virtual bool SerializeInternal(`[`MeasureStream`](#classyojimbo_1_1_measure_stream)` & stream)` | Implements serialize measure by calling into [ConnectionPacket::Serialize](#structyojimbo_1_1_connection_packet_1ae966870eedb3dfaa11692e4d93e110cd) with a [MeasureStream](#classyojimbo_1_1_measure_stream).
`public inline void SetMessageFactory(`[`MessageFactory`](#classyojimbo_1_1_message_factory)` & messageFactory)` | 

## Members

#### `public uint16_t sequence` 

The connection packet sequence number. Wraps around and keeps working. See [yojimbo::sequence_greater_than](#namespaceyojimbo_1ae1b7e3deef46222ecf7b3620838c7459) and [yojimbo::sequence_less_than](#namespaceyojimbo_1ad45f4fee5c716e4e1e057eebb9597689).



#### `public uint16_t ack` 

The sequence number of the most recent packet received from the other side of the connection.



#### `public uint32_t ack_bits` 

Bit n is set if packet ack - n was received from the other side of the connection. See [yojimbo::GenerateAckBits](#namespaceyojimbo_1a0d9af22df17066a4705a81d1da4617de).



#### `public int numChannelEntries` 

The number of channel entries in this packet. Each channel entry corresponds to message data for a particular channel.



#### `public `[`ChannelPacketData`](#structyojimbo_1_1_channel_packet_data)` * channelEntry` 

Per-channel message data that was included in this packet.



#### `public  ConnectionPacket()` 





#### `public  ~ConnectionPacket()` 

[Connection](#classyojimbo_1_1_connection) packet destructor.

Releases all references to messages included in this packet.


**See also**: [Message](#classyojimbo_1_1_message)


**See also**: [MessageFactory](#classyojimbo_1_1_message_factory)


**See also**: [ChannelPacketData](#structyojimbo_1_1_channel_packet_data)

#### `public bool AllocateChannelData(`[`MessageFactory`](#classyojimbo_1_1_message_factory)` & messageFactory,int numEntries)` 

Allocate channel data in this packet.

The allocation is performed with the allocator that is set on the message factory.

When this is used on the server, the allocator corresponds to the per-client allocator corresponding to the client that is sending this connection packet. See Server::m_clientAllocator.

This is intended to silo each client to their own set of resources on the server, so malicious clients cannot launch an attack to deplete resources shared with other clients.


#### Parameters
* `messageFactory` The message factory used to create and destroy messages. 


* `numEntries` The number of channel entries to allocate. This corresponds to the number of channels that have data to include in the connection packet.





#### Returns
True if the allocation succeeded, false otherwise.


**See also**: [Connection::GeneratePacket](#classyojimbo_1_1_connection_1aaf8a19bcdd9ad05ff598b6e975881290)

#### `public template<typename Stream>`  <br/>`bool Serialize(Stream & stream)` 

The template function for serializing the connection packet.

Unifies packet read and write, making it harder to accidentally desync one from the other.

#### `public virtual bool SerializeInternal(`[`ReadStream`](#classyojimbo_1_1_read_stream)` & stream)` 

Implements serialize read by calling into [ConnectionPacket::Serialize](#structyojimbo_1_1_connection_packet_1ae966870eedb3dfaa11692e4d93e110cd) with a [ReadStream](#classyojimbo_1_1_read_stream).



#### `public virtual bool SerializeInternal(`[`WriteStream`](#classyojimbo_1_1_write_stream)` & stream)` 

Implements serialize write by calling into [ConnectionPacket::Serialize](#structyojimbo_1_1_connection_packet_1ae966870eedb3dfaa11692e4d93e110cd) with a [WriteStream](#classyojimbo_1_1_write_stream).



#### `public virtual bool SerializeInternal(`[`MeasureStream`](#classyojimbo_1_1_measure_stream)` & stream)` 

Implements serialize measure by calling into [ConnectionPacket::Serialize](#structyojimbo_1_1_connection_packet_1ae966870eedb3dfaa11692e4d93e110cd) with a [MeasureStream](#classyojimbo_1_1_measure_stream).



#### `public inline void SetMessageFactory(`[`MessageFactory`](#classyojimbo_1_1_message_factory)` & messageFactory)` 





# struct `yojimbo::ConnectionReceivedPacketData` 






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

## Members

# struct `yojimbo::ConnectionRequestPacket` 

```
struct yojimbo::ConnectionRequestPacket
  : public yojimbo::Packet
```  

Sent from client to server when a client is first requesting a connection.

This is the very first packet the server receives from a potential new client.

Contains a connect token which the server checks to make sure is valid. This is so the server only allows connections from authenticated clients.

IMPORTANT: All the data required to establish, authenticate and encrypt the connection is encoded inside the connect token data.

For insecure connects, please refer to [InsecureConnectPacket](#structyojimbo_1_1_insecure_connect_packet).


**See also**: [ConnectToken](#structyojimbo_1_1_connect_token)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public uint64_t connectTokenExpireTimestamp` | The timestamp when the connect token expires. Connect tokens are typically short lived (45 seconds only).
`public uint8_t connectTokenData` | Encrypted connect token data generated by matchmaker. See matcher.go.
`public uint8_t connectTokenNonce` | Nonce required to decrypt the connect token. Basically a sequence number. Increments with each connect token generated by matcher.go.
`public inline  ConnectionRequestPacket()` | 
`public template<typename Stream>`  <br/>`inline bool Serialize(Stream & stream)` | 
`public  YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()` | 

## Members

#### `public uint64_t connectTokenExpireTimestamp` 

The timestamp when the connect token expires. Connect tokens are typically short lived (45 seconds only).



#### `public uint8_t connectTokenData` 

Encrypted connect token data generated by matchmaker. See matcher.go.



#### `public uint8_t connectTokenNonce` 

Nonce required to decrypt the connect token. Basically a sequence number. Increments with each connect token generated by matcher.go.



#### `public inline  ConnectionRequestPacket()` 





#### `public template<typename Stream>`  <br/>`inline bool Serialize(Stream & stream)` 





#### `public  YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()` 





# struct `yojimbo::ConnectionSentPacketData` 






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public uint8_t acked` | 

## Members

#### `public uint8_t acked` 





# struct `yojimbo::ConnectToken` 


Connect token passed from the client to server when it requests a secure connection.

How do connect tokens work?

First, matcher.go generates a connect token equivalent structure in golang, encodes it to JSON, encrypts it, base64 encodes it and transmits it to the client over HTTPS.

The client receives this encrypted connect token over HTTPS when it requests a match, along with information about which servers to connect to, and the keys for packet encryption (hence the need for HTTPS).

The client takes the connect token and passes it to each server it tries to connect to inside connection request packets. The server inspects these connection request packets and only allows connections from clients with a valid connect token.

This creates a system where servers only allow connections from authenticated clients, effectively transmitting whatever authentication the backend has performed to the dedicated servers, without the dedicated servers and matcher needing to have any direct communication.


**See also**: [ConnectionRequestPacket](#structyojimbo_1_1_connection_request_packet)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public uint64_t protocolId` | The protocol id the connect token corresponds to. Filters out unrelated protocols from connecting.
`public uint64_t clientId` | The unique client id. Only one connection per-client id is allowed at a time on a server.
`public uint64_t expireTimestamp` | Timestamp when this connect token expires.
`public int numServerAddresses` | The number of server addresses in the connect token whitelist. Connect tokens are only valid to connect to the server addresses in this list.
`public `[`Address`](#classyojimbo_1_1_address)` serverAddresses` | This connect token only allows connection to these server addresses.
`public uint8_t clientToServerKey` | The key for encrypted communication from client -> server.
`public uint8_t serverToClientKey` | The key for encrypted communication from server -> client.
`public inline  ConnectToken()` | 
`public bool operator==(const `[`ConnectToken`](#structyojimbo_1_1_connect_token)` & other) const` | 
`public bool operator!=(const `[`ConnectToken`](#structyojimbo_1_1_connect_token)` & other) const` | 

## Members

#### `public uint64_t protocolId` 

The protocol id the connect token corresponds to. Filters out unrelated protocols from connecting.



#### `public uint64_t clientId` 

The unique client id. Only one connection per-client id is allowed at a time on a server.



#### `public uint64_t expireTimestamp` 

Timestamp when this connect token expires.



#### `public int numServerAddresses` 

The number of server addresses in the connect token whitelist. Connect tokens are only valid to connect to the server addresses in this list.



#### `public `[`Address`](#classyojimbo_1_1_address)` serverAddresses` 

This connect token only allows connection to these server addresses.



#### `public uint8_t clientToServerKey` 

The key for encrypted communication from client -> server.



#### `public uint8_t serverToClientKey` 

The key for encrypted communication from server -> client.



#### `public inline  ConnectToken()` 





#### `public bool operator==(const `[`ConnectToken`](#structyojimbo_1_1_connect_token)` & other) const` 





#### `public bool operator!=(const `[`ConnectToken`](#structyojimbo_1_1_connect_token)` & other) const` 





# struct `yojimbo::ConnectTokenEntry` 


Used by the server to remember and reject connect tokens that have already been used.

This protects against attacks where the same connect token is used to connect multiple clients to the server in a short period of time.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public double time` | The time for this entry. Used to replace the oldest entries once the connect token array fills up.
`public `[`Address`](#classyojimbo_1_1_address)` address` | [Address](#classyojimbo_1_1_address) of the client that sent this connect token. Binds a connect token to a particular address so it can't be exploited.
`public uint8_t mac` | HMAC of connect token. We use this to avoid replay attacks where the same token is sent repeatedly for different addresses.
`public inline  ConnectTokenEntry()` | 

## Members

#### `public double time` 

The time for this entry. Used to replace the oldest entries once the connect token array fills up.



#### `public `[`Address`](#classyojimbo_1_1_address)` address` 

[Address](#classyojimbo_1_1_address) of the client that sent this connect token. Binds a connect token to a particular address so it can't be exploited.



#### `public uint8_t mac` 

HMAC of connect token. We use this to avoid replay attacks where the same token is sent repeatedly for different addresses.



#### `public inline  ConnectTokenEntry()` 





# struct `yojimbo::DisconnectPacket` 

```
struct yojimbo::DisconnectPacket
  : public yojimbo::Packet
```  

Sent between client and server after connection is established when either side disconnects cleanly.

Speeds up clean disconnects, so the other side doesn't have to timeout before realizing it.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public template<typename Stream>`  <br/>`inline bool Serialize(Stream &)` | 
`public  YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()` | 

## Members

#### `public template<typename Stream>`  <br/>`inline bool Serialize(Stream &)` 





#### `public  YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()` 





# struct `yojimbo::InsecureConnectPacket` 

```
struct yojimbo::InsecureConnectPacket
  : public yojimbo::Packet
```  

Sent from client to server requesting an insecure connect.

Insecure connects don't have packet encryption, and don't support authentication. Any client that knows the server IP address can connect.

They are provided for use in development, eg. ease of connecting to development servers running on your LAN.

Don't use insecure connects in production! Make sure you #define YOJIMBO_SECURE_MODE 1 in production builds!

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public uint64_t clientId` | The unique client id that identifies this client to the web backend. Pass in a random number if you don't have one yet.
`public uint64_t clientSalt` | Random number rolled each time [Client::InsecureConnect](#classyojimbo_1_1_client_1acf884a58a6b98aef48d7063c6181a622) is called. Used to make client reconnects robust, by ignoring packets sent from previous client connect sessions from the same address.
`public inline  InsecureConnectPacket()` | 
`public template<typename Stream>`  <br/>`inline bool Serialize(Stream & stream)` | 
`public  YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()` | 

## Members

#### `public uint64_t clientId` 

The unique client id that identifies this client to the web backend. Pass in a random number if you don't have one yet.



#### `public uint64_t clientSalt` 

Random number rolled each time [Client::InsecureConnect](#classyojimbo_1_1_client_1acf884a58a6b98aef48d7063c6181a622) is called. Used to make client reconnects robust, by ignoring packets sent from previous client connect sessions from the same address.



#### `public inline  InsecureConnectPacket()` 





#### `public template<typename Stream>`  <br/>`inline bool Serialize(Stream & stream)` 





#### `public  YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()` 





# struct `yojimbo::KeepAlivePacket` 

```
struct yojimbo::KeepAlivePacket
  : public yojimbo::Packet
```  

Sent once client/server connection is established, but only if necessary to avoid time out.

Also used as a payload to transmit the client index down to the client after connection, so the client knows which client slot they were assigned ot.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public int clientIndex` | The index of the client in [0,maxClients-1]. Used to inform the client which client slot they were assigned to once they have connected.
`public uint64_t clientSalt` | Random number rolled on each call to [Client::InsecureConnect](#classyojimbo_1_1_client_1acf884a58a6b98aef48d7063c6181a622). Makes insecure reconnect much more robust, by distinguishing each connect session from previous ones.
`public inline  KeepAlivePacket()` | 
`public template<typename Stream>`  <br/>`inline bool Serialize(Stream & stream)` | 
`public  YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()` | 

## Members

#### `public int clientIndex` 

The index of the client in [0,maxClients-1]. Used to inform the client which client slot they were assigned to once they have connected.



#### `public uint64_t clientSalt` 

Random number rolled on each call to [Client::InsecureConnect](#classyojimbo_1_1_client_1acf884a58a6b98aef48d7063c6181a622). Makes insecure reconnect much more robust, by distinguishing each connect session from previous ones.



#### `public inline  KeepAlivePacket()` 





#### `public template<typename Stream>`  <br/>`inline bool Serialize(Stream & stream)` 





#### `public  YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()` 





# struct `yojimbo::Log2` 


Calculates the log 2 of an unsigned 32 bit integer at compile time.

**See also**: [yojimbo::Log2](#structyojimbo_1_1_log2)


**See also**: [yojimbo::BitsRequired](#structyojimbo_1_1_bits_required)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

## Members

# struct `yojimbo::MatcherInternal` 






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public mbedtls_net_context server_fd` | 
`public mbedtls_entropy_context entropy` | 
`public mbedtls_ctr_drbg_context ctr_drbg` | 
`public mbedtls_ssl_context ssl` | 
`public mbedtls_ssl_config conf` | 
`public mbedtls_x509_crt cacert` | 

## Members

#### `public mbedtls_net_context server_fd` 





#### `public mbedtls_entropy_context entropy` 





#### `public mbedtls_ctr_drbg_context ctr_drbg` 





#### `public mbedtls_ssl_context ssl` 





#### `public mbedtls_ssl_config conf` 





#### `public mbedtls_x509_crt cacert` 





# struct `yojimbo::MatchResponse` 


Response sent back from the matcher web service when the client requests a match.

IMPORTANT: This response is transmitted over HTTPS because it contains encryption keys for packets sent between the client and server.


**See also**: [Client::Connect](#classyojimbo_1_1_client_1a005a49077acaddff7fd29ea26fe07687)


**See also**: [ConnectToken](#structyojimbo_1_1_connect_token)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public int numServerAddresses` | The number of server addresses to connect to in [1,MaxServersPerConnect].
`public `[`Address`](#classyojimbo_1_1_address)` serverAddresses` | The array of server addresses that the client can connect to, in order of first to last to try.
`public uint8_t connectTokenData` | The encrypted connect token data.
`public uint8_t connectTokenNonce` | The nonce the connect token was encrypted with.
`public uint8_t clientToServerKey` | The key for client to server encrypted packets.
`public uint8_t serverToClientKey` | The key for server to client encrypted packets.
`public uint64_t connectTokenExpireTimestamp` | The timestamp at which this connect token expires.
`public inline  MatchResponse()` | 

## Members

#### `public int numServerAddresses` 

The number of server addresses to connect to in [1,MaxServersPerConnect].



#### `public `[`Address`](#classyojimbo_1_1_address)` serverAddresses` 

The array of server addresses that the client can connect to, in order of first to last to try.



#### `public uint8_t connectTokenData` 

The encrypted connect token data.



#### `public uint8_t connectTokenNonce` 

The nonce the connect token was encrypted with.



#### `public uint8_t clientToServerKey` 

The key for client to server encrypted packets.



#### `public uint8_t serverToClientKey` 

The key for server to client encrypted packets.



#### `public uint64_t connectTokenExpireTimestamp` 

The timestamp at which this connect token expires.



#### `public inline  MatchResponse()` 





# struct `yojimbo::PacketReadWriteInfo` 


Information passed into low-level functions to read and write packets.



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public bool rawFormat` | If true then packets are written in "raw" format without crc32 (useful for encrypted packets which have packet signature elsewhere).
`public int prefixBytes` | Prefix this number of bytes when reading and writing packets. Used for the variable length sequence number at the start of encrypted packets.
`public uint64_t protocolId` | Protocol id that distinguishes your protocol from other packets sent over UDP.
`public `[`Allocator`](#classyojimbo_1_1_allocator)` * streamAllocator` | This allocator is passed in to the stream and used for dynamic allocations while reading and writing packets.
`public `[`PacketFactory`](#classyojimbo_1_1_packet_factory)` * packetFactory` | [Packet](#classyojimbo_1_1_packet) factory defines the set of packets that can be read. Also called to create packet objects when a packet is read from the network.
`public const uint8_t * allowedPacketTypes` | Array of allowed packet types. One entry per-packet type in [0,numPacketTypes-1] as defined by the packet factory. If a packet type is not allowed then serialization of that packet will fail. Allows the caller to disable certain packet types dynamically.
`public void * context` | Context for packet serialization. Optional. Pass in NULL if not using it. Set on the stream and accessible during packet serialization via [ReadStream::GetContext](#classyojimbo_1_1_base_stream_1acd52d9528cd9afcefd6bdfbab3e4e0d1), [WriteStream::GetContext](#classyojimbo_1_1_base_stream_1acd52d9528cd9afcefd6bdfbab3e4e0d1), etc.
`public void * userContext` | User context for packet serialization. Optional. Pass in NULL if not using it. Set on the stream and accessible during packet serialization via [ReadStream::GetUserContext](#classyojimbo_1_1_base_stream_1a06f4596e6baa6dac537ec364a5cfb4ab), [WriteStream::GetUserContext](#classyojimbo_1_1_base_stream_1a06f4596e6baa6dac537ec364a5cfb4ab), etc.
`public inline  PacketReadWriteInfo()` | 

## Members

#### `public bool rawFormat` 

If true then packets are written in "raw" format without crc32 (useful for encrypted packets which have packet signature elsewhere).



#### `public int prefixBytes` 

Prefix this number of bytes when reading and writing packets. Used for the variable length sequence number at the start of encrypted packets.



#### `public uint64_t protocolId` 

Protocol id that distinguishes your protocol from other packets sent over UDP.



#### `public `[`Allocator`](#classyojimbo_1_1_allocator)` * streamAllocator` 

This allocator is passed in to the stream and used for dynamic allocations while reading and writing packets.



#### `public `[`PacketFactory`](#classyojimbo_1_1_packet_factory)` * packetFactory` 

[Packet](#classyojimbo_1_1_packet) factory defines the set of packets that can be read. Also called to create packet objects when a packet is read from the network.



#### `public const uint8_t * allowedPacketTypes` 

Array of allowed packet types. One entry per-packet type in [0,numPacketTypes-1] as defined by the packet factory. If a packet type is not allowed then serialization of that packet will fail. Allows the caller to disable certain packet types dynamically.



#### `public void * context` 

Context for packet serialization. Optional. Pass in NULL if not using it. Set on the stream and accessible during packet serialization via [ReadStream::GetContext](#classyojimbo_1_1_base_stream_1acd52d9528cd9afcefd6bdfbab3e4e0d1), [WriteStream::GetContext](#classyojimbo_1_1_base_stream_1acd52d9528cd9afcefd6bdfbab3e4e0d1), etc.



#### `public void * userContext` 

User context for packet serialization. Optional. Pass in NULL if not using it. Set on the stream and accessible during packet serialization via [ReadStream::GetUserContext](#classyojimbo_1_1_base_stream_1a06f4596e6baa6dac537ec364a5cfb4ab), [WriteStream::GetUserContext](#classyojimbo_1_1_base_stream_1a06f4596e6baa6dac537ec364a5cfb4ab), etc.



#### `public inline  PacketReadWriteInfo()` 





# struct `yojimbo::PopCount` 


Calculates the population count of an unsigned 32 bit integer at compile time.

Population count is the number of bits in the integer that set to 1.

See "Hacker's Delight" and [http://www.hackersdelight.org/hdcodetxt/popArrayHS.c.txt](http://www.hackersdelight.org/hdcodetxt/popArrayHS.c.txt)


**See also**: [yojimbo::Log2](#structyojimbo_1_1_log2)


**See also**: [yojimbo::BitsRequired](#structyojimbo_1_1_bits_required)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

## Members

# struct `yojimbo::ServerClientData` 


Per-client data stored on the server.

Stores data for connected clients such as their address, globally unique client id, last packet send and receive times used for timeouts and keep-alive packets and so on.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public `[`Address`](#classyojimbo_1_1_address)` address` | The address of this client. Packets are sent and received from the client using this address, therefore only one client with the address may be connected at any time.
`public uint64_t clientId` | Globally unique client id. Only one client with a specific client id may be connected to the server at any time.
`public double connectTime` | The time that the client connected to the server. Used to determine how long the client has been connected.
`public double lastPacketSendTime` | The last time a packet was sent to this client. Used to determine when it's necessary to send keep-alive packets.
`public double lastPacketReceiveTime` | The last time a packet was received from this client. Used for timeouts.
`public bool fullyConnected` | True if this client is 'fully connected'. Fully connected means the client has received a keep-alive packet from the server containing its client index and replied back to the server with a keep-alive packet confirming that it knows its client index.
`public uint64_t clientSalt` | The client salt is a random number rolled on each insecure client connect. It is used to distinguish one client connect session from another, so reconnects are more reliable. See [Client::InsecureConnect](#classyojimbo_1_1_client_1acf884a58a6b98aef48d7063c6181a622) for details.
`public bool insecure` | True if this client connected in insecure mode. This means the client connected via [Client::InsecureConnect](#classyojimbo_1_1_client_1acf884a58a6b98aef48d7063c6181a622) and is sending and receiving packets without encryption. Please use insecure mode only during development, it is not suitable for production use.
`public inline  ServerClientData()` | 

## Members

#### `public `[`Address`](#classyojimbo_1_1_address)` address` 

The address of this client. Packets are sent and received from the client using this address, therefore only one client with the address may be connected at any time.



#### `public uint64_t clientId` 

Globally unique client id. Only one client with a specific client id may be connected to the server at any time.



#### `public double connectTime` 

The time that the client connected to the server. Used to determine how long the client has been connected.



#### `public double lastPacketSendTime` 

The last time a packet was sent to this client. Used to determine when it's necessary to send keep-alive packets.



#### `public double lastPacketReceiveTime` 

The last time a packet was received from this client. Used for timeouts.



#### `public bool fullyConnected` 

True if this client is 'fully connected'. Fully connected means the client has received a keep-alive packet from the server containing its client index and replied back to the server with a keep-alive packet confirming that it knows its client index.



#### `public uint64_t clientSalt` 

The client salt is a random number rolled on each insecure client connect. It is used to distinguish one client connect session from another, so reconnects are more reliable. See [Client::InsecureConnect](#classyojimbo_1_1_client_1acf884a58a6b98aef48d7063c6181a622) for details.



#### `public bool insecure` 

True if this client connected in insecure mode. This means the client connected via [Client::InsecureConnect](#classyojimbo_1_1_client_1acf884a58a6b98aef48d7063c6181a622) and is sending and receiving packets without encryption. Please use insecure mode only during development, it is not suitable for production use.



#### `public inline  ServerClientData()` 





# struct `yojimbo::TransportContext` 


Gives the transport access to resources it needs to read and write packets.

Each transport has a default context set by [Transport::SetContext](#classyojimbo_1_1_transport_1a6d358255024d79ae1538e81363bd4b2f) and cleared by [Transport::ClearContext](#classyojimbo_1_1_transport_1ab5a47eb15876c9e8bed7b52133bc56b8).

Each transport also provides a context mapping that can be used to associate specific addresses with their own context.

The server uses per-client contexts to setup a mapping between connected clients and the resources for that client like allocators, packet factories, message factories and replay protection.

The benefit is that each client is silo'd to their own set of resources and cannot launch an attack to deplete resources shared with other clients.


**See also**: [TransportContext](#structyojimbo_1_1_transport_context)


**See also**: [Transport::SetContext](#classyojimbo_1_1_transport_1a6d358255024d79ae1538e81363bd4b2f)


**See also**: [Transport::ClearContext](#classyojimbo_1_1_transport_1ab5a47eb15876c9e8bed7b52133bc56b8)


**See also**: [Transport::AddContextMapping](#classyojimbo_1_1_transport_1a4d0e96d87d6a7b3890ca077614e7a4e8)


**See also**: [Transport::RemoveContextMapping](#classyojimbo_1_1_transport_1a1faec80840144c109f266b403a186a13)


**See also**: [Transport::ResetContextMappings](#classyojimbo_1_1_transport_1ad7710e1146051a542f083b2894ecee8b)

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public `[`Allocator`](#classyojimbo_1_1_allocator)` * allocator` | The allocator set on the stream. See Stream::GetAllocator. This lets packets to allocate and free memory as they read and write packets.
`public `[`PacketFactory`](#classyojimbo_1_1_packet_factory)` * packetFactory` | The packet factory used to create packets.
`public `[`ReplayProtection`](#classyojimbo_1_1_replay_protection)` * replayProtection` | The replay protection object used to filter out old and duplicate encrypted packets being replayed. Protects against packet replay attacks.
`public struct `[`ConnectionContext`](#structyojimbo_1_1_connection_context)` * connectionContext` | The connection context. This provides information needed by the [ConnectionPacket](#structyojimbo_1_1_connection_packet) to read and write messages and data block fragments.
`public void * userContext` | The user context. This lets the client pass a pointer to data so it is accessible when reading and writing packets. See Stream::GetUserContext.
`public int encryptionIndex` | The encryption index. This is an optimization that avoids repeatedly searching for the encryption mapping by index. If a context is setup for that address, the encryption index is cached in the context.
`public inline  TransportContext()` | 
`public inline  TransportContext(`[`Allocator`](#classyojimbo_1_1_allocator)` & _allocator,`[`PacketFactory`](#classyojimbo_1_1_packet_factory)` & _packetFactory)` | 
`public inline void Clear()` | 

## Members

#### `public `[`Allocator`](#classyojimbo_1_1_allocator)` * allocator` 

The allocator set on the stream. See Stream::GetAllocator. This lets packets to allocate and free memory as they read and write packets.



#### `public `[`PacketFactory`](#classyojimbo_1_1_packet_factory)` * packetFactory` 

The packet factory used to create packets.



#### `public `[`ReplayProtection`](#classyojimbo_1_1_replay_protection)` * replayProtection` 

The replay protection object used to filter out old and duplicate encrypted packets being replayed. Protects against packet replay attacks.



#### `public struct `[`ConnectionContext`](#structyojimbo_1_1_connection_context)` * connectionContext` 

The connection context. This provides information needed by the [ConnectionPacket](#structyojimbo_1_1_connection_packet) to read and write messages and data block fragments.



#### `public void * userContext` 

The user context. This lets the client pass a pointer to data so it is accessible when reading and writing packets. See Stream::GetUserContext.



#### `public int encryptionIndex` 

The encryption index. This is an optimization that avoids repeatedly searching for the encryption mapping by index. If a context is setup for that address, the encryption index is cached in the context.



#### `public inline  TransportContext()` 





#### `public inline  TransportContext(`[`Allocator`](#classyojimbo_1_1_allocator)` & _allocator,`[`PacketFactory`](#classyojimbo_1_1_packet_factory)` & _packetFactory)` 





#### `public inline void Clear()` 





# struct `yojimbo::ChannelPacketData::BlockData` 


Data sent when a channel is sending a block message.

**See also**: [BlockMessage](#classyojimbo_1_1_block_message).

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public `[`BlockMessage`](#classyojimbo_1_1_block_message)` * message` | The message the block is attached to. The message is serialized and included as well as the block data which is split up into fragments.
`public uint8_t * fragmentData` | Pointer to the fragment data being sent in this packet. Blocks are split up into fragments of size [ChannelConfig::fragmentSize](#structyojimbo_1_1_channel_config_1a69907f7813260da7ade25c8c960c779c).
`public uint64_t messageId` | The message id that this block is attached to. Used for ordering. [Message](#classyojimbo_1_1_message) id increases with each packet sent across a channel.
`public uint64_t fragmentId` | The id of the fragment being sent in [0,numFragments-1].
`public uint64_t fragmentSize` | The size of the fragment. Typically this is [ChannelConfig::fragmentSize](#structyojimbo_1_1_channel_config_1a69907f7813260da7ade25c8c960c779c), except for the last fragment, which may be smaller.
`public uint64_t numFragments` | The number of fragments this block is split up into. Lets the receiver know when all fragments have been received.
`public int messageType` | The message type. Used to create the corresponding message object on the receiver side once all fragments are received.

## Members

#### `public `[`BlockMessage`](#classyojimbo_1_1_block_message)` * message` 

The message the block is attached to. The message is serialized and included as well as the block data which is split up into fragments.



#### `public uint8_t * fragmentData` 

Pointer to the fragment data being sent in this packet. Blocks are split up into fragments of size [ChannelConfig::fragmentSize](#structyojimbo_1_1_channel_config_1a69907f7813260da7ade25c8c960c779c).



#### `public uint64_t messageId` 

The message id that this block is attached to. Used for ordering. [Message](#classyojimbo_1_1_message) id increases with each packet sent across a channel.



#### `public uint64_t fragmentId` 

The id of the fragment being sent in [0,numFragments-1].



#### `public uint64_t fragmentSize` 

The size of the fragment. Typically this is [ChannelConfig::fragmentSize](#structyojimbo_1_1_channel_config_1a69907f7813260da7ade25c8c960c779c), except for the last fragment, which may be smaller.



#### `public uint64_t numFragments` 

The number of fragments this block is split up into. Lets the receiver know when all fragments have been received.



#### `public int messageType` 

The message type. Used to create the corresponding message object on the receiver side once all fragments are received.



# struct `yojimbo::ChannelPacketData::MessageData` 


Data sent when a channel is sending regular messages.



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public int numMessages` | The number of messages included in the packet for this channel.
`public `[`Message`](#classyojimbo_1_1_message)` ** messages` | Array of message pointers (dynamically allocated). The messages in this array have references added, so they must be released when the packet containing this channel data is destroyed.

## Members

#### `public int numMessages` 

The number of messages included in the packet for this channel.



#### `public `[`Message`](#classyojimbo_1_1_message)` ** messages` 

Array of message pointers (dynamically allocated). The messages in this array have references added, so they must be released when the packet containing this channel data is destroyed.



# struct `yojimbo::ReliableOrderedChannel::MessageReceiveQueueEntry` 


An entry in the receive queue of the reliable-ordered channel.



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public `[`Message`](#classyojimbo_1_1_message)` * message` | The message pointer. Has at a reference count of at least 1 while in the receive queue. Ownership of the message is passed back to the caller when the message is dequeued.

## Members

#### `public `[`Message`](#classyojimbo_1_1_message)` * message` 

The message pointer. Has at a reference count of at least 1 while in the receive queue. Ownership of the message is passed back to the caller when the message is dequeued.



# struct `yojimbo::ReliableOrderedChannel::MessageSendQueueEntry` 


An entry in the send queue of the reliable-ordered channel.

Messages stay into the send queue until acked. Each message is acked individually, so there can be "holes" in the message send queue.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public `[`Message`](#classyojimbo_1_1_message)` * message` | Pointer to the message. When inserted in the send queue the message has one reference. It is released when the message is acked and removed from the send queue.
`public double timeLastSent` | The time the message was last sent. Used to implement [ChannelConfig::messageResendTime](#structyojimbo_1_1_channel_config_1a4cee28ed7fe1b7df30c715ae9740db67).
`public uint32_t measuredBits` | The number of bits the message takes up in a bit stream.
`public uint32_t block` | 1 if this is a block message. Block messages are treated differently to regular messages when sent over a reliable-ordered channel.

## Members

#### `public `[`Message`](#classyojimbo_1_1_message)` * message` 

Pointer to the message. When inserted in the send queue the message has one reference. It is released when the message is acked and removed from the send queue.



#### `public double timeLastSent` 

The time the message was last sent. Used to implement [ChannelConfig::messageResendTime](#structyojimbo_1_1_channel_config_1a4cee28ed7fe1b7df30c715ae9740db67).



#### `public uint32_t measuredBits` 

The number of bits the message takes up in a bit stream.



#### `public uint32_t block` 

1 if this is a block message. Block messages are treated differently to regular messages when sent over a reliable-ordered channel.



# struct `yojimbo::BaseTransport::PacketEntry` 






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public uint64_t sequence` | The sequence number of the packet. Always 0 if the packet is not encrypted.
`public `[`Address`](#classyojimbo_1_1_address)` address` | The address of the packet. Depending on the queue, this is the address the packet should be sent to, or the address that sent the packet.
`public `[`Packet`](#classyojimbo_1_1_packet)` * packet` | The packet object. While the packet in a queue, it is owned by that queue, and the queue is responsible for destroying that packet, or handing ownership off to somebody else (eg. after ReceivePacket).
`public inline  PacketEntry()` | 

## Members

#### `public uint64_t sequence` 

The sequence number of the packet. Always 0 if the packet is not encrypted.



#### `public `[`Address`](#classyojimbo_1_1_address)` address` 

The address of the packet. Depending on the queue, this is the address the packet should be sent to, or the address that sent the packet.



#### `public `[`Packet`](#classyojimbo_1_1_packet)` * packet` 

The packet object. While the packet in a queue, it is owned by that queue, and the queue is responsible for destroying that packet, or handing ownership off to somebody else (eg. after ReceivePacket).



#### `public inline  PacketEntry()` 





# struct `yojimbo::NetworkSimulator::PacketEntry` 


A packet buffered in the network simulator.



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public `[`Address`](#classyojimbo_1_1_address)` from` | [Address](#classyojimbo_1_1_address) the packet was sent from.
`public `[`Address`](#classyojimbo_1_1_address)` to` | [Address](#classyojimbo_1_1_address) the packet should be sent to.
`public double deliveryTime` | Delivery time for this packet (seconds).
`public uint8_t * packetData` | [Packet](#classyojimbo_1_1_packet) data (owns this pointer).
`public int packetSize` | Size of packet in bytes.
`public inline  PacketEntry()` | 

## Members

#### `public `[`Address`](#classyojimbo_1_1_address)` from` 

[Address](#classyojimbo_1_1_address) the packet was sent from.



#### `public `[`Address`](#classyojimbo_1_1_address)` to` 

[Address](#classyojimbo_1_1_address) the packet should be sent to.



#### `public double deliveryTime` 

Delivery time for this packet (seconds).



#### `public uint8_t * packetData` 

[Packet](#classyojimbo_1_1_packet) data (owns this pointer).



#### `public int packetSize` 

Size of packet in bytes.



#### `public inline  PacketEntry()` 





# struct `yojimbo::ReliableOrderedChannel::ReceiveBlockData` 


Internal state for a block being received across the reliable ordered channel.

Stores the fragments received over the network for the block, and completes once all fragments have been received.

IMPORTANT: Although there can be multiple block messages in the message send and receive queues, only one data block can be in flights over the wire at a time.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public bool active` | True if we are currently receiving a block.
`public int numFragments` | The number of fragments in this block.
`public int numReceivedFragments` | The number of fragments received.
`public uint16_t messageId` | The message id corresponding to the block.
`public int messageType` | [Message](#classyojimbo_1_1_message) type of the block being received.
`public uint32_t blockSize` | Block size in bytes.
`public `[`BitArray`](#classyojimbo_1_1_bit_array)` * receivedFragment` | Has fragment n been received?
`public uint8_t * blockData` | Block data for receive.
`public `[`BlockMessage`](#classyojimbo_1_1_block_message)` * blockMessage` | Block message (sent with fragment 0).
`public inline  ReceiveBlockData(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,int maxBlockSize,int maxFragmentsPerBlock)` | 
`public inline  ~ReceiveBlockData()` | 
`public inline void Reset()` | 

## Members

#### `public bool active` 

True if we are currently receiving a block.



#### `public int numFragments` 

The number of fragments in this block.



#### `public int numReceivedFragments` 

The number of fragments received.



#### `public uint16_t messageId` 

The message id corresponding to the block.



#### `public int messageType` 

[Message](#classyojimbo_1_1_message) type of the block being received.



#### `public uint32_t blockSize` 

Block size in bytes.



#### `public `[`BitArray`](#classyojimbo_1_1_bit_array)` * receivedFragment` 

Has fragment n been received?



#### `public uint8_t * blockData` 

Block data for receive.



#### `public `[`BlockMessage`](#classyojimbo_1_1_block_message)` * blockMessage` 

Block message (sent with fragment 0).



#### `public inline  ReceiveBlockData(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,int maxBlockSize,int maxFragmentsPerBlock)` 





#### `public inline  ~ReceiveBlockData()` 





#### `public inline void Reset()` 





# struct `yojimbo::ReliableOrderedChannel::SendBlockData` 


Internal state for a block being sent across the reliable ordered channel.

Stores the block data and tracks which fragments have been acked. The block send completes when all fragments have been acked.

IMPORTANT: Although there can be multiple block messages in the message send and receive queues, only one data block can be in flights over the wire at a time.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public bool active` | True if we are currently sending a block.
`public int blockSize` | The size of the block (bytes).
`public int numFragments` | Number of fragments in the block being sent.
`public int numAckedFragments` | Number of acked fragments in the block being sent.
`public uint16_t blockMessageId` | The message id the block is attached to.
`public `[`BitArray`](#classyojimbo_1_1_bit_array)` * ackedFragment` | Has fragment n been received?
`public double * fragmentSendTime` | Last time fragment was sent.
`public uint8_t * blockData` | The block data.
`public inline  SendBlockData(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,int maxBlockSize,int maxFragmentsPerBlock)` | 
`public inline  ~SendBlockData()` | 
`public inline void Reset()` | 

## Members

#### `public bool active` 

True if we are currently sending a block.



#### `public int blockSize` 

The size of the block (bytes).



#### `public int numFragments` 

Number of fragments in the block being sent.



#### `public int numAckedFragments` 

Number of acked fragments in the block being sent.



#### `public uint16_t blockMessageId` 

The message id the block is attached to.



#### `public `[`BitArray`](#classyojimbo_1_1_bit_array)` * ackedFragment` 

Has fragment n been received?



#### `public double * fragmentSendTime` 

Last time fragment was sent.



#### `public uint8_t * blockData` 

The block data.



#### `public inline  SendBlockData(`[`Allocator`](#classyojimbo_1_1_allocator)` & allocator,int maxBlockSize,int maxFragmentsPerBlock)` 





#### `public inline  ~SendBlockData()` 





#### `public inline void Reset()` 





# struct `yojimbo::ReliableOrderedChannel::SentPacketEntry` 


Maps packet level acks to messages and fragments for the reliable-ordered channel.



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public double timeSent` | The time the packet was sent. Used to estimate round trip time.
`public uint16_t * messageIds` | Pointer to an array of message ids. Dynamically allocated because the user can configure the maximum number of messages in a packet per-channel with [ChannelConfig::maxMessagesPerPacket](#structyojimbo_1_1_channel_config_1a9d2ddcfa107a7768a42da69419d72b56).
`public uint32_t numMessageIds` | The number of message ids in in the array.
`public uint32_t acked` | 1 if this packet has been acked.
`public uint64_t block` | 1 if this packet contains a fragment of a block message.
`public uint64_t blockMessageId` | The block message id. Valid only if "block" is 1.
`public uint64_t blockFragmentId` | The block fragment id. Valid only if "block" is 1.

## Members

#### `public double timeSent` 

The time the packet was sent. Used to estimate round trip time.



#### `public uint16_t * messageIds` 

Pointer to an array of message ids. Dynamically allocated because the user can configure the maximum number of messages in a packet per-channel with [ChannelConfig::maxMessagesPerPacket](#structyojimbo_1_1_channel_config_1a9d2ddcfa107a7768a42da69419d72b56).



#### `public uint32_t numMessageIds` 

The number of message ids in in the array.



#### `public uint32_t acked` 

1 if this packet has been acked.



#### `public uint64_t block` 

1 if this packet contains a fragment of a block message.



#### `public uint64_t blockMessageId` 

The block message id. Valid only if "block" is 1.



#### `public uint64_t blockFragmentId` 

The block fragment id. Valid only if "block" is 1.



