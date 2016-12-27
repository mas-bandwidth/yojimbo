package main

// #cgo pkg-config: libsodium
// #include <sodium.h>
import "C"

import (
    "time"
    "fmt"
    "log"
    "unsafe"
    "strconv"
    "net/http"
    "sync/atomic"
    "encoding/json"
    "encoding/base64"
    "encoding/binary"
    "github.com/gorilla/mux"
)

const Port = 8080
const NonceBytes = 8
const KeyBytes = 32
const AuthBytes = 16
const MacBytes = 16
const ConnectTokenBytes = 1024
const MaxServersPerConnectToken = 8
const ConnectTokenExpireSeconds = 30
const ServerAddress = "127.0.0.1:40000"

type ConnectToken struct {
    ProtocolId         string `json:"protocolId"`
    ClientId           string `json:"clientId"`
    ExpireTimestamp    string `json:"expireTimestamp"`
    NumServerAddresses string `json:"numServerAddresses"`
    ServerAddresses [] string `json:"serverAddresses"`
    ClientToServerKey  string `json:"clientToServerKey"`
    ServerToClientKey  string `json:"serverToClientKey"`
}

func GenerateKey() [] byte {
    key := [KeyBytes]byte {}
    C.randombytes_buf( unsafe.Pointer(&key), KeyBytes )
    return key[:]
}

func PrintBytes( label string, data [] byte ) {
    fmt.Printf( "%s: ", label )
    for i := 0; i < len( data ); i++ { fmt.Printf( "0x%02x,", data[i] ) }
    fmt.Printf( "\n" )
}

func Encrypt( message [] byte, additional [] byte, nonce uint64, key [] byte ) ( []byte, bool ) {
    nonceBytes := make( []byte, 8 )
    binary.LittleEndian.PutUint64( nonceBytes, nonce )
    encrypted := make( []byte, len(message) + AuthBytes )
    encryptedLengthLongLong := ( C.ulonglong( len( encrypted ) ) )
    ok := int( C.crypto_aead_chacha20poly1305_encrypt(
        (*C.uchar) ( &encrypted[0] ),
        &encryptedLengthLongLong,
        (*C.uchar) ( &message[0] ),
        (C.ulonglong) ( len( message ) ),
        (*C.uchar) ( &additional[0] ),
        (C.ulonglong) ( len( additional ) ),
        (*C.uchar) ( nil ),
        (*C.uchar) ( &nonceBytes[0] ),
        (*C.uchar) ( &key[0] ) ) ) == 0
    return encrypted, ok
}

func GenerateConnectToken( protocolId uint64, clientId uint64, serverAddresses [] string ) ConnectToken {
    connectToken := ConnectToken {}
    connectToken.ProtocolId = strconv.FormatUint( protocolId, 10 )
    connectToken.ClientId = strconv.FormatUint( clientId, 10 )
    connectToken.ExpireTimestamp = strconv.FormatUint( uint64( time.Now().Unix() + ConnectTokenExpireSeconds ), 10 )
    connectToken.NumServerAddresses = strconv.Itoa( len( serverAddresses ) )
    connectToken.ServerAddresses = serverAddresses
    connectToken.ClientToServerKey = base64.StdEncoding.EncodeToString( GenerateKey() )
    connectToken.ServerToClientKey = base64.StdEncoding.EncodeToString( GenerateKey() )
    return connectToken
}

func EncryptConnectToken( connectToken ConnectToken, nonce uint64 ) ( []byte, bool ) {
    connectTokenJSON, error := json.Marshal( connectToken )
    tokenData := make( []byte, ConnectTokenBytes - AuthBytes )
    for i := 0; i < len( connectTokenJSON ); i++ { tokenData[i] = connectTokenJSON[i] }
    if ( error != nil ) { return []byte(nil), false }
    expireTimestamp, _ := strconv.ParseUint( connectToken.ExpireTimestamp, 10, 64 );
    additionalData := make( []byte, 8 )
    binary.LittleEndian.PutUint64( additionalData, expireTimestamp )
    return Encrypt( tokenData, additionalData, nonce, PrivateKey )
}

type MatchResponse struct {
    ConnectTokenData                string `json:"connectTokenData"`
    ConnectTokenNonce               string `json:"connectTokenNonce"`
    ServerAddresses []              string `json:"serverAddresses"`
    ClientToServerKey               string `json:"clientToServerKey"`
    ServerToClientKey               string `json:"serverToClientKey"`
    ConnectTokenExpireTimestamp     string `json:"connectTokenExpireTimestamp"`
}

func GenerateMatchResponse( connectToken ConnectToken, nonce uint64 ) ( MatchResponse, bool ) {
    matchResponse := MatchResponse {}
    matchResponse.ConnectTokenNonce = strconv.FormatUint( nonce, 10 )
    encryptedConnectToken, ok := EncryptConnectToken( connectToken, nonce )
    if ( ok ) { matchResponse.ConnectTokenData = base64.StdEncoding.EncodeToString( encryptedConnectToken ) }
    matchResponse.ServerAddresses = connectToken.ServerAddresses
    matchResponse.ClientToServerKey = connectToken.ClientToServerKey
    matchResponse.ServerToClientKey = connectToken.ServerToClientKey
    matchResponse.ConnectTokenExpireTimestamp = connectToken.ExpireTimestamp
    return matchResponse, ok
}

var MatchNonce = uint64(0)

var PrivateKey = [] byte { 0x60, 0x6a, 0xbe, 0x6e, 0xc9, 0x19, 0x10, 0xea, 
                           0x9a, 0x65, 0x62, 0xf6, 0x6f, 0x2b, 0x30, 0xe4, 
                           0x43, 0x71, 0xd6, 0x2c, 0xd1, 0x99, 0x27, 0x26,
                           0x6b, 0x3c, 0x60, 0xf4, 0xb7, 0x15, 0xab, 0xa1 };

func Base64EncodeString( s string ) string {
    stringAsBytes := []byte( s )
    stringAsBytes = append( stringAsBytes, 0 )
    return base64.StdEncoding.EncodeToString( stringAsBytes )
}

func MatchHandler( w http.ResponseWriter, r * http.Request ) {
    vars := mux.Vars( r )
    clientId, _ := strconv.ParseUint( vars["clientId"], 10, 64 )
    protocolId, _ := strconv.ParseUint( vars["protocolId"], 10, 64 )
    serverAddresses := []string { Base64EncodeString( ServerAddress ) }
    connectToken := GenerateConnectToken( protocolId, clientId, serverAddresses[:] )
    matchResponse, ok := GenerateMatchResponse( connectToken, atomic.AddUint64( &MatchNonce, 1 ) )
    w.Header().Set( "Content-Type", "application/json" )
    if ( ok ) { 
        fmt.Printf( "matched client %.16x to %s\n", clientId, ServerAddress )
        json.NewEncoder(w).Encode( matchResponse ); 
    }
}

func main() {
    result := int( C.sodium_init() )
    if result != 0 { panic( "failed to initialize sodium" ) }
    fmt.Printf( "\nstarted matchmaker on port %d\n\n", Port )
    r := mux.NewRouter()
    r.HandleFunc( "/match/{protocolId:[0-9]+}/{clientId:[0-9]+}", MatchHandler )
    log.Fatal( http.ListenAndServeTLS( ":" + strconv.Itoa(Port), "server.pem", "server.key", r ) )
}
