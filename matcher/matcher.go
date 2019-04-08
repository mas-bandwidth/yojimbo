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
    "unsafe"
    "strconv"
    "net/http"
    "encoding/base64"
    "encoding/binary"
    "github.com/gorilla/mux"
    "github.com/gorilla/context"
)

const Port = 8080
const ServerAddress = "127.0.0.1"
const ServerPort = 40000
const KeyBytes = 32
const AuthBytes = 16
const ConnectTokenExpiry = 45
const ConnectTokenBytes = 2048
const ConnectTokenPrivateBytes = 1024
const UserDataBytes = 256
const TimeoutSeconds = 5
const VersionInfo = "NETCODE 1.02\x00"

var PrivateKey = [] byte { 0x60, 0x6a, 0xbe, 0x6e, 0xc9, 0x19, 0x10, 0xea, 
                           0x9a, 0x65, 0x62, 0xf6, 0x6f, 0x2b, 0x30, 0xe4, 
                           0x43, 0x71, 0xd6, 0x2c, 0xd1, 0x99, 0x27, 0x26,
                           0x6b, 0x3c, 0x60, 0xf4, 0xb7, 0x15, 0xab, 0xa1 };

const (
    ADDRESS_NONE = 0
    ADDRESS_IPV4 = 1
    ADDRESS_IPV6 = 2
)

func WriteAddresses( buffer []byte, addresses []net.UDPAddr ) (int) {
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
    return offset
}

type ConnectTokenPrivate struct {
    ClientId uint64
    TimeoutSeconds int32
    ServerAddresses []net.UDPAddr
    ClientToServerKey [KeyBytes]byte
    ServerToClientKey [KeyBytes]byte
    UserData [UserDataBytes]byte
}

func NewConnectTokenPrivate(clientId uint64, serverAddresses []net.UDPAddr, timeoutSeconds int32, userData []byte, clientToServerKey []byte, serverToClientKey []byte ) (*ConnectTokenPrivate) {
    connectTokenPrivate := &ConnectTokenPrivate{}
    connectTokenPrivate.ClientId = clientId
    connectTokenPrivate.TimeoutSeconds = timeoutSeconds
    connectTokenPrivate.ServerAddresses = serverAddresses
    copy( connectTokenPrivate.UserData[:], userData[0:UserDataBytes] )
    copy( connectTokenPrivate.ClientToServerKey[:], clientToServerKey[0:KeyBytes] )
    copy( connectTokenPrivate.ServerToClientKey[:], serverToClientKey[0:KeyBytes] )
    return connectTokenPrivate
}

func (token *ConnectTokenPrivate) Write( buffer []byte ) {
    binary.LittleEndian.PutUint64(buffer[0:], token.ClientId)
    binary.LittleEndian.PutUint32(buffer[8:], (uint32)(token.TimeoutSeconds))
    addressBytes := WriteAddresses( buffer[12:], token.ServerAddresses )
    copy( buffer[12+addressBytes:], token.ClientToServerKey[:] )
    copy( buffer[12+addressBytes+KeyBytes:], token.ServerToClientKey[:] )
    copy( buffer[12+addressBytes+KeyBytes*2:], token.UserData[:] )
}

type ConnectToken struct {
    ProtocolId uint64
    CreateTimestamp uint64
    ExpireTimestamp uint64
    Sequence uint64
    PrivateData *ConnectTokenPrivate
    TimeoutSeconds int32
    ServerAddresses []net.UDPAddr
    ClientToServerKey [KeyBytes]byte
    ServerToClientKey [KeyBytes]byte
    PrivateKey [KeyBytes]byte
}

func NewConnectToken(clientId uint64, serverAddresses []net.UDPAddr, protocolId uint64, expireSeconds uint64, timeoutSeconds int32, userData []byte, privateKey []byte) (*ConnectToken) {
    connectToken := &ConnectToken{}
    connectToken.ProtocolId = protocolId
    connectToken.CreateTimestamp = uint64(time.Now().Unix())
    if expireSeconds >= 0 {
        connectToken.ExpireTimestamp = connectToken.CreateTimestamp + expireSeconds
    } else {
        connectToken.ExpireTimestamp = 0xFFFFFFFFFFFFFFFF
    }
    connectToken.TimeoutSeconds = timeoutSeconds
    connectToken.ServerAddresses = serverAddresses
    C.randombytes_buf(unsafe.Pointer(&connectToken.ClientToServerKey[0]), KeyBytes)
    C.randombytes_buf(unsafe.Pointer(&connectToken.ServerToClientKey[0]), KeyBytes)
    copy( connectToken.PrivateKey[:], privateKey[:] )
    connectToken.PrivateData = NewConnectTokenPrivate( clientId, serverAddresses, timeoutSeconds, userData, connectToken.ClientToServerKey[:], connectToken.ServerToClientKey[:] )
    return connectToken
}

