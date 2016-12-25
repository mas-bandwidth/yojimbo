/*
     Yojimbo Client/Server Network Protocol Library.

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

#ifndef YOJIMBO_SEQUENCE_BUFFER_H
#define YOJIMBO_SEQUENCE_BUFFER_H

#include "yojimbo_allocator.h"

namespace yojimbo
{
    // todo: document this file

    template <typename T> class SequenceBuffer
    {
    public:

        SequenceBuffer( Allocator & allocator, int size )
        {
            assert( size > 0 );
            m_size = size;
            m_sequence = 0;
            m_allocator = &allocator;
            m_entry_sequence = (uint32_t*) YOJIMBO_ALLOCATE( allocator, sizeof( uint32_t ) * size );
            m_entries = (T*) YOJIMBO_ALLOCATE( allocator, sizeof(T) * size );
            Reset();
        }

        ~SequenceBuffer()
        {
            assert( m_allocator );

            YOJIMBO_FREE( *m_allocator, m_entries );
            YOJIMBO_FREE( *m_allocator, m_entry_sequence );

            m_allocator = NULL;
        }

        void Reset()
        {
            m_sequence = 0;
            memset( m_entry_sequence, 0xFF, sizeof( uint32_t ) * m_size );
        }

        T * Insert( uint16_t sequence )
        {
            if ( sequence_greater_than( sequence + 1, m_sequence ) )
            {
                RemoveEntries( m_sequence, sequence );

                m_sequence = sequence + 1;
            }
            else if ( sequence_less_than( sequence, m_sequence - m_size ) )
            {
                return NULL;
            }

            const int index = sequence % m_size;

            m_entry_sequence[index] = sequence;

            return &m_entries[index];
        }

        void Remove( uint16_t sequence )
        {
            m_entry_sequence[ sequence % m_size ] = 0xFFFFFFFF;
        }

        bool Available( uint16_t sequence ) const
        {
            return m_entry_sequence[ sequence % m_size ] == 0xFFFFFFFF;
        }

        bool Exists( uint16_t sequence ) const
        {
            return m_entry_sequence[ sequence % m_size ] == uint32_t( sequence );
        }

        const T * Find( uint16_t sequence ) const
        {
            const int index = sequence % m_size;
            if ( m_entry_sequence[index] == uint32_t( sequence ) )
                return &m_entries[index];
            else
                return NULL;
        }

        T * Find( uint16_t sequence )
        {
            const int index = sequence % m_size;
            if ( m_entry_sequence[index] == uint32_t( sequence ) )
                return &m_entries[index];
            else
                return NULL;
        }

        T * GetAtIndex( int index )
        {
            assert( index >= 0 );
            assert( index < m_size );
            return m_entry_sequence[index] != 0xFFFFFFFF ? &m_entries[index] : NULL;
        }

        uint16_t GetSequence() const 
        {
            return m_sequence;
        }

        int GetIndex( uint16_t sequence ) const
        {
            return sequence % m_size;
        }

        int GetSize() const
        {
            return m_size;
        }

    protected:

        void RemoveEntries( int start_sequence, int finish_sequence )
        {
            if ( finish_sequence < start_sequence ) 
                finish_sequence += 65535;
            for ( int sequence = start_sequence; sequence <= finish_sequence; ++sequence )
                m_entry_sequence[sequence % m_size] = 0xFFFFFFFF;
        }

    private:

        Allocator * m_allocator;
        T * m_entries;
        uint32_t * m_entry_sequence;
        int m_size;
        uint16_t m_sequence;

        SequenceBuffer( const SequenceBuffer<T> & other );

        SequenceBuffer<T> & operator = ( const SequenceBuffer<T> & other );
    };

    template <typename T> void GenerateAckBits( const SequenceBuffer<T> & packets, 
                                                uint16_t & ack,
                                                uint32_t & ack_bits )
    {
        ack = packets.GetSequence() - 1;
        ack_bits = 0;
        uint32_t mask = 1;
        for ( int i = 0; i < 32; ++i )
        {
            uint16_t sequence = ack - i;
            if ( packets.Exists( sequence ) )
                ack_bits |= mask;
            mask <<= 1;
        }
    }
}

#endif
