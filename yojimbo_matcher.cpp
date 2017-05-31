/*
    Yojimbo Network Library.
    
    Copyright © 2016 - 2017, The Network Protocol Company, Inc.
*/

#include "yojimbo_config.h"
#include "yojimbo_matcher.h"
#include <mbedtls/config.h>
#include <mbedtls/platform.h>
#include <mbedtls/net.h>
#include <mbedtls/debug.h>
#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/error.h>
#include <mbedtls/certs.h>
#include <inttypes.h>
#include "netcode.h"

#define SERVER_PORT "8080"
#define SERVER_NAME "localhost"

namespace yojimbo
{
    struct MatcherInternal
    {
        mbedtls_net_context server_fd;
        mbedtls_entropy_context entropy;
        mbedtls_ctr_drbg_context ctr_drbg;
        mbedtls_ssl_context ssl;
        mbedtls_ssl_config conf;
        mbedtls_x509_crt cacert;
    };

    Matcher::Matcher( Allocator & allocator )
    {
        yojimbo_assert( ConnectTokenBytes == NETCODE_CONNECT_TOKEN_BYTES );
        m_allocator = &allocator;
        m_initialized = false;
        m_matchStatus = MATCH_IDLE;
        m_internal = YOJIMBO_NEW( allocator, MatcherInternal );
    }

    Matcher::~Matcher()
    {
        mbedtls_net_free( &m_internal->server_fd );
        mbedtls_x509_crt_free( &m_internal->cacert );
        mbedtls_ssl_free( &m_internal->ssl );
        mbedtls_ssl_config_free( &m_internal->conf );
        mbedtls_ctr_drbg_free( &m_internal->ctr_drbg );
        mbedtls_entropy_free( &m_internal->entropy );
        YOJIMBO_DELETE( *m_allocator, MatcherInternal, m_internal );
    }

    bool Matcher::Initialize()
    {
        const char * pers = "yojimbo_client";

        mbedtls_net_init( &m_internal->server_fd );
        mbedtls_ssl_init( &m_internal->ssl );
        mbedtls_ssl_config_init( &m_internal->conf );
        mbedtls_x509_crt_init( &m_internal->cacert );
        mbedtls_ctr_drbg_init( &m_internal->ctr_drbg );
        mbedtls_entropy_init( &m_internal->entropy );

        int result;

        if ( ( result = mbedtls_ctr_drbg_seed( &m_internal->ctr_drbg, mbedtls_entropy_func, &m_internal->entropy, (const unsigned char *) pers, strlen( pers ) ) ) != 0 )
        {
            yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: mbedtls_ctr_drbg_seed failed (%d)\n", result );
            return false;
        }

        if ( mbedtls_x509_crt_parse( &m_internal->cacert, (const unsigned char *) mbedtls_test_cas_pem, mbedtls_test_cas_pem_len ) < 0 )
        {
            yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: mbedtls_x509_crt_parse failed (%d)\n", result );
            return false;
        }

        m_initialized = true;

        memset( m_connectToken, 0, sizeof( m_connectToken ) );

        return true;
    }

