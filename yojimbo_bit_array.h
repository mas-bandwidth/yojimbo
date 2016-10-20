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

#ifndef YOJIMBO_BIT_ARRAY_H
#define YOJIMBO_BIT_ARRAY_H

#include "yojimbo_allocator.h"

namespace yojimbo
{
    class BitArray
    {
    public:

        BitArray( Allocator & allocator, int size )
        {
            assert( size > 0 );
            m_allocator = &allocator;
            m_size = size;
            m_bytes = 8 * ( ( size / 64 ) + ( ( size % 64 ) ? 1 : 0 ) );
            assert( m_bytes > 0 );
            m_data = (uint64_t*) allocator.Allocate( m_bytes );
            Clear();
        }

        ~BitArray()
        {
            assert( m_data );
            assert( m_allocator );
            m_allocator->Free( m_data );
            m_allocator = NULL;
            m_data = NULL;
        }

        void Clear()
        {
            assert( m_data );
            memset( m_data, 0, m_bytes );
        }

        void SetBit( int index )
        {
            assert( index >= 0 );
            assert( index < m_size );
            const int data_index = index >> 6;
            const int bit_index = index & ( (1<<6) - 1 );
            assert( bit_index >= 0 );
            assert( bit_index < 64 );
            m_data[data_index] |= uint64_t(1) << bit_index;
        }

        void ClearBit( int index )
        {
            assert( index >= 0 );
            assert( index < m_size );
            const int data_index = index >> 6;
            const int bit_index = index & ( (1<<6) - 1 );
            m_data[data_index] &= ~( uint64_t(1) << bit_index );
        }

        uint64_t GetBit( int index ) const
        {
            assert( index >= 0 );
            assert( index < m_size );
            const int data_index = index >> 6;
            const int bit_index = index & ( (1<<6) - 1 );
            assert( bit_index >= 0 );
            assert( bit_index < 64 );
            return ( m_data[data_index] >> bit_index ) & 1;
        }

        int GetSize() const
        {
            return m_size;
        }

    private:

        Allocator * m_allocator;

        int m_size;
        int m_bytes;
        uint64_t * m_data;

        BitArray( const BitArray & other );
        BitArray & operator = ( const BitArray & other );
    };
}

#endif
