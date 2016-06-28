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
const ConnectTokenExpirySeconds = 10
const ServerAddress = "127.0.0.1:50000"

type ConnectToken struct {
    ProtocolId         string `json:"protocolId"`
    ClientId           string `json:"clientId"`
    ExpiryTimestamp    string `json:"expiryTimestamp"`
    NumServerAddresses string `json:"numServerAddresses"`
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

func PrintBytes( label string, data [] byte ) {
    fmt.Printf( "%s: ", label )
    for i := 0; i < len( data ); i++ { fmt.Printf( "0x%02x,", data[i] ) }
    fmt.Printf( "\n" )
}

func Encrypt( message [] byte, nonce uint64, key [] byte ) ( []byte, bool ) {
    nonceBytes := make( []byte, 8 )
    binary.LittleEndian.PutUint64( nonceBytes, nonce )
    encrypted := make( []byte, len(message) + AuthBytes )
    encryptedLengthLongLong := ( C.ulonglong( len( encrypted ) ) )
    ok := int( C.crypto_aead_chacha20poly1305_encrypt(
        (*C.uchar) ( &encrypted[0] ),
        &encryptedLengthLongLong,
        (*C.uchar) ( &message[0] ),
        (C.ulonglong) ( len( message ) ),
        (*C.uchar) ( nil ),
        (C.ulonglong) ( 0 ),
        (*C.uchar) ( nil ),
        (*C.uchar) ( &nonceBytes[0] ),
        (*C.uchar) ( &key[0] ) ) ) == 0
//    PrintBytes( "nonce", nonceBytes )
//    PrintBytes( "encrypted", encrypted )
    return encrypted, ok
}

func GenerateConnectToken( protocolId uint32, clientId uint64, serverAddresses [] string ) ConnectToken {
    connectToken := ConnectToken {}
    connectToken.ProtocolId = strconv.FormatUint( uint64(protocolId), 10 )
    connectToken.ClientId = strconv.FormatUint( clientId, 10 )
    connectToken.ExpiryTimestamp = strconv.FormatUint( uint64( time.Now().Unix() + ConnectTokenExpirySeconds ), 10 )
    connectToken.NumServerAddresses = strconv.Itoa( len( serverAddresses ) )
    connectToken.ServerAddresses = serverAddresses
    connectToken.ClientToServerKey = base64.StdEncoding.EncodeToString( GenerateKey() )
    connectToken.ServerToClientKey = base64.StdEncoding.EncodeToString( GenerateKey() )
    connectToken.Random = base64.StdEncoding.EncodeToString( GenerateKey() )
    return connectToken
}

func EncryptConnectToken( connectToken ConnectToken, nonce uint64 ) ( []byte, bool ) {
    connectTokenJSON, error := json.Marshal( connectToken )
    tokenData := make( []byte, ConnectTokenBytes - AuthBytes )
    for i := 0; i < len( connectTokenJSON ); i++ { tokenData[i] = connectTokenJSON[i] }
    if ( error != nil ) { return []byte(nil), false }
    return Encrypt( tokenData, nonce, PrivateKey )
}

type MatchResponse struct {
    ConnectToken       string `json:"connectToken"`
    ConnectNonce       string `json:"connectNonce"`
    ServerAddresses [] string `json:"serverAddresses"`
    ClientToServerKey  string `json:"clientToServerKey"`
    ServerToClientKey  string `json:"serverToClientKey"`
}

func GenerateMatchResponse( connectToken ConnectToken, nonce uint64 ) ( MatchResponse, bool ) {
    matchResponse := MatchResponse {}
    matchResponse.ConnectNonce = strconv.FormatUint( nonce, 10 )
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
    vars := mux.Vars( r )
    clientId, _ := strconv.ParseUint( vars["clientId"], 10, 64 )
    protocolId, _ := strconv.ParseUint( vars["protocolId"], 10, 32 )
    serverAddresses := []string { base64.StdEncoding.EncodeToString( []byte( ServerAddress ) ) }
    connectToken := GenerateConnectToken( uint32( protocolId ), clientId, serverAddresses[:] )
    matchResponse, ok := GenerateMatchResponse( connectToken, MatchNonce )
    if ( ok ) { json.NewEncoder(w).Encode( matchResponse ); MatchNonce++ }
}

func main() {
    result := int( C.sodium_init() )
    if result != 0 { panic( "failed to initialize sodium" ) }
    fmt.Printf( "\nstarted matchmaker on port %d\n\n", Port )
    r := mux.NewRouter()
    r.HandleFunc( "/match/{protocolId:[0-9]+}/{clientId:[0-9]+}", MatchHandler )
    log.Fatal( http.ListenAndServe( ":" + strconv.Itoa(Port), r ) )
}
