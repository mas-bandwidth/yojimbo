package main

import (
    "fmt"
    "log"
    "strconv"
    "net/http"
    "sync/atomic"
    "encoding/base64"
    "github.com/gorilla/mux"
    "github.com/gorilla/context"
    "github.com/networkprotocol/netcode.io/go/netcode"
)

const Port = 8080
const ServerAddress = "127.0.0.1:40000"

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
    atomic.AddUint64( &MatchNonce, 1 )
    /*
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
    */
}

func main() {
    fmt.Printf( "\nstarted matchmaker on port %d\n\n", Port )
    router := mux.NewRouter()
    router.HandleFunc( "/match/{protocolId:[0-9]+}/{clientId:[0-9]+}", MatchHandler )
    log.Fatal( http.ListenAndServeTLS( ":" + strconv.Itoa(Port), "server.pem", "server.key", context.ClearHandler( router ) ) )
}
