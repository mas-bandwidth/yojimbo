package main

import (
	"bytes"
	"encoding/base64"
	"encoding/binary"
	"io"
	"net"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/go-chi/chi/v5"
	"golang.org/x/crypto/chacha20poly1305"
)

func TestGenerateConnectTokenNetcode103(t *testing.T) {
	if len(versionInfo) != 13 || versionInfo != "NETCODE 1.03\x00" {
		t.Fatalf("unexpected versionInfo %q (len %d)", versionInfo, len(versionInfo))
	}

	const (
		testClientID   = uint64(0x0102030405060708)
		testProtocolID = uint64(0x1122334455667788)
	)

	addrs := []net.UDPAddr{
		{IP: net.ParseIP("127.0.0.1"), Port: 40000},
		{IP: net.ParseIP("::1"), Port: 40001},
	}

	var userData [userDataBytes]byte
	for i := range userData {
		userData[i] = byte(i*7 + 3)
	}

	token, err := generateConnectToken(testClientID, addrs, testProtocolID, connectTokenExpiry, timeoutSeconds, userData[:], privateKey)
	if err != nil {
		t.Fatalf("generateConnectToken failed: %v", err)
	}
	if len(token) != connectTokenBytes {
		t.Fatalf("expected %d-byte connect token, got %d", connectTokenBytes, len(token))
	}
	if string(token[:13]) != "NETCODE 1.03\x00" {
		t.Fatalf("unexpected wire version prefix %q", string(token[:13]))
	}

	gotProtocolID := binary.LittleEndian.Uint64(token[13:21])
	if gotProtocolID != testProtocolID {
		t.Fatalf("protocolID mismatch: got %#x, want %#x", gotProtocolID, testProtocolID)
	}
	createTimestamp := binary.LittleEndian.Uint64(token[21:29])
	expireTimestamp := binary.LittleEndian.Uint64(token[29:37])
	if expireTimestamp != createTimestamp+connectTokenExpiry {
		t.Fatalf("unexpected timestamps: create=%d expire=%d", createTimestamp, expireTimestamp)
	}

	nonce := token[37:61]
	encryptedPrivate := append([]byte(nil), token[61:61+connectTokenPrivateBytes]...)

	var aad [13 + 8 + 8]byte
	copy(aad[:13], versionInfo)
	binary.LittleEndian.PutUint64(aad[13:21], gotProtocolID)
	binary.LittleEndian.PutUint64(aad[21:29], expireTimestamp)

	aead, err := chacha20poly1305.NewX(privateKey)
	if err != nil {
		t.Fatalf("NewX failed: %v", err)
	}
	plaintextPrivate, err := aead.Open(nil, nonce, encryptedPrivate, aad[:])
	if err != nil {
		t.Fatalf("private connect token failed XChaCha20-Poly1305 decryption under NETCODE 1.03 AAD: %v", err)
	}
	if len(plaintextPrivate) != connectTokenPrivateBytes-authBytes {
		t.Fatalf("unexpected decrypted private token size %d", len(plaintextPrivate))
	}

	if gotClientID := binary.LittleEndian.Uint64(plaintextPrivate[0:8]); gotClientID != testClientID {
		t.Fatalf("private clientID mismatch: got %#x, want %#x", gotClientID, testClientID)
	}
	if gotTimeout := int32(binary.LittleEndian.Uint32(plaintextPrivate[8:12])); gotTimeout != timeoutSeconds {
		t.Fatalf("private timeout mismatch: got %d, want %d", gotTimeout, timeoutSeconds)
	}
	if numServers := binary.LittleEndian.Uint32(plaintextPrivate[12:16]); numServers != 2 {
		t.Fatalf("private numServers mismatch: got %d, want 2", numServers)
	}

	addrBytes := 4 + 7 + 19
	privC2S := plaintextPrivate[12+addrBytes : 12+addrBytes+keyBytes]
	privS2C := plaintextPrivate[12+addrBytes+keyBytes : 12+addrBytes+keyBytes*2]
	privUserData := plaintextPrivate[12+addrBytes+keyBytes*2 : 12+addrBytes+keyBytes*2+userDataBytes]
	if !bytes.Equal(privUserData, userData[:]) {
		t.Fatal("private userData mismatch")
	}

	pubOffset := 61 + connectTokenPrivateBytes
	if pubTimeout := int32(binary.LittleEndian.Uint32(token[pubOffset : pubOffset+4])); pubTimeout != timeoutSeconds {
		t.Fatalf("public timeout mismatch: got %d, want %d", pubTimeout, timeoutSeconds)
	}
	pubC2S := token[pubOffset+4+addrBytes : pubOffset+4+addrBytes+keyBytes]
	pubS2C := token[pubOffset+4+addrBytes+keyBytes : pubOffset+4+addrBytes+keyBytes*2]
	if !bytes.Equal(pubC2S, privC2S) || !bytes.Equal(pubS2C, privS2C) {
		t.Fatal("public/private connect token key mismatch")
	}
}

func TestMatchHandlerServesValidNetcode103Token(t *testing.T) {
	router := chi.NewRouter()
	router.Get("/match/{protocolID:[0-9]+}/{clientID:[0-9]+}", matchHandler)

	req := httptest.NewRequest(http.MethodGet, "/match/123456789/42", nil)
	rec := httptest.NewRecorder()
	router.ServeHTTP(rec, req)

	res := rec.Result()
	defer res.Body.Close()
	if res.StatusCode != http.StatusOK {
		t.Fatalf("expected 200 OK, got %d", res.StatusCode)
	}

	body, err := io.ReadAll(res.Body)
	if err != nil {
		t.Fatalf("failed to read response body: %v", err)
	}
	if len(body) != 2732 {
		t.Fatalf("expected 2732 base64 chars, got %d", len(body))
	}

	decoded, err := base64.StdEncoding.DecodeString(string(body))
	if err != nil {
		t.Fatalf("invalid base64 response: %v", err)
	}
	if len(decoded) != connectTokenBytes {
		t.Fatalf("expected %d decoded bytes, got %d", connectTokenBytes, len(decoded))
	}
	if string(decoded[:13]) != "NETCODE 1.03\x00" {
		t.Fatalf("expected NETCODE 1.03 header, got %q", string(decoded[:13]))
	}
}