    void Matcher::RequestMatch( uint64_t protocolId, uint64_t clientId )
    {
        yojimbo_assert( m_initialized );

        char request[1024];
        int bytesRead = 0;

        int result;

        if ( ( result = mbedtls_net_connect( &m_internal->server_fd, SERVER_NAME, SERVER_PORT, MBEDTLS_NET_PROTO_TCP ) ) != 0 )
        {
            yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: mbedtls_net_connect failed (%d)\n", result );
            m_matchStatus = MATCH_FAILED;
            goto cleanup;
        }

        if ( ( result = mbedtls_ssl_config_defaults( &m_internal->conf,
                        MBEDTLS_SSL_IS_CLIENT,
                        MBEDTLS_SSL_TRANSPORT_STREAM,
                        MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )
        {
            yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: mbedtls_net_connect failed (%d)\n", result );
            m_matchStatus = MATCH_FAILED;
            goto cleanup;
        }

        mbedtls_ssl_conf_authmode( &m_internal->conf, MBEDTLS_SSL_VERIFY_OPTIONAL );
        mbedtls_ssl_conf_ca_chain( &m_internal->conf, &m_internal->cacert, NULL );
        mbedtls_ssl_conf_rng( &m_internal->conf, mbedtls_ctr_drbg_random, &m_internal->ctr_drbg );

        if ( ( result = mbedtls_ssl_setup( &m_internal->ssl, &m_internal->conf ) ) != 0 )
        {
            yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: mbedtls_ssl_setup failed (%d)\n", result );
            m_matchStatus = MATCH_FAILED;
            goto cleanup;
        }

        if ( ( result = mbedtls_ssl_set_hostname( &m_internal->ssl, "yojimbo" ) ) != 0 )
        {
            yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: mbedtls_ssl_set_hostname failed (%d)\n", result );
            m_matchStatus = MATCH_FAILED;
            goto cleanup;
        }

        mbedtls_ssl_set_bio( &m_internal->ssl, &m_internal->server_fd, mbedtls_net_send, mbedtls_net_recv, NULL );

        while ( ( result = mbedtls_ssl_handshake( &m_internal->ssl ) ) != 0 )
        {
            if ( result != MBEDTLS_ERR_SSL_WANT_READ && result != MBEDTLS_ERR_SSL_WANT_WRITE )
            {
                yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: mbedtls_ssl_handshake failed (%d)\n", result );
                m_matchStatus = MATCH_FAILED;
                goto cleanup;
            }
        }

        // todo: you want to turn this on for release
        /*
        uint32_t flags;
        if ( ( flags = mbedtls_ssl_get_verify_result( &m_internal->ssl ) ) != 0 )
        {
            // IMPORTANT: In secure mode you must use a valid certificate, not a self signed one!
            debug_printf( "mbedtls_ssl_get_verify_result failed - flags = %x\n", flags );
            m_matchStatus = MATCH_FAILED;
            goto cleanup;
        }
        */

        sprintf( request, "GET /match/%" PRIu64 "/%" PRIu64 " HTTP/1.0\r\n\r\n", protocolId, clientId );

        yojimbo_printf( YOJIMBO_LOG_LEVEL_DEBUG, "match request:\n" );
        yojimbo_printf( YOJIMBO_LOG_LEVEL_DEBUG, "%s\n", request );

        while ( ( result = mbedtls_ssl_write( &m_internal->ssl, (uint8_t*) request, strlen( request ) ) ) <= 0 )
        {
            if ( result != MBEDTLS_ERR_SSL_WANT_READ && result != MBEDTLS_ERR_SSL_WANT_WRITE )
            {
                yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: mbedtls_ssl_write failed (%d)\n", result );
                m_matchStatus = MATCH_FAILED;
                goto cleanup;
            }
        }

        char buffer[2*ConnectTokenBytes];

        memset( buffer, 0, sizeof( buffer ) );

        do
        {
            result = mbedtls_ssl_read( &m_internal->ssl, (uint8_t*) ( buffer + bytesRead ), sizeof( buffer ) - bytesRead - 1 );

            if ( result == MBEDTLS_ERR_SSL_WANT_READ || result == MBEDTLS_ERR_SSL_WANT_WRITE )
                continue;

            if ( result == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY )
                break;

            if ( result <= 0 )
                break;

            bytesRead += result;
        }
        while( 1 );

        yojimbo_assert( bytesRead <= (int) sizeof( buffer ) );

        printf( "bytes read = %d\n", bytesRead );

        // todo: grab the base64 and decode it
        /*
        json = strstr( (const char*)buf, "\r\n\r\n" );

        if ( json && ParseMatchResponse( json, m_matchResponse ) )
        {
            m_matchStatus = MATCH_READY;
        }
        else
        {
            yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "failed to parse match response json:\n%s\n", json );
            m_matchStatus = MATCH_FAILED;
        }
        */

    cleanup:

        mbedtls_ssl_close_notify( &m_internal->ssl );
    }

    MatchStatus Matcher::GetMatchStatus()
    {
        return m_matchStatus;
    }

    void Matcher::GetConnectToken( uint8_t * connectToken )
    {
        yojimbo_assert( connectToken );
        yojimbo_assert( m_matchStatus == MATCH_READY );
        if ( m_matchStatus == MATCH_READY )
        {
            memcpy( connectToken, m_connectToken, ConnectTokenBytes );
        }
    }
}