func EncryptAEAD(message []byte, additional []byte, nonce []byte, key []byte) bool {
    encryptedLengthLongLong := (C.ulonglong(len(message)))
    return C.crypto_aead_xchacha20poly1305_ietf_encrypt(
        (*C.uchar)(&message[0]),
        &encryptedLengthLongLong,
        (*C.uchar)(&message[0]),
        (C.ulonglong)(len(message)),
        (*C.uchar)(&additional[0]),
        (C.ulonglong)(len(additional)),
        (*C.uchar)(nil),
        (*C.uchar)(&nonce[0]),
        (*C.uchar)(&key[0])) == 0
}

func (token *ConnectToken) Write( buffer []byte ) (bool) {
    copy( buffer, VersionInfo )
    binary.LittleEndian.PutUint64(buffer[13:], token.ProtocolId)
    binary.LittleEndian.PutUint64(buffer[21:], token.CreateTimestamp)
    binary.LittleEndian.PutUint64(buffer[29:], token.ExpireTimestamp)
    nonce := make([]byte, 24)
    C.randombytes_buf(unsafe.Pointer(&nonce[0]), 24)
    copy( buffer[37:], nonce[:] )
    token.PrivateData.Write( buffer[61:] )
    additional := make([]byte, 13+8+8)
    copy( additional, VersionInfo[0:13] )
    binary.LittleEndian.PutUint64(additional[13:], token.ProtocolId)
    binary.LittleEndian.PutUint64(additional[21:], token.ExpireTimestamp)
    if !EncryptAEAD( buffer[61:61+ConnectTokenPrivateBytes-AuthBytes], additional[:], nonce[:], token.PrivateKey[:] ) {
        return false
    }
    binary.LittleEndian.PutUint32(buffer[ConnectTokenPrivateBytes+61:], (uint32)(token.TimeoutSeconds))
    offset := WriteAddresses( buffer[1024+61+4:], token.ServerAddresses )
    copy( buffer[1024+61+4+offset:], token.ClientToServerKey[:] )
    copy( buffer[1024+61+4+offset+KeyBytes:], token.ServerToClientKey[:] )
    return true
}

func GenerateConnectToken(clientId uint64, serverAddresses []net.UDPAddr, protocolId uint64, expireSeconds uint64, timeoutSeconds int32, userData []byte, privateKey []byte) ([]byte) {
    connectToken := NewConnectToken( clientId, serverAddresses, protocolId, expireSeconds, timeoutSeconds, userData, privateKey )
    if connectToken == nil {
        return nil
    }
    buffer := make([]byte, ConnectTokenBytes )
    if !connectToken.Write( buffer ) {
        return nil
    }
    return buffer
}

func MatchHandler( w http.ResponseWriter, r * http.Request ) {
    vars := mux.Vars( r )
    clientId, _ := strconv.ParseUint( vars["clientId"], 10, 64 )
    protocolId, _ := strconv.ParseUint( vars["protocolId"], 10, 64 )
    serverAddresses := make( []net.UDPAddr, 1 )
    serverAddresses[0] = net.UDPAddr{ IP: net.ParseIP( ServerAddress ), Port: ServerPort }
    userData := make( []byte, UserDataBytes )
    connectToken := GenerateConnectToken( clientId, serverAddresses, protocolId, ConnectTokenExpiry, TimeoutSeconds, userData, PrivateKey )
    if connectToken == nil {
        log.Printf( "error: failed to generate connect token" )
        return
    }
    connectTokenBase64 := base64.StdEncoding.EncodeToString( connectToken )
    w.Header().Set( "Content-Type", "application/text" )
    if _, err := io.WriteString( w, connectTokenBase64 ); err != nil {
        log.Printf( "error: failed to write string response" )
        return
    }
    fmt.Printf( "matched client %.16x to %s:%d\n", clientId, ServerAddress, ServerPort )
}

func main() {
    fmt.Printf( "\nstarted matchmaker on port %d\n\n", Port )
    router := mux.NewRouter()
    router.HandleFunc( "/match/{protocolId:[0-9]+}/{clientId:[0-9]+}", MatchHandler )
    log.Fatal( http.ListenAndServeTLS( ":" + strconv.Itoa(Port), "server.pem", "server.key", context.ClearHandler( router ) ) )
}
