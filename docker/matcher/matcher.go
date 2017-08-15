package main

// #cgo pkg-config: libsodium
// #include <sodium.h>
import "C"

import (
    "io"
    "fmt"
    "log"
    "net"
    "time"
    "strconv"
    "net/http"
    "sync/atomic"
    "encoding/base64"
    "encoding/binary"
    "github.com/gorilla/mux"
    "github.com/gorilla/context"
)

const Port = 8080
const ServerAddress = "127.0.0.1"
const ServerPort = 40000
const ConnectTokenExpiry = 45
const ConnectTokenBytes = 2048
const ConnectTokenPrivateBytes = 1024
const UserDataBytes = 256
const TimeoutSeconds = 5
const VersionInfo = "NETCODE 1.01\x00"

var MatchNonce = uint64(0)

var PrivateKey = [] byte { 0x60, 0x6a, 0xbe, 0x6e, 0xc9, 0x19, 0x10, 0xea, 
                           0x9a, 0x65, 0x62, 0xf6, 0x6f, 0x2b, 0x30, 0xe4, 
                           0x43, 0x71, 0xd6, 0x2c, 0xd1, 0x99, 0x27, 0x26,
                           0x6b, 0x3c, 0x60, 0xf4, 0xb7, 0x15, 0xab, 0xa1 };


/*
// Writes the servers and client <-> server keys to the supplied buffer
func (shared *sharedTokenData) WriteShared(buffer *Buffer) error {
    buffer.WriteInt32(shared.TimeoutSeconds)
    buffer.WriteUint32(uint32(len(shared.ServerAddrs)))

    for _, addr := range shared.ServerAddrs {
        host, port, err := net.SplitHostPort(addr.String())
        if err != nil {
            return errors.New("invalid port for host: " + addr.String())
        }

        parsed := net.ParseIP(host)
        if parsed == nil {
            return errors.New("invalid ip address")
        }

        parsedIpv4 := parsed.To4()
        if parsedIpv4 != nil {
            buffer.WriteUint8(uint8(ADDRESS_IPV4))
            for i := 0; i < len(parsedIpv4); i += 1 {
                buffer.WriteUint8(parsedIpv4[i])
            }
        } else {
            buffer.WriteUint8(uint8(ADDRESS_IPV6))
            for i := 0; i < len(parsed); i += 2 {
                var n uint16
                // net.IP is already big endian encoded, encode it to create little endian encoding.
                n = uint16(parsed[i]) << 8
                n = uint16(parsed[i+1])
                buffer.WriteUint16(n)
            }
        }

        p, err := strconv.ParseUint(port, 10, 16)
        if err != nil {
            return err
        }
        buffer.WriteUint16(uint16(p))
    }
    buffer.WriteBytesN(shared.ClientKey, KEY_BYTES)
    buffer.WriteBytesN(shared.ServerKey, KEY_BYTES)
    return nil
}
*/

/*
type ConnectTokenPrivate struct {
    sharedTokenData         // holds the server addresses, client <-> server keys
    ClientId        uint64  // id for this token
    UserData        []byte  // used to store user data
    mac             []byte  // used to store the message authentication code after encryption/before decryption
//    TokenData       *Buffer // used to store the serialized/encrypted buffer
}
*/

/*
func NewConnectTokenPrivate(clientId uint64, timeoutSeconds int32, serverAddrs []net.UDPAddr, userData []byte) *ConnectTokenPrivate {
    p := &ConnectTokenPrivate{}
    p.TokenData = NewBuffer(CONNECT_TOKEN_PRIVATE_BYTES)
    p.TimeoutSeconds = timeoutSeconds
    p.ClientId = clientId
    p.UserData = userData
    p.ServerAddrs = serverAddrs
    p.mac = make([]byte, MAC_BYTES)
    return p
}

func (p *ConnectTokenPrivate) Generate() error {
    return p.GenerateShared()
}
*/

/*
func NewConnectTokenPrivateEncrypted(buffer []byte) *ConnectTokenPrivate {
    p := &ConnectTokenPrivate{}
    p.mac = make([]byte, MAC_BYTES)
    p.TokenData = NewBufferFromRef(buffer)
    return p
}
*/

/*
func (p *ConnectTokenPrivate) Mac() []byte {
    return p.mac
}
*/

/*
func (p *ConnectTokenPrivate) Write() ([]byte, error) {
    p.TokenData.WriteUint64(p.ClientId)

    if err := p.WriteShared(p.TokenData); err != nil {
        return nil, err
    }

    p.TokenData.WriteBytesN(p.UserData, USER_DATA_BYTES)
    return p.TokenData.Buf, nil
}
*/

