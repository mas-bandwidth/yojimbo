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

#include "yojimbo_config.h"
#include "yojimbo_tokens.h"
#include "yojimbo_encryption.h"

#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;    

namespace yojimbo
{
    bool ConnectToken::operator == ( const ConnectToken & other ) const
    {
        if ( protocolId != other.protocolId )
            return false;
        
        if ( clientId != other.clientId )
            return false;
            
        if ( expireTimestamp != other.expireTimestamp )
            return false;
            
        if ( numServerAddresses != other.numServerAddresses )
            return false;
            
        for ( int i = 0; i < numServerAddresses; ++i )
        {
            if ( serverAddresses[i] != other.serverAddresses[i] )
                return false;
        }

        if ( memcmp( clientToServerKey, other.clientToServerKey, KeyBytes ) != 0 )
            return false;

        if ( memcmp( serverToClientKey, other.serverToClientKey, KeyBytes ) != 0 )
            return false;

        return true;
    }

    bool ConnectToken::operator != ( const ConnectToken & other ) const
    {
        return ! ( (*this) == other );
    }

    void GenerateConnectToken( ConnectToken & token, uint64_t clientId, int numServerAddresses, const Address * serverAddresses, uint64_t protocolId, int expireSeconds )
    {
        uint64_t timestamp = (uint64_t) time( NULL );
        
        token.protocolId = protocolId;
        token.clientId = clientId;
        token.expireTimestamp = timestamp + expireSeconds;
        
        assert( numServerAddresses > 0 );
        assert( numServerAddresses <= MaxServersPerConnect );
        token.numServerAddresses = numServerAddresses;
        for ( int i = 0; i < numServerAddresses; ++i )
            token.serverAddresses[i] = serverAddresses[i];

        GenerateKey( token.clientToServerKey );    

        GenerateKey( token.serverToClientKey );
    }

    bool EncryptConnectToken( const ConnectToken & token, uint8_t * encryptedMessage, const uint8_t * nonce, const uint8_t * key )
    {
        char message[ConnectTokenBytes-MacBytes];
        memset( message, 0, ConnectTokenBytes - MacBytes );
        if ( !WriteConnectTokenToJSON( token, message, ConnectTokenBytes - MacBytes ) )
            return false;

        uint64_t encryptedLength;

        uint64_t expireTimestampNetworkOrder = host_to_network( token.expireTimestamp );        // network order is little endian

        if ( !Encrypt_AEAD( (const uint8_t*)message, ConnectTokenBytes - MacBytes, encryptedMessage, encryptedLength, (const uint8_t*) &expireTimestampNetworkOrder, 8, nonce, key ) )
            return false;

        assert( encryptedLength == ConnectTokenBytes );

        return true;
    }

    bool DecryptConnectToken( const uint8_t * encryptedMessage, ConnectToken & decryptedToken, const uint8_t * nonce, const uint8_t * key, uint64_t expireTimestamp )
    {
        const int encryptedMessageLength = ConnectTokenBytes;

        uint64_t decryptedMessageLength;
        uint8_t decryptedMessage[ConnectTokenBytes];

        uint64_t expireTimestampNetworkOrder = host_to_network( expireTimestamp );              // network order is little endian

        if ( !Decrypt_AEAD( encryptedMessage, encryptedMessageLength, decryptedMessage, decryptedMessageLength, (const uint8_t*) &expireTimestampNetworkOrder, 8, nonce, key ) )
            return false;

        assert( decryptedMessageLength == ConnectTokenBytes - MacBytes );

        return ReadConnectTokenFromJSON( (const char*) decryptedMessage, decryptedToken );
    }

    static void insert_number_as_string( Writer<StringBuffer> & writer, const char * key, uint64_t number )
    {
        char buffer[256];
        sprintf( buffer, "%" PRId64, number );
        writer.Key( key ); writer.String( buffer );
    }

    static void insert_data_as_base64_string( Writer<StringBuffer> & writer, const char * key, const uint8_t * data, int data_length )
    {
        char * buffer = (char*) alloca( data_length * 2 );
        base64_encode_data( data, data_length, buffer, data_length * 2 );
        writer.Key( key ); writer.String( buffer );
    }

    bool WriteConnectTokenToJSON( const ConnectToken & connectToken, char * output, int outputSize )
    {
        StringBuffer s;
        Writer<StringBuffer> writer(s);
    
        writer.StartObject();
    
        insert_number_as_string( writer, "protocolId", connectToken.protocolId );

        insert_number_as_string( writer, "clientId", connectToken.clientId );

        insert_number_as_string( writer, "expireTimestamp", connectToken.expireTimestamp );

        insert_number_as_string( writer, "numServerAddresses", connectToken.numServerAddresses );

        writer.Key( "serverAddresses" );

        writer.StartArray();

        for ( int i = 0; i < connectToken.numServerAddresses; ++i )
        {
            char serverAddress[MaxAddressLength];
            assert( connectToken.serverAddresses[i].IsValid() );
            connectToken.serverAddresses[i].ToString( serverAddress, MaxAddressLength );

            char serverAddressBase64[MaxAddressLength*2];
            base64_encode_string( serverAddress, serverAddressBase64, sizeof( serverAddressBase64 ) );

            writer.String( serverAddressBase64 );
        }

        writer.EndArray();

        insert_data_as_base64_string( writer, "clientToServerKey", connectToken.clientToServerKey, KeyBytes );

        insert_data_as_base64_string( writer, "serverToClientKey", connectToken.serverToClientKey, KeyBytes );

        writer.EndObject();

        const char * json_output = s.GetString();

        int json_bytes = (int) strlen( json_output ) + 1;

        if ( json_bytes > outputSize )
            return false;

        memcpy( output, json_output, json_bytes );

        return true;
    }

