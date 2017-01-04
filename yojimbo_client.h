/*
    Yojimbo Client/Server Network Protocol Library.
    
    Copyright © 2016, The Network Protocol Company, Inc.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
           in the documentation and/or other materials provided with the distribution.

        3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived 
           from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef YOJIMBO_CLIENT_H
#define YOJIMBO_CLIENT_H

#include "yojimbo_config.h"
#include "yojimbo_packet.h"
#include "yojimbo_allocator.h"
#include "yojimbo_transport.h"
#include "yojimbo_encryption.h"
#include "yojimbo_connection.h"
#include "yojimbo_packet_processor.h"
#include "yojimbo_client_server_packets.h"
#include "yojimbo_tokens.h"

namespace yojimbo
{
    /**
        The set of state in the client state machine.

        A client starts in the default state CLIENT_STATE_DISCONNECTED (0). 

        Positive states lower than CLIENT_STATE_CONNECTED represent states where the client is negotiating connection to a server following a call to Client::Connect or Client::InsecureConnect. While in these states, Client::IsConnecting returns true.
        
        When a client state connects to a server it transitions to CLIENT_STATE_CONNECTED. While in this state, Client::IsConnected returns true and Client::IsConnecting returns false.

        Negative states correspond to client-side errors. There is one client state per error condition that the client can experience. If one of these errors occur, the client disconnects and transitions to that error state. When in one of these error state, Client::IsConnected returns false and Client::ConnectionFailed returns true.
        
        @see Client::GetClientState
        @see Client::IsConnecting
        @see Client::IsConnected
        @see Client::ConnectedFailed
     */

    enum ClientState
    {
#if !YOJIMBO_SECURE_MODE
        CLIENT_STATE_INSECURE_CONNECT_TIMEOUT = -9,                             ///< The client tried to connect to a server via Client::InsecureConnect, but the connection timed out.
#endif // #if !YOJIMBO_SECURE_MODE
        CLIENT_STATE_PACKET_FACTORY_ERROR = -8,                                 ///< The client disconnected because the packet factory got into an error state.
        CLIENT_STATE_MESSAGE_FACTORY_ERROR = -7,                                ///< The client disconnected because the message factory got into an error state.
        CLIENT_STATE_ALLOCATOR_ERROR = -6,                                      ///< The client disconnected because the allocator got into an error state. This happens when the allocator runs out of memory.
        CLIENT_STATE_CONNECTION_ERROR = -5,                                     ///< The client disconnected because the connection got into an error state. This happens when something goes wrong when reading and writing messages.
        CLIENT_STATE_CONNECTION_REQUEST_TIMEOUT = -4,                           ///< The client timed out while sending connection request packets to the server.
        CLIENT_STATE_CHALLENGE_RESPONSE_TIMEOUT = -3,                           ///< The client timed out while sending challenge response packets to the server.
        CLIENT_STATE_CONNECTION_TIMEOUT = -2,                                   ///< The client timed out after successfully connecting to the server.
        CLIENT_STATE_CONNECTION_DENIED = -1,                                    ///< The server denied the connection request. This happens when the server is full.
        CLIENT_STATE_DISCONNECTED = 0,                                          ///< The client is disconnected. This is the default state. Also, this is the state the client transitions to if it cleanly disconnects from the server (client side), or via disconnect packet sent from the server.
#if !YOJIMBO_SECURE_MODE
        CLIENT_STATE_SENDING_INSECURE_CONNECT,                                  ///< The client is sending insecure connect packets to the server. This state immediately follows Client::InsecureConnect and transitions directly to CLIENT_STATE_CONNECTED when an insecure connection is established.
#endif // #if !YOJIMBO_SECURE_MODE
        CLIENT_STATE_SENDING_CONNECTION_REQUEST,                                ///< The client is sending connection request packets to the server. This state immediately follows Client::Connect. It transitions to CLIENT_STATE_SENDING_CHALLENGE_RESPONSE and then to CLIENT_STATE_CONNECTED.
        CLIENT_STATE_SENDING_CHALLENGE_RESPONSE,                                ///< The client is sending challenge response packets to the server. Challenge/response during connect filters out clients trying to connect with a spoofed packet source address.
        CLIENT_STATE_CONNECTED                                                  ///< The client is connected to the server.
    };

    /** 
        Helper function to convert client state enum value to a string. Useful for logging and debugging.

        @param clientState The client state to convert to a string.
        
        @returns The client state in a user friendly string, eg. "connected".
     */

    inline const char * GetClientStateName( ClientState clientState )
    {
        switch ( clientState )
        {
#if !YOJIMBO_SECURE_MODE
            case CLIENT_STATE_INSECURE_CONNECT_TIMEOUT:         return "insecure connect timeout";
#endif // #if !YOJIMBO_SECURE_MODE
            case CLIENT_STATE_PACKET_FACTORY_ERROR:             return "packet factory error";
            case CLIENT_STATE_MESSAGE_FACTORY_ERROR:            return "message factory error";
            case CLIENT_STATE_ALLOCATOR_ERROR:                  return "allocator error";
            case CLIENT_STATE_CONNECTION_REQUEST_TIMEOUT:       return "connection request timeout";
            case CLIENT_STATE_CHALLENGE_RESPONSE_TIMEOUT:       return "challenge response timeout";
            case CLIENT_STATE_CONNECTION_TIMEOUT:               return "connection timeout";
            case CLIENT_STATE_CONNECTION_ERROR:                 return "connection error";
            case CLIENT_STATE_CONNECTION_DENIED:                return "connection denied";
            case CLIENT_STATE_DISCONNECTED:                     return "disconnected";
#if !YOJIMBO_SECURE_MODE
            case CLIENT_STATE_SENDING_INSECURE_CONNECT:         return "sending insecure connect";
#endif // #if !YOJIMBO_SECURE_MODE
            case CLIENT_STATE_SENDING_CONNECTION_REQUEST:       return "sending connection request";
            case CLIENT_STATE_SENDING_CHALLENGE_RESPONSE:       return "sending challenge response";
            case CLIENT_STATE_CONNECTED:                        return "connected";
            default:
                assert( false );
                return "???";
        }
    }

    /// Client counters. These are useful for debugging and telemetry.

    enum ClientCounters
    {
        CLIENT_COUNTER_TODO,                                                    ///< TODO: Add some client counters.

        NUM_CLIENT_COUNTERS                                                     
    };

    /** 
        A client that connects to a server.

        This class is designed to be inherited from to create your own client class.
     */

    class Client : public ConnectionListener
    {
    public:

        /**
            The client constructor.

            @param allocator The allocator for all memory used by the client.
            @param transport The transport for sending and receiving packets.
            @param config The client/server configuration.
            @param time The current time in seconds. See Client::AdvanceTime
         */

        explicit Client( Allocator & allocator, Transport & transport, const ClientServerConfig & config, double time );

        /**
            The client destructor.

            IMPORTANT: Please call Client::Disconnect before destroying the client. This is necessary because Disconnect is virtual and calling virtual methods from destructors does not give the expected behavior when you override that method.
         */

        virtual ~Client();

        /**
            Set the user context.

            The user context is set on the stream when packets and read and written. It lets you pass in a pointer to some structure that you want to have available when reading and writing packets.

            Typical use case is to pass in an array of min/max ranges for values determined by some data that is loaded from a toolchain vs. being known at compile time. 

            If you do use a user context, please make sure the data that contributes to the user context is checksummed and included in the protocol id, so clients and servers with incompatible data can't connect to each other.

            @see Stream::GetUserContext
         */

        void SetUserContext( void * context );

#if !YOJIMBO_SECURE_MODE

        /** 
            Connect to a server (insecure).

            IMPORTANT: Insecure connections are not encrypted and do not provide authentication. 

            They are provided for convienence in development only, and should not be used in production code!

            You can completely disable insecure connections in your retail build by defining YOJIMBO_SECURE_MODE 1

            @param clientId A globally unique client id used to identify clients when your server talks to your web backend. If you don't have a concept of client id yet, roll a random 64bit integer.
            @param serverAddress The address of the server to connect to.
         */

        void InsecureConnect( uint64_t clientId, const Address & serverAddress );
        
        /** 
            Connect to a list of servers (insecure).

            The client tries to connect to each server in the list, in turn, until one of the servers is connected to, or it reaches the end of the server address list.

            IMPORTANT: Insecure connections are not encrypted and do not provide authentication. 

            They are provided for convienence in development only, and should not be used in production code!

            You can completely disable insecure connections in your retail build by defining YOJIMBO_SECURE_MODE 1

            @param clientId A globally unique client id used to identify clients when your server talks to your web backend. If you don't have a concept of client id yet, roll a random 64bit integer.
            @param serverAddresses The list of server addresses to connect to, in order of first to last.
            @param numServerAddresses Number of server addresses in [1,yojimbo::MaxServersPerConnect].
         */

        void InsecureConnect( uint64_t clientId, const Address serverAddresses[], int numServerAddresses );

#endif // #if !YOJIMBO_SECURE_MODE

        /** 
            Connect to a server (secure).

            This function takes a connect token generated by matcher.go and passes it to the server to establish a secure connection.

            Secure connections are encrypted and authenticated. If the server runs in secure mode, it will only accept connections from clients with a connect token, thus stopping unauthenticated clients from connecting to your server.

            @param clientId A globally unique client id used to identify clients when your server talks to your web backend. If you don't have a concept of client id yet, roll a random 64bit integer.
            @param serverAddress The address of the server to connect to.
            @param connectTokenData Pointer to the connect token data from the matcher.
            @param connectTokenNonce Pointer to the connect token nonce from the matcher.
            @param clientToServerKey The encryption key for client to server packets.
            @param serverToClientKey The encryption key for server to client packets.
            @param connectTokenExpireTimestamp The timestamp for when the connect token expires. Used by the server to quickly reject stale connect tokens without decrypting them.
         */

        void Connect( uint64_t clientId,
                      const Address & serverAddress, 
                      const uint8_t * connectTokenData, 
                      const uint8_t * connectTokenNonce,
                      const uint8_t * clientToServerKey,
                      const uint8_t * serverToClientKey,
                      uint64_t connectTokenExpireTimestamp );

        /** 
            Connect to a list of servers (secure).

            The client tries to connect to each server in the list, in turn, until one of the servers is connected to, or it reaches the end of the server address list.

            This is designed for use with the matcher, which can return a list of up to yojimbo::MaxServersPerConnect with the connect token, to work around race conditions where the server sends multiple clients to fill the same slot on a server.

            Secure connections are encrypted and authenticated. If the server runs in secure mode, it will only accept connections from clients with a connect token, thus stopping unauthenticated from connecting to your server.

            @param clientId A globally unique client id used to identify clients when your server talks to your web backend. If you don't have a concept of client id yet, roll a random 64bit integer.
            @param serverAddresses The list of server addresses to connect to, in order of first to last.
            @param numServerAddresses Number of server addresses in [1,yojimbo::MaxServersPerConnect].
            @param connectTokenData Pointer to the connect token data from the matcher.
            @param connectTokenNonce Pointer to the connect token nonce from the matcher.
            @param clientToServerKey The encryption key for client to server packets.
            @param serverToClientKey The encryption key for server to client packets.
            @param connectTokenExpireTimestamp The timestamp for when the connect token expires. Used by the server to quickly reject stale connect tokens without decrypting them.
         */

        void Connect( uint64_t clientId, 
                      const Address serverAddresses[], int numServerAddresses,
                      const uint8_t * connectTokenData, 
                      const uint8_t * connectTokenNonce,
                      const uint8_t * clientToServerKey,
                      const uint8_t * serverToClientKey,
                      uint64_t connectTokenExpireTimestamp );

        /**
            Disconnect from the server.

            This function is safe to call if not currently connected. In that case it will do nothing.

            @param clientState The client state to transition to on disconnect. By default CLIENT_STATE_DISCONNECTED, but you may also specify one of the negative client state values for error states.
            @param sendDisconnectPacket If true then disconnect packets will be sent immediately to the server to notify it that the client disconnected. This makes the server client slot get cleaned up and recycled faster for other clients to connect, vs. timing out in 5-10 seconds. The only situation where this should be false is if the client is disconnecting because it timed out from the server.
         */

        void Disconnect( ClientState clientState = CLIENT_STATE_DISCONNECTED, bool sendDisconnectPacket = true );

        /**
            Send packets to server.
         
            This function drives the sending of packets to the server such as connection request, challenge response, keep-alive packet and connection packets to transmit messages from client to server.

            Packets sent from this function are queued for sending on the transport, and are not actually serialized and sent to the network until you call Transport::WritePackets.
         */

        void SendPackets();

        /**
            Receive packets sent from the server.

            This function receives and processes packets from the receive queue of the Transport. To minimize the latency of packet processing, make sure you call Transport::ReadPackets shortly before calling this function.

            @see ProcessPackets
         */

        void ReceivePackets();

        /**
            Check for timeouts.

            Compares the last time a packet was received from the server vs. the current time.

            If no packet has been received within the timeout period, the client is disconnected and set to an error state. See ClientState for details.

            @see Client::AdvanceTime
            @see ClientServerConfig::connectionTimeOut
            @see ClientServerConfig::connectionNegotiationTimeOut
         */

        void CheckForTimeOut();

        /**
            Advance client time.

            Call this at the end of each frame to advance the client time forward. 

            IMPORTANT: Please use a double for your time value so it maintains sufficient accuracy as time increases.
         */

        void AdvanceTime( double time );

        /**
            Is the client connecting to a server?

            This is true while the client is negotiation connection with a server. This is the period after a call to Client::Connect or Client::InsecureConnect, but before the client establishes a connection, or goes into an error state because it couldn't connect.

            @returns true if the client is currently connecting to, but is not yet connected to a server.
         */

        bool IsConnecting() const;

        /**
            Is the client connected to a server?

            This is true once a client successfully finishes connection negotiation, and connects to a server. It is false while connecting to a server.

            Corresponds to the client being in CLIENT_STATE_CONNECTED.

            @returns true if the client is connected to a server.
         */

        bool IsConnected() const;

        /**
            Is the client in a disconnected state?

            A disconnected state is CLIENT_STATE_DISCONNECTED, or any of the negative client error state values. Effectively, true if client state is <= 0.

            @returns true if the client is in a disconnected state.
         */

        bool IsDisconnected() const;

        /**
            Is the client in an error state?

            When the client disconnects because of a client-side error, it disconnects and sets one of the negative client state values. Effectively, true if client state < 0.

            @returns true if the client disconnected to an error state.
         */

        bool ConnectionFailed() const;

        /**
            Get the current client state.

            The client state machine is used to negotiate connection with the server, and handle error states. Each state corresponds to an entry in the ClientState enum.

            @see yojimbo::GetClientStateName
         */

        ClientState GetClientState() const;

        /**
            Gets the current client time.

            @see Client::AdvanceTime
         */

        double GetTime() const;

        /**
            Get the client id (globally unique).

            This corresponds to the client id parameter passed into the last call to Client::Connect or Client::InsecureConnect

            @returns The globally unique client id for this client.
         */

        uint64_t GetClientId() const;

        /**
            Get the client index.

            The client index is the slot number that the client is occupying on the server. 

            @returns The client index in [0,maxClients-1], where maxClients is the number of client slots allocated on the server in Server::Start.
         */

        int GetClientIndex() const;

        /**
            Get a counter value.

            Counters are used to track event and actions performed by the client. They are useful for debugging, testing and telemetry.

            @returns The counter value. See yojimbo::ClientCounters for the set of client counters.
         */

        uint64_t GetCounter( int index ) const;

        /**
            Create a message of the specified type.

            The message created by this function is typically passed to Client::SendMsg. In this case, the send message function takes ownership of the message pointer and will release it for you.

            If you are using the message in some other way, you are responsible for manually releasing it via Client::ReleaseMsg.

            @param type The message type. The set of message types depends on the message factory set on the client.

            @returns A pointer to the message created, or NULL if no message could be created.

            @see MessageFactory
         */

        Message * CreateMsg( int type );

        /** 
            Check if there is room in the channel send queue to send one message.

            This function is useful in soak tests and unit tests where I want to send messages as quickly as possible, but don't want to overflow the send queue.

            You don't need to call this function manually each time you call Client::SendMsg. It's already asserted on in debug build and in release if it returns false it sets a runtime error that disconnects the client.

            @param channelId The id of the channel in [0,numChannels-1].
            
            @returns True if the channel has room for one more message to be added to its send queue. False otherwise.
         */

        bool CanSendMsg( int channelId = 0 );

        /**
            Queue a message to be sent to the server.

            Adds a message to the send queue of the specified channel. 

            The reliability and ordering guarantees of how the message will be received on the other side are determined by the configuration of the channel.
    
            IMPORTANT: This function takes ownership of the message and ensures that the message is released when it finished being sent. This lets you create a message with Client::CreateMsg and pass it directly into this function. You don't need to manually release the message.

            @param message The message to be sent. It must be allocated from the message factory set on this client.
            @param channelId The id of the channel to send the message across in [0,numChannels-1].

            @see ChannelConfig
            @see ClientServerConfig
         */

        void SendMsg( Message * message, int channelId = 0 );

        /** 
            Poll this method to receive messages from the server.

            Typical usage is to iterate across the set of channels and poll this to receive messages until it returns NULL.

            IMPORTANT: The message returned by this function has one reference. You are responsible for releasing this message via Client::ReleaseMsg.

            @param channelId The id of the channel to try to receive a message from.

            @returns A pointer to the received message, NULL if there are no messages to receive.
         */

        Message * ReceiveMsg( int channelId = 0 );

        /**
            Release a message returned by Client::ReceiveMsg.

            This is a convenience function. It is equivalent to calling MessageFactory::Release on the message factory set on this client (see Client::GetMsgFactory).

            @param message The message to release. Must be non-NULL.

            @see Client::ReceiveMsg
         */

        void ReleaseMsg( Message * message );

        /**
            Get the message factory used by the client.

            The message factory determines the set of messages exchanged between the client and server.

            @returns The message factory.

            @see YOJIMBO_CLIENT_MESSAGE_FACTORY
         */

        MessageFactory & GetMsgFactory();

        /**
            Get the allocator used for client allocations.

            This memory is used for packet, message allocations and stream allocations while the client is connecting/connected to the server.

            The amount of memory backing this allocator is specified by ClientServerConfig::clientMemory.

            @returns The allocator client allocator.
         */

        Allocator & GetClientAllocator();

    protected:

        /**
            Helper function to create a packet by type.

            Just a shortcut to PacketFactory::CreatePacket for convenience.

            @param type The type of packet to create.

            @returns The packet object that was created. NULL if a packet could be created. You *must* check this. It *will* happen when the packet factory runs out of memory to allocate packets!
         */

        Packet * CreatePacket( int type );

        /**
            Override this method to get a callback when the client starts to connect to a server.

            @param address The address of the server that is being connected to.
         */

        virtual void OnConnect( const Address & address ) { (void) address; }

        /**
            Override this method to get a callback when the client state changes.

            The previous and current state are guaranteed to be different, otherwise this callback is not called.

            IMPORTANT: This callback is called at the end of the connection start process, so all client server data is valid, such as the server address and client state at the point this callback is executed.

            @param previousState The previous client state.
            @param currentState The current client state that was just transitioned to.
            @see yojimbo::GetClientStateName
         */

        virtual void OnClientStateChange( ClientState previousState, ClientState currentState ) { (void) previousState; (void) currentState; }

        /**
            Override this method to get a callback when the client disconnects from the server.

            IMPORTANT: This callback is executed before the client disconnect, so all details and states regarding the client connection are still valid (eg. GetServerAddress) and so on.

            If you want more detail about why the client is disconnecting, override Client::OnClientStateChange and follow state transitions that way.
         */

        virtual void OnDisconnect() {}

        /**
            Override this method to get a callback when the client sends a packet.

            @param packetType The type of packet being sent, according to the client packet factory. See PacketFactory.
            @param to The address the packet is being sent to.
            @param immediate True if the packet is to be flushed to the network and sent immediately.

            @see Client::SendPackets
         */

        virtual void OnPacketSent( int packetType, const Address & to, bool immediate ) { (void) packetType; (void) to; (void) immediate; }

        /**
            Override this method to get a callback when the client receives a packet.

            @param packetType The type of packet being sent, according to the client packet factory. See PacketFactory.
            @param from The address of the machine that sent the packet.

            @see Client::ReceivePackets
         */

        virtual void OnPacketReceived( int packetType, const Address & from ) { (void) packetType; (void) from; }

        /**
            Override this method to get a callback when a connection packet is generated and sent to the server.

            Connection packets are carrier packets that transmit messages between the client and server. They are only generated if you enabled messages in the ClientServerConfig (true by default).

            @param connection The connection that the packet belongs to. There is just one connection object on the client-side.
            @param sequence The sequence number of the connection packet being sent.

            @see ClientServerConfig::enableMessages
         */

        virtual void OnConnectionPacketGenerated( Connection * connection, uint16_t sequence ) { (void) connection; (void) sequence; }

        /**
            Override this method to get a callback when a connection packet is acked by the client (eg. the client notified the server that packet was received).

            Connection packets are carrier packets that transmit messages between the client and server. They are automatically generated when messages are enabled in ClientServerConfig (true by default).

            @param connection The connection that the packet belongs to. There is just one connection object on the client-side.
            @param sequence The packet sequence number of the connection packet that was acked by the server.

            @see ClientServerConfig::enableMessages
         */

        virtual void OnConnectionPacketAcked( Connection * connection, uint16_t sequence ) { (void) connection; (void) sequence; }

        /**
            Override this method to get a callback when a connection packet is received from the server.

            Connection packets are carrier packets that transmit messages between the client and server. They are automatically generated when messages are enabled in ClientServerConfig (true by default).

            @param connection The connection that the packet belongs to. There is just one connection object on the client-side.
            @param sequence The sequence number of the connection packet that was received.

            @see ClientServerConfig::enableMessages
         */

        virtual void OnConnectionPacketReceived( Connection * connection, uint16_t sequence ) { (void) connection; (void) sequence; }

        /**
            Override this method to get a callback when a block fragment is received from the server.

            This callback lets you implement a progress bar for large block transmissions.

            @param connection The connection that the block fragment belongs to. There is just one connection object on the client-side.
            @param channelId The channel the block is being sent over.
            @param messageId The message id the block is attached to.
            @param fragmentId The fragment id that is being processed. Fragment ids are in the range [0,numFragments-1].
            @param fragmentBytes The size of the fragment in bytes.
            @param numFragmentsReceived The number of fragments received for this block so far (including this one).
            @param numFragmentsInBlock The total number of fragments in this block. The block receive completes when all fragments are received.

            @see BlockMessage::AttachBlock
         */

        virtual void OnConnectionFragmentReceived( Connection * connection, int channelId, uint16_t messageId, uint16_t fragmentId, int fragmentBytes, int numFragmentsReceived, int numFragmentsInBlock ) { (void) connection; (void) channelId; (void) messageId; (void) fragmentId; (void) fragmentBytes; (void) numFragmentsReceived; (void) numFragmentsInBlock; }

        /** 
            Override this method to process user packets sent from the server.

            User packets let you extend the yojimbo by adding your own packet types to be exchanged between client and server. See PacketFactory.

            Most users won't need to create custom packet types, and will extend the protocol by defining their own message types instead. See MessageFactory.

            @param packet The user packet received from the server.

            @returns Return true if the user packet was processed successfully. Returning false if the packet could not be processed, or if is of a type you don't expect. This ensures that unexpected packet types don't keep the connection alive when it should time out. out.
         */

        virtual bool ProcessUserPacket( Packet * packet ) { (void) packet; return false; }

    protected:

        virtual void InitializeConnection( uint64_t clientId );

        virtual void ShutdownConnection();

        virtual void SetEncryptedPacketTypes();

        virtual PacketFactory * CreatePacketFactory( Allocator & allocator );

        virtual MessageFactory * CreateMessageFactory( Allocator & allocator );

        virtual void SetClientState( ClientState clientState );

        virtual void ResetConnectionData( ClientState clientState = CLIENT_STATE_DISCONNECTED );

        virtual void ResetBeforeNextConnect();

        virtual bool ConnectToNextServer();

#if !YOJIMBO_SECURE_MODE
        virtual void InternalInsecureConnect( const Address & serverAddress );
#endif // #if !YOJIMBO_SECURE_MODE

        virtual void InternalSecureConnect( const Address & serverAddress );

        virtual void SendPacketToServer( Packet * packet );

    private:

        void SendPacketToServer_Internal( Packet * packet, bool immediate = false );

    protected:

        void ProcessConnectionDenied( const ConnectionDeniedPacket & packet, const Address & address );

        void ProcessChallenge( const ChallengePacket & packet, const Address & address );

        void ProcessKeepAlive( const KeepAlivePacket & packet, const Address & address );

        void ProcessDisconnect( const DisconnectPacket & packet, const Address & address );

        void ProcessConnectionPacket( ConnectionPacket & packet, const Address & address );

        void ProcessPacket( Packet * packet, const Address & address, uint64_t sequence );

        bool IsPendingConnect();

        void CompletePendingConnect( int clientIndex );

    protected:

        void Defaults();

        virtual void CreateAllocators();

        virtual void DestroyAllocators();

        virtual Allocator * CreateAllocator( Allocator & allocator, void * memory, size_t bytes );

        ClientServerConfig m_config;                                        ///< The client/server configuration.

        Allocator * m_allocator;                                            ///< The allocator passed in to the client on creation.

        void * m_userContext;                                               ///< The user context. Lets the user pass information to packet serialize functions.

        uint8_t * m_clientMemory;                                           ///< Memory backing the client allocator. Allocated from m_allocator.

        Allocator * m_clientAllocator;                                      ///< The client allocator. Everything allocated between Client::Connect and Client::Disconnect is allocated and freed via this allocator.

        PacketFactory * m_packetFactory;                                    ///< Packet factory for creating and destroying messages. Created via Client::CreatePacketFactory.

        MessageFactory * m_messageFactory;                                  ///< Message factory for creating and destroying messages. Created via Client::CreateMessageFactory. Optional.

        ReplayProtection * m_replayProtection;                              ///< Replay protection discards old and duplicate packets. Protects against packet replay attacks.

        bool m_allocateConnection;                                          ///< True if we should allocate a connection object. This is true when ClientServerConfig::enableMessages is true.

        Connection * m_connection;                                          ///< The connection object for exchanging messages with the server. Optional.

        uint64_t m_clientId;                                                ///< The globally unique client id (set on each call to connect).

        int m_clientIndex;                                                  ///< The client slot index on the server [0,maxClients-1]. -1 if not connected.

        TransportContext m_transportContext;                                ///< Transport context for reading and writing packets.

        ConnectionContext m_connectionContext;                              ///< Connection context for serializing connection packets.

        ClientState m_clientState;                                          ///< The current client state.

        uint64_t m_connectTokenExpireTimestamp;                             ///< Expire timestamp for connect token used in secure connect. This is used as the additional data in the connect token AEAD, so we can quickly reject stale connect tokens without decrypting them.

        int m_serverAddressIndex;                                           ///< Current index in the server address array. This is the server we are currently connecting to.

        int m_numServerAddresses;                                           ///< Number of server addresses in the array.

        Address m_serverAddresses[MaxServersPerConnect];                    ///< List of server addresses we are connecting to.

        Address m_serverAddress;                                            ///< The current server address we are connecting/connected to.

        double m_lastPacketSendTime;                                        ///< The last time we sent a packet to the server.

        double m_lastPacketReceiveTime;                                     ///< The last time we received a packet from the server.

        Transport * m_transport;                                            ///< The transport for sending and receiving packets.

        bool m_shouldDisconnect;                                            ///< Set to true when we receive a disconnect packet from the server so we can defer disconnection until the update.

        ClientState m_shouldDisconnectState;                                ///< The client state to transition to on disconnect via m_shouldDisconnect

        double m_time;                                                      ///< The current client time. See Client::AdvanceTime

#if !YOJIMBO_SECURE_MODE
        uint64_t m_clientSalt;                                              ///< The client salt used for insecure connect. This distinguishes the current insecure client session from a previous one from the same IP:port combination, making insecure reconnects much more robust.
#endif // #if !YOJIMBO_SECURE_MODE

        uint64_t m_sequence;                                                ///< Sequence # for packets sent from client to server.

        uint64_t m_counters[NUM_CLIENT_COUNTERS];                           ///< Counters to aid with debugging and telemetry.

        uint8_t m_connectTokenData[ConnectTokenBytes];                      ///< Encrypted connect token data for the connection request packet.

        uint8_t m_connectTokenNonce[NonceBytes];                            ///< Nonce to send to server so it can decrypt the connect token.

        uint8_t m_challengeTokenData[ChallengeTokenBytes];                  ///< Encrypted challenge token data for challenge response packet

        uint8_t m_challengeTokenNonce[NonceBytes];                          ///< Nonce to send to server so it can decrypt the challenge token.

        uint8_t m_clientToServerKey[KeyBytes];                              ///< Client to server packet encryption key.

        uint8_t m_serverToClientKey[KeyBytes];                              ///< Server to client packet encryption key.

    private:

        Client( const Client & other );
        
        const Client & operator = ( const Client & other );
    };
}

#define YOJIMBO_CLIENT_ALLOCATOR( allocator_class )                                                     \
    Allocator * CreateAllocator( Allocator & allocator, void * memory, size_t bytes )                   \
    {                                                                                                   \
        return YOJIMBO_NEW( allocator, allocator_class, memory, bytes );                                \
    }

#define YOJIMBO_CLIENT_PACKET_FACTORY( packet_factory_class )                                           \
    PacketFactory * CreatePacketFactory( Allocator & allocator )                                        \
    {                                                                                                   \
        return YOJIMBO_NEW( allocator, packet_factory_class, allocator );                               \
    }

#define YOJIMBO_CLIENT_MESSAGE_FACTORY( message_factory_class )                                         \
    MessageFactory * CreateMessageFactory( Allocator & allocator )                                      \
    {                                                                                                   \
        return YOJIMBO_NEW( allocator, message_factory_class, allocator );                              \
    }

#endif // #ifndef YOJIMBO_CLIENT_H