/*
// Encrypts, in place, the TokenData buffer, assumes Write() has already been called.
func (token *ConnectTokenPrivate) Encrypt(protocolId, expireTimestamp, sequence uint64, privateKey []byte) error {
    additionalData, nonce := buildTokenCryptData(protocolId, expireTimestamp, sequence)
    encBuf := token.TokenData.Buf[:CONNECT_TOKEN_PRIVATE_BYTES-MAC_BYTES]
    if err := EncryptAead(encBuf, additionalData, nonce, privateKey); err != nil {
        return err
    }

    if len(token.TokenData.Buf) != CONNECT_TOKEN_PRIVATE_BYTES {
        return errors.New("error in encrypt invalid token private byte size")
    }

    copy(token.mac, token.TokenData.Buf[CONNECT_TOKEN_PRIVATE_BYTES-MAC_BYTES:])
    return nil
}
*/

/*
func buildTokenCryptData(protocolId, expireTimestamp, sequence uint64) ([]byte, []byte) {
    additionalData := NewBuffer(VERSION_INFO_BYTES + 8 + 8)
    additionalData.WriteBytes([]byte(VERSION_INFO))
    additionalData.WriteUint64(protocolId)
    additionalData.WriteUint64(expireTimestamp)

    nonce := NewBuffer(SizeUint64 + SizeUint32)
    nonce.WriteUint32(0)
    nonce.WriteUint64(sequence)
    return additionalData.Buf, nonce.Buf
}
*/

/*
// Token used for connecting
type ConnectToken struct {
    sharedTokenData                      // a shared container holding the server addresses, client and server keys
    VersionInfo     []byte               // the version information for client <-> server communications
    ProtocolId      uint64               // protocol id for communications
    CreateTimestamp uint64               // when this token was created
    ExpireTimestamp uint64               // when this token expires
    Sequence        uint64               // the sequence id
    PrivateData     *ConnectTokenPrivate // reference to the private parts of this connect token
}

// Create a new empty token and empty private token
func NewConnectToken() *ConnectToken {
    token := &ConnectToken{}
    token.PrivateData = &ConnectTokenPrivate{}
    return token
}

// Generates the token and private token data with the supplied config values and sequence id.
// This will also write and encrypt the private token
func (token *ConnectToken) Generate(clientId uint64, serverAddrs []net.UDPAddr, versionInfo string, protocolId uint64, expireSeconds uint64, timeoutSeconds int32, sequence uint64, userData, privateKey []byte) error {
    token.CreateTimestamp = uint64(time.Now().Unix())
    token.ExpireTimestamp = token.CreateTimestamp + (expireSeconds * 1000)
    token.TimeoutSeconds = timeoutSeconds
    token.VersionInfo = []byte(VersionInfo)
    token.ProtocolId = protocolId
    token.Sequence = sequence

    token.PrivateData = NewConnectTokenPrivate(clientId, timeoutSeconds, serverAddrs, userData)
    if err := token.PrivateData.Generate(); err != nil {
        return err
    }

    token.ClientKey = token.PrivateData.ClientKey
    token.ServerKey = token.PrivateData.ServerKey
    token.ServerAddrs = serverAddrs

    if _, err := token.PrivateData.Write(); err != nil {
        return err
    }

    if err := token.PrivateData.Encrypt(token.ProtocolId, token.ExpireTimestamp, sequence, privateKey); err != nil {
        return err
    }

    return nil
}

// Writes the ConnectToken and previously encrypted ConnectTokenPrivate data to a byte slice
func (token *ConnectToken) Write() ([]byte, error) {
    buffer := NewBuffer(CONNECT_TOKEN_BYTES)
    buffer.WriteBytes(token.VersionInfo)
    buffer.WriteUint64(token.ProtocolId)
    buffer.WriteUint64(token.CreateTimestamp)
    buffer.WriteUint64(token.ExpireTimestamp)
    buffer.WriteUint64(token.Sequence)

    // assumes private token has already been encrypted
    buffer.WriteBytes(token.PrivateData.Buffer())

    // writes server/client key and addresses to public part of the buffer
    if err := token.WriteShared(buffer); err != nil {
        return nil, err
    }

    return buffer.Buf, nil
}
*/

