package main

// #cgo pkg-config: libsodium
// #include <sodium.h>
// #include <stdlib.h>
import "C"

import (
    "time"
    "fmt"
    "log"
    "unsafe"
    "strconv"
    "net/http"
    "encoding/json"
    "encoding/base64"
    "github.com/gorilla/mux"
)

const Port = 8080
const NonceBytes = 8
const KeyBytes = 32
const AuthBytes = 16
const MacBytes = 16
const MaxServersPerConnectToken = 8
const ConnectTokenExpirySeconds = 10
const ServerAddress = "127.0.0.1:5000"

type ConnectToken struct {
    ProtocolId         string `json:"protocolId"`
    ClientId           string `json:"clientId"`
    ExpiryTimestamp    string `json:"expiryTimestamp"`
    ServerAddresses [] string `json:"serverAddresses"`
    ClientToServerKey  string `json:"clientToServerKey"`
    ServerToClientKey  string `json:"serverToClientKey"`
    Random             string `json:"random"`
}

func GenerateKey() [] byte {
    key := [KeyBytes]byte {}
    C.randombytes_buf( unsafe.Pointer(&key), KeyBytes )
    return key[:]
}

func EncryptAEAD( message [] byte, additional [] byte, nonce [] byte, key [] byte ) ( []byte, bool ) {
    encrypted := make( [] byte, len( message ) + AuthBytes )
    encryptedLengthLongLong := ( C.ulonglong( len( encrypted ) ) )
    ok := int( C.crypto_aead_aes256gcm_encrypt(
        (*C.uchar) ( &encrypted[0]),
        &encryptedLengthLongLong,
        (*C.uchar) ( &message[0] ),
        (C.ulonglong) ( len( message ) ),
        (*C.uchar) ( &additional[0] ),
        (C.ulonglong) ( len( additional ) ),
        (*C.uchar) ( nil ),
        (*C.uchar) ( &nonce[0]),
        (*C.uchar) ( &key[0] ) ) ) == 0
    return encrypted, ok
}

func GenerateConnectToken( protocolId uint32, clientId uint64, serverAddresses [] string ) ConnectToken {
    connectToken := ConnectToken {}
    connectToken.ProtocolId = strconv.FormatUint( uint64(protocolId), 10 )
    connectToken.ClientId = strconv.FormatUint( clientId, 10 )
    connectToken.ExpiryTimestamp = strconv.FormatUint( uint64( time.Now().Unix() + ConnectTokenExpirySeconds ), 10 )
    connectToken.ServerAddresses = serverAddresses
    connectToken.ClientToServerKey = base64.StdEncoding.EncodeToString( GenerateKey() )
    connectToken.ServerToClientKey = base64.StdEncoding.EncodeToString( GenerateKey() )
    connectToken.Random = base64.StdEncoding.EncodeToString( GenerateKey() )
    return connectToken
}

func EncryptConnectToken( connectToken ConnectToken, nonce uint64 ) ( []byte, bool ) {
    connectTokenJSON, error := json.Marshal( connectToken )
    if ( error != nil ) { return []byte(nil), false }
    // ...
    return []byte( connectTokenJSON ), true
}

type MatchResponse struct {
    ConnectToken       string `json:"connectToken"`
    ServerAddresses [] string `json:"serverAddresses"`
    ClientToServerKey  string `json:"clientToServerKey"`     // IMPORTANT: Make sure you send this over HTTPS!
    ServerToClientKey  string `json:"serverToClientKey"`
}

func GenerateMatchResponse( connectToken ConnectToken, nonce uint64 ) ( MatchResponse, bool ) {
    matchResponse := MatchResponse {}
    encryptedConnectToken, ok := EncryptConnectToken( connectToken, nonce )
    if ( ok ) { matchResponse.ConnectToken = base64.StdEncoding.EncodeToString( encryptedConnectToken ) }
    matchResponse.ServerAddresses = connectToken.ServerAddresses
    matchResponse.ClientToServerKey = connectToken.ClientToServerKey
    matchResponse.ServerToClientKey = connectToken.ServerToClientKey
    return matchResponse, ok
}

var MatchNonce = uint64(0)

var PrivateKey = [] byte { 0x60, 0x6a, 0xbe, 0x6e, 0xc9, 0x19, 0x10, 0xea, 
                           0x9a, 0x65, 0x62, 0xf6, 0x6f, 0x2b, 0x30, 0xe4, 
                           0x43, 0x71, 0xd6, 0x2c, 0xd1, 0x99, 0x27, 0x26,
                           0x6b, 0x3c, 0x60, 0xf4, 0xb7, 0x15, 0xab, 0xa1 };

func MatchHandler( w http.ResponseWriter, r * http.Request ) {
    clientId := uint64(1)
    protocolId := uint32(0x12341561)
    serverAddresses := []string { base64.StdEncoding.EncodeToString( []byte( ServerAddress ) ) }
    connectToken := GenerateConnectToken( protocolId, clientId, serverAddresses[:] )
    matchResponse, ok := GenerateMatchResponse( connectToken, MatchNonce )
    if ( ok ) { json.NewEncoder(w).Encode( matchResponse ); MatchNonce++ }
}

func main() {
    result := int( C.sodium_init() )
    if result != 0 { panic( "failed to initiliaze sodium" ) }
    fmt.Printf( "\nstarted matchmaker on port %d\n\n", Port )
    r := mux.NewRouter()
    r.HandleFunc( "/match", MatchHandler )
    log.Fatal( http.ListenAndServe( ":" + strconv.Itoa(Port), r ) )
}
