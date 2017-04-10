/*		 
 * Sux: Succinct data structures
 *
 * Copyright (C) 2007-2013 Sebastiano Vigna 
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as published by the Free
 *  Software Foundation; either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  This library is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <cassert>
#include <cstring>
#include "simple_rank.h"

#define LOG2_LONGWORDS_PER_ENTRY 5
#define LONGWORDS_PER_ENTRY (1 << LOG2_LONGWORDS_PER_ENTRY)

simple_rank::simple_rank() {}

simple_rank::simple_rank( const uint64_t * const bits, const uint64_t num_bits ) {
	this->bits = bits;
	int num_words = ( num_bits + 63 ) / 64;
	num_counts = num_words >> LOG2_LONGWORDS_PER_ENTRY;
	
	// Init rank structure
	counts = new uint64_t[ num_counts + 2 ]();

	uint64_t c = 0;
	uint64_t pos = 0;
	for( uint64_t i = 0; i < num_words; i += LONGWORDS_PER_ENTRY ) {
		counts[ pos++ ] = c;
		for( int j = 0;  j < LONGWORDS_PER_ENTRY; j++ ) {
			if ( i + j < num_words ) c += __builtin_popcountll( bits[ i + j ] );
		}
	}

	assert( pos < num_counts + 2 );
	counts[ pos ] = c;
	assert( c <= num_bits );
}

simple_rank::~simple_rank() {
	delete [] counts;
}


uint64_t simple_rank::rank( const uint64_t k ) {
	const uint64_t word = k / 64;
	const uint64_t block = word >> LOG2_LONGWORDS_PER_ENTRY;
	uint64_t c = counts[ block ];

	for( int i = block << LOG2_LONGWORDS_PER_ENTRY; i < word; i++ ) c += __builtin_popcountll( bits[ i ] );
	return c + __builtin_popcountll( bits[ word ] & ( 1ULL << k % 64 ) - 1 );
}

uint64_t simple_rank::bit_count() {
	return num_counts * 64;
}

void simple_rank::print_counts() {}
