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

using namespace std;

#define __STDC_LIMIT_MACROS 1
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "rank9sel.h"
#include "rank9b.h"
#include "jacobson.h"
#include "elias_fano.h"
#include "simple_select.h"
#include "simple_rank.h"
#include "simple_select_half.h"
#include "posrep.h"

static uint64_t s[ 16 ] = {
	0xAAAAAAAAAAAAAAAAULL, 0xAAAAAAAAAAAAAAAAULL, 0xAAAAAAAAAAAAAAAAULL, 0xAAAAAAAAAAAAAAAAULL, 
	0xAAAAAAAAAAAAAAAAULL, 0xAAAAAAAAAAAAAAAAULL, 0xAAAAAAAAAAAAAAAAULL, 0xAAAAAAAAAAAAAAAAULL, 
	0xAAAAAAAAAAAAAAAAULL, 0xAAAAAAAAAAAAAAAAULL, 0xAAAAAAAAAAAAAAAAULL, 0xAAAAAAAAAAAAAAAAULL, 
	0xAAAAAAAAAAAAAAAAULL, 0xAAAAAAAAAAAAAAAAULL, 0xAAAAAAAAAAAAAAAAULL, 0xAAAAAAAAAAAAAAAAULL
};

static uint64_t __inline xrand(void) {
    static int p;
    uint64_t s0 = s[ p ];
    uint64_t s1 = s[ p = ( p + 1 ) & 15 ];
    s1 ^= s1 << 31; // a
    s1 ^= s1 >> 11; // b
    s0 ^= s0 >> 30; // c
    return ( s[ p ] = s0 ^ s1 ) * 1181783497276652981LL;
}

uint64_t getusertime() {
	struct rusage rusage;
	getrusage( 0, &rusage );
	return rusage.ru_utime.tv_sec * 1000000ULL + rusage.ru_utime.tv_usec;
}

int main( int argc, char *argv[] ) {
	assert( argc >= 3 );
	assert( sizeof(int) == 4 );

	if ( argc < 3 ) {
		fprintf( stderr, "Usage: %s NUMBITS DENSITY0 [DENSITY1]\n", argv[ 0 ] );
		return 0;
	}

	const int64_t num_bits = strtoll( argv[ 1 ], NULL, 0 );
	printf( "Number of bits: %lld\n", num_bits );
	printf( "Number of words: %lld\n", num_bits / 64 );
	printf( "Number of blocks: %lld\n", ( num_bits / 64 ) / 16 );
	uint64_t * const bits = (uint64_t *)calloc( num_bits / 64 + 1, sizeof *bits );

	double density0 = atof( argv[ 2 ] ), density1 = argc > 3 ? atof( argv[ 3 ] ) : density0;
	assert( density0 >= 0 );
	assert( density0 <= 1 );
	assert( density1 >= 0 );
	assert( density1 <= 1 );

	// Init array with given density
	const uint64_t threshold0 = (uint64_t)((UINT64_MAX) * density0), threshold1 = (uint64_t)((UINT64_MAX) * density1);

	uint64_t num_ones_first_half = 0, num_ones_second_half = 0;

	for( int64_t i = 0; i < num_bits / 2; i++ ) if ( xrand() < threshold0 ) { num_ones_first_half++; bits[ i / 64 ] |= 1LL << i % 64; }
	for( int64_t i = num_bits / 2; i < num_bits; i++ ) if ( xrand() < threshold1 ) { num_ones_second_half++; bits[ i / 64 ] |= 1LL << i % 64; }

#ifdef DEBUG
	printf("First words: %016llx %016llx %016llx %016llx\n", bits[ 0 ], bits[ 1 ], bits[ 2 ], bits[ 3 ] );
#endif

#ifdef MAX_LOG2_LONGWORDS_PER_SUBINVENTORY
	CLASS rs( bits, num_bits, MAX_LOG2_LONGWORDS_PER_SUBINVENTORY );
#else
	CLASS rs( bits, num_bits );
#endif

	int64_t dummy = 0x12345678; // Just to keep the compiler from excising code.

	// Cache random positions
	uint64_t * const position = (uint64_t *)calloc( POSITIONS, sizeof *position );
	assert( num_bits != 0 );

	int64_t start, elapsed;
	double s;

	printf( "Bit cost: %lld (%.2f%%)\n", rs.bit_count(), (rs.bit_count()*100.0)/num_bits);

#ifndef NORANKTEST

	for( int i = POSITIONS; i-- != 0; ) position[ i ] = xrand() % num_bits;

	start = getusertime();

	for( int k = REPEATS; k-- != 0; )
		for( int i = 0; i < POSITIONS; i++ )
			dummy ^= rs.rank( position[ i ] );


	elapsed = getusertime() - start;
	s = elapsed / 1E6;
	printf( "%f s, %f ranks/s, %f ns/rank\n", s, (REPEATS * POSITIONS) / s, 1E9 * s / (REPEATS * POSITIONS) );
#endif

#ifndef NOSELECTTEST

	if ( num_ones_first_half && num_ones_second_half  ) {
		for( int64_t i = POSITIONS; i-- != 0; ) position[ i ] = ( i & 1 ) ? xrand() % num_ones_first_half : num_ones_first_half + xrand() % num_ones_second_half;

		start = getusertime();

		for( int k = REPEATS; k-- != 0; )
			for( int i = 0; i < POSITIONS; i++ )
				dummy ^= rs.select( position[ i ] );

		elapsed = getusertime() - start;
		s = elapsed / 1E6;
		printf( "%f s, %f selects/s, %f ns/select\n", s, (REPEATS * POSITIONS) / s, 1E9 * s / (REPEATS * POSITIONS) );
	}
	else printf( "Too few ones to measure select speed\n" );
#endif

	rs.print_counts();
	if ( !dummy ) putchar(0); // To avoid excision

	return 0;
}
