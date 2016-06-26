package main

import (
    "time"
    "fmt"
    "log"
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
const ServerAddress = "127.0.0.1:5000"
const MaxServersPerConnectToken = 8
const ConnectTokenExpirySeconds = 10

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
    // todo: actually get this key from libsodium
    key := [KeyBytes]byte { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    return key[:]
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

func HomeHandler( w http.ResponseWriter, r * http.Request ) {
    clientId := uint64(1)
    protocolId := uint32(0x12341561)
    // todo: don't base64 here. pass in server address strings as array and base64 inside GenerateConnectToken
    serverAddresses := [1]string { base64.StdEncoding.EncodeToString( []byte( ServerAddress ) ) }
    connectToken := GenerateConnectToken( protocolId, clientId, serverAddresses[:] )
    json.NewEncoder(w).Encode( connectToken )
}

func main() {
    fmt.Printf( "\nstarted matchmaker on port %d\n\n", Port )
    r := mux.NewRouter()
    r.HandleFunc( "/", HomeHandler )
    log.Fatal( http.ListenAndServe( ":" + strconv.Itoa(Port), r ) )
}