    static bool read_int_from_string( Document & doc, const char * key, int & value )
    {
        if ( !doc.HasMember( key ) )
            return false;

        if ( !doc[key].IsString() )
            return false;

        value = atoi( doc[key].GetString() );

        return true;
    }

    static bool read_uint64_from_string( Document & doc, const char * key, uint64_t & value )
    {
        if ( !doc.HasMember( key ) )
            return false;

        if ( !doc[key].IsString() )
            return false;

        value = (uint64_t) strtoull( doc[key].GetString(), NULL, 10 );

        return true;
    }

    static bool read_data_from_base64_string( Document & doc, const char * key, uint8_t * data, int data_bytes )
    {
        if ( !doc.HasMember( key ) )
            return false;

        if ( !doc[key].IsString() )
            return false;

        const char * string = doc[key].GetString();

        const int string_length = (int) strlen( string );

        int read_data_bytes = base64_decode_data( string, data, string_length );

        return read_data_bytes == data_bytes;
    }

    bool ReadConnectTokenFromJSON( const char * json, ConnectToken & connectToken )
    {
        assert( json );

        Document doc;
        doc.Parse( json );
        if ( doc.HasParseError() )
            return false;

        if ( !read_uint64_from_string( doc, "protocolId", connectToken.protocolId ) )
            return false;

        if ( !read_uint64_from_string( doc, "clientId", connectToken.clientId ) )
            return false;

        if ( !read_uint64_from_string( doc, "expireTimestamp", connectToken.expireTimestamp ) )
            return false;

        if ( !read_int_from_string( doc, "numServerAddresses", connectToken.numServerAddresses ) )
            return false;

        if ( connectToken.numServerAddresses < 0 || connectToken.numServerAddresses > MaxServersPerConnect )
            return false;

        const Value & serverAddresses = doc["serverAddresses"];

        if ( !serverAddresses.IsArray() )
            return false;

        if ( (int) serverAddresses.Size() != connectToken.numServerAddresses )
            return false;

        for ( SizeType i = 0; i < serverAddresses.Size(); ++i )
        {
            if ( !serverAddresses[i].IsString() )
                return false;

            const char * string = serverAddresses[i].GetString();

            const int string_length = (int) strlen( string );

            const int MaxStringLength = 128;

            if ( string_length > 128 )
                return false;

            char buffer[MaxStringLength*2];

            base64_decode_string( string, buffer, sizeof( buffer ) );

            connectToken.serverAddresses[i] = Address( buffer );

            if ( !connectToken.serverAddresses[i].IsValid() )
                return false;
        }

        if ( !read_data_from_base64_string( doc, "clientToServerKey", connectToken.clientToServerKey, KeyBytes ) )
            return false;

        if ( !read_data_from_base64_string( doc, "serverToClientKey", connectToken.serverToClientKey, KeyBytes ) )
            return false;
        
        return true;
    }

    bool GenerateChallengeToken( const ConnectToken & connectToken, const uint8_t * connectTokenMac, ChallengeToken & challengeToken )
    {
        if ( connectToken.clientId == 0 )
            return false;

        challengeToken.clientId = connectToken.clientId;

        memcpy( challengeToken.connectTokenMac, connectTokenMac, MacBytes );

        return true;
    }

    bool EncryptChallengeToken( ChallengeToken & token, uint8_t * encryptedMessage, const uint8_t * nonce, const uint8_t * key )
    {
        uint8_t message[ChallengeTokenBytes - MacBytes];
        memset( message, 0, ChallengeTokenBytes - MacBytes );
        WriteStream stream( message, ChallengeTokenBytes - MacBytes );
        if ( !token.Serialize( stream ) )
            return false;

        stream.Flush();
        
        uint64_t encryptedLength;

        if ( !Encrypt_AEAD( message, ChallengeTokenBytes - MacBytes, encryptedMessage, encryptedLength, NULL, 0, nonce, key ) )
            return false;

        assert( encryptedLength == ChallengeTokenBytes );

        return true;
    }

    bool DecryptChallengeToken( const uint8_t * encryptedMessage, ChallengeToken & decryptedToken, const uint8_t * nonce, const uint8_t * key )
    {
        const int encryptedMessageLength = ChallengeTokenBytes;

        uint64_t decryptedMessageLength;
        uint8_t decryptedMessage[ChallengeTokenBytes];

        if ( !Decrypt_AEAD( encryptedMessage, encryptedMessageLength, decryptedMessage, decryptedMessageLength, NULL, 0, nonce, key ) )
            return false;

        assert( decryptedMessageLength == ChallengeTokenBytes - MacBytes );

        ReadStream stream( decryptedMessage, ChallengeTokenBytes - MacBytes );
        if ( !decryptedToken.Serialize( stream ) )
            return false;

        return true;
    }
}
