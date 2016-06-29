/*
    Yojimbo Client/Server Network Library.
    
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

#include "yojimbo_matcher.h"

#include "mbedtls/config.h"
#include "mbedtls/platform.h"
#include "mbedtls/net.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

#define SERVER_PORT "8080"
#define SERVER_NAME "localhost"
#define GET_REQUEST "GET /match/123141/1 HTTP/1.0\r\n\r\n"

namespace yojimbo
{
    struct MatcherImpl
    {
        mbedtls_net_context server_fd;
        mbedtls_entropy_context entropy;
        mbedtls_ctr_drbg_context ctr_drbg;
        mbedtls_ssl_context ssl;
        mbedtls_ssl_config conf;
        mbedtls_x509_crt cacert;
    };

    Matcher::Matcher()
    {
        m_initialized = false;
        m_status = MATCHER_IDLE;
        m_impl = new MatcherImpl();     // todo: convert to allocator
    }

    Matcher::~Matcher()
    {
        mbedtls_net_free( &m_impl->server_fd );
        mbedtls_x509_crt_free( &m_impl->cacert );
        mbedtls_ssl_free( &m_impl->ssl );
        mbedtls_ssl_config_free( &m_impl->conf );
        mbedtls_ctr_drbg_free( &m_impl->ctr_drbg );
        mbedtls_entropy_free( &m_impl->entropy );

        delete m_impl;
        m_impl = NULL;
    }

    bool Matcher::Initialize()
    {
        int ret;

        const char *pers = "yojimbo_client";

        mbedtls_net_init( &m_impl->server_fd );
        mbedtls_ssl_init( &m_impl->ssl );
        mbedtls_ssl_config_init( &m_impl->conf );
        mbedtls_x509_crt_init( &m_impl->cacert );
        mbedtls_ctr_drbg_init( &m_impl->ctr_drbg );
        mbedtls_entropy_init( &m_impl->entropy );

        if ( ( ret = mbedtls_ctr_drbg_seed( &m_impl->ctr_drbg, mbedtls_entropy_func, &m_impl->entropy, (const unsigned char *) pers, strlen( pers ) ) ) != 0 )
            return false;

        ret = mbedtls_x509_crt_parse( &m_impl->cacert, (const unsigned char *) mbedtls_test_cas_pem, mbedtls_test_cas_pem_len );
        if ( ret < 0 )
            return false;

        m_initialized = true;

        return true;
    }

    void Matcher::RequestMatch( uint32_t protocolId, uint64_t clientId )
    {
        assert( m_initialized );

        (void)protocolId;
        (void)clientId;
        
        int ret;

        if ( ( ret = mbedtls_net_connect( &m_impl->server_fd, SERVER_NAME, SERVER_PORT, MBEDTLS_NET_PROTO_TCP ) ) != 0 )
        {
            m_status = MATCHER_FAILED;
            return;
        }

        if ( ( ret = mbedtls_ssl_config_defaults( &m_impl->conf,
                        MBEDTLS_SSL_IS_CLIENT,
                        MBEDTLS_SSL_TRANSPORT_STREAM,
                        MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )
        {
            m_status = MATCHER_FAILED;
            return;
        }

        mbedtls_ssl_conf_authmode( &m_impl->conf, MBEDTLS_SSL_VERIFY_OPTIONAL );
        mbedtls_ssl_conf_ca_chain( &m_impl->conf, &m_impl->cacert, NULL );
        mbedtls_ssl_conf_rng( &m_impl->conf, mbedtls_ctr_drbg_random, &m_impl->ctr_drbg );

        if( ( ret = mbedtls_ssl_setup( &m_impl->ssl, &m_impl->conf ) ) != 0 )
        {
            m_status = MATCHER_FAILED;
            return;
        }

        if ( ( ret = mbedtls_ssl_set_hostname( &m_impl->ssl, "mbed TLS Server 1" ) ) != 0 )
        {
            m_status = MATCHER_FAILED;
            return;
        }

        mbedtls_ssl_set_bio( &m_impl->ssl, &m_impl->server_fd, mbedtls_net_send, mbedtls_net_recv, NULL );

        while ( ( ret = mbedtls_ssl_handshake( &m_impl->ssl ) ) != 0 )
        {
            if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
            {
                m_status = MATCHER_FAILED;
                return;
            }
        }

        uint32_t flags;
        if ( ( flags = mbedtls_ssl_get_verify_result( &m_impl->ssl ) ) != 0 )
        {
            // note: could not verify certificate (eg. it is self-signed)
        }

        unsigned char buf[4*1024];

        int len = sprintf( (char *) buf, GET_REQUEST );

        while ( ( ret = mbedtls_ssl_write( &m_impl->ssl, buf, len ) ) <= 0 )
        {
            if ( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
            {
                m_status = MATCHER_FAILED;
                return;
            }
        }

        len = ret;

        do
        {
            len = sizeof( buf ) - 1;
            memset( buf, 0, sizeof( buf ) );
            ret = mbedtls_ssl_read( &m_impl->ssl, buf, len );

            if ( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE )
                continue;

            if ( ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY )
                break;

            if ( ret <= 0 )
                break;

            len = ret;

            const char * json = strstr( (const char*)buf, "\r\n\r\n" ) + 4;

            if ( !json )
                break;

            printf( "\n%s\n", json );

            m_status = MATCHER_READY;
        }
        while( 1 );

        mbedtls_ssl_close_notify( &m_impl->ssl );

        m_status = MATCHER_FAILED;
    }

    MatcherStatus Matcher::GetStatus()
    {
        return m_status;
    }

    void Matcher::GetMatch( Match & match )
    {
        match = Match();
    }
}