type ConnectToken struct {
    ProtocolId uint64
    CreateTimestamp uint64
    ExpireTimestamp uint64
    Sequence uint64
    TimeoutSeconds int32
    ServerAddresses []net.UDPAddr
}

func NewConnectToken(clientId uint64, serverAddresses []net.UDPAddr, protocolId uint64, expireSeconds uint64, timeoutSeconds int32, sequence uint64, userData []byte, privateKey []byte) (*ConnectToken, error) {
    token := &ConnectToken{}
    token.ProtocolId = protocolId
    token.CreateTimestamp = uint64(time.Now().Unix())
    if expireSeconds >= 0 {
        token.ExpireTimestamp = token.CreateTimestamp + expireSeconds
    } else {
        token.ExpireTimestamp = 0xFFFFFFFFFFFFFFFF
    }
    token.Sequence = sequence
    token.TimeoutSeconds = timeoutSeconds
    token.ServerAddresses = serverAddresses
    return token, nil
}

const (
    ADDRESS_NONE = 0
    ADDRESS_IPV4 = 1
    ADDRESS_IPV6 = 2
)

func WriteAddresses( buffer []byte, addresses []net.UDPAddr ) {
    binary.LittleEndian.PutUint32(buffer[0:], (uint32)(len(addresses)))
    offset := 4
    for _, addr := range addresses {
        ipv4 := addr.IP.To4()
        port := addr.Port
        if ipv4 != nil {
            buffer[offset] = ADDRESS_IPV4
            buffer[offset+1] = ipv4[0]
            buffer[offset+2] = ipv4[1]
            buffer[offset+3] = ipv4[2]
            buffer[offset+4] = ipv4[3]
            buffer[offset+5] = (byte) (port&0xFF)
            buffer[offset+6] = (byte) (port>>8)
        } else {
            buffer[offset] = ADDRESS_IPV6
            copy( buffer[offset+1:], addr.IP )
            buffer[offset+17] = (byte) (port&0xFF)
            buffer[offset+18] = (byte) (port>>8)
        }
        offset += 19
    }
}

func (token *ConnectToken) Write() ([]byte, error) {
    buffer := make([]byte, ConnectTokenBytes )
    copy( buffer, VersionInfo )
    binary.LittleEndian.PutUint64(buffer[13:], token.ProtocolId)
    binary.LittleEndian.PutUint64(buffer[21:], token.CreateTimestamp)
    binary.LittleEndian.PutUint64(buffer[29:], token.ExpireTimestamp)
    binary.LittleEndian.PutUint64(buffer[37:], token.Sequence)
    // todo: write private connect token data
    binary.LittleEndian.PutUint32(buffer[ConnectTokenPrivateBytes+45:], (uint32)(token.TimeoutSeconds))
    WriteAddresses( buffer[1024+49:], token.ServerAddresses )
    return buffer, nil
}

func MatchHandler( w http.ResponseWriter, r * http.Request ) {
    vars := mux.Vars( r )
    atomic.AddUint64( &MatchNonce, 1 )
    clientId, _ := strconv.ParseUint( vars["clientId"], 10, 64 )
    protocolId, _ := strconv.ParseUint( vars["protocolId"], 10, 64 )
    serverAddresses := make( []net.UDPAddr, 1 )
    serverAddresses[0] = net.UDPAddr{ IP: net.ParseIP( ServerAddress ), Port: ServerPort }
    userData := make( []byte, UserDataBytes )
    connectToken, err := NewConnectToken( clientId, serverAddresses, protocolId, ConnectTokenExpiry, TimeoutSeconds, MatchNonce, userData, PrivateKey ); 
    if err != nil {
        panic( err )
    }
    connectTokenData, err := connectToken.Write(); 
    if err != nil {
        panic( err )
    }
    connectTokenString := base64.StdEncoding.EncodeToString( connectTokenData )
    w.Header().Set( "Content-Type", "application/text" )
    if _, err := io.WriteString( w, connectTokenString ); err != nil {
        panic( err )
    }
    fmt.Printf( "matched client %.16x to %s:%d [%.16x]\n", clientId, ServerAddress, ServerPort, protocolId )
}

func main() {
    fmt.Printf( "\nstarted matchmaker on port %d\n\n", Port )
    router := mux.NewRouter()
    router.HandleFunc( "/match/{protocolId:[0-9]+}/{clientId:[0-9]+}", MatchHandler )
    log.Fatal( http.ListenAndServeTLS( ":" + strconv.Itoa(Port), "server.pem", "server.key", context.ClearHandler( router ) ) )
}
