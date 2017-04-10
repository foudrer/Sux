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
#include <math.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "bal_paren.h"

static uint32_t rnd_curr = 1;

extern long long far_find_close;

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

__inline static int count( const uint64_t x ) {
	uint64_t byte_sums = x - ( ( x & 0xa * ONES_STEP_4 ) >> 1 );
	byte_sums = ( byte_sums & 3 * ONES_STEP_4 ) + ( ( byte_sums >> 2 ) & 3 * ONES_STEP_4 );
	byte_sums = ( byte_sums + ( byte_sums >> 4 ) ) & 0x0f * ONES_STEP_8;
	return byte_sums * ONES_STEP_8 >> 56;
}


uint64_t getusertime() {
	struct rusage rusage;
	getrusage( 0, &rusage );
	return rusage.ru_utime.tv_sec * 1000000ULL + ( rusage.ru_utime.tv_usec / 1000 ) * 1000;
}

__inline static void set( uint64_t * const bits, const uint64_t pos ) {
	bits[ pos / 64 ] |= 1ULL << pos % 64;
}

void fill_paren_rec( uint64_t *bits, uint64_t start, uint64_t end, uint64_t mod ) {
	set( bits, start );
	if ( start + 2 == end ) return;

	const uint64_t offset = ( xrand() % ( ( end - start - 2 ) / 2 ) ) * 2;
	if ( offset == 0 ) {
		fill_paren_rec( bits, start + 1, end - 1, mod );
		return;
	}

	fill_paren_rec( bits, start + 1, start + 1 + offset, mod );
	fill_paren_rec( bits, start + 1 + offset, end - 1, mod );
}

void fill_paren( uint64_t *bits, uint64_t num_bits, double twist ) {
	bits[ 0 ] = 1; // First open parenthesis
	for( int i = 1, r = 0; i < num_bits - 1; i++ ) {
		const double coeff = r * ( num_bits - 1 - i + r + 2 ) / ( 2. * ( num_bits - 1 - i ) * ( r + 1 ) );
		assert( coeff >= 0 );
		assert( coeff <= 1 );

		if ( xrand() >= UINT64_MAX * ( coeff != 1 ? twist * coeff: 1 ) ) {
			bits[ i / 64 ] |= 1ULL << i % 64;
			r++;
		}
		else r--;
		assert(r >= 0);
	} 
}

int main( int argc, char *argv[] ) {
	assert( argc >= 2 );
	assert( sizeof(int) == 4 );
	assert( sizeof(long long) == 8 );

	if ( argc < 2 ) {
		fprintf( stderr, "Usage: %s NUMBITS [TWIST]\n", argv[ 0 ] );
		return 0;
	}

	const long long num_bits = strtoll( argv[ 1 ], NULL, 0 );
	printf( "Number of bits: %lld\n", num_bits );
	printf( "Number of words: %lld\n", num_bits / 64 );
	printf( "Number of blocks: %lld\n", ( num_bits / 64 ) / 16 );
	uint64_t * const bits = (uint64_t *)calloc( num_bits / 64 + 1, sizeof *bits );

	double twist = argc > 2 ? atof( argv[ 2 ] ) : 1;
	assert( twist >= 0 );
	assert( twist <= 1 );

	// Init array with given twist
	const uint64_t mod = (uint64_t)(RAND_MAX * twist);
	fill_paren( bits, num_bits, twist );

#ifdef DEBUG
	printf("First words: %016llx %016llx %016llx %016llx\n", bits[ 0 ], bits[ 1 ], bits[ 2 ], bits[ 3 ] );
	for( int i = 0; i < num_bits; i++ ) printf( "%c", ( bits[ i / 64 ] & 1ULL << i % 64 ) ? '(' : ')' );
	printf("\n");
#endif

#ifndef NDEBUG
	fprintf( stderr, "Checking string...\n" );
	uint64_t c = 0;
	for( uint64_t i = 0; i < num_bits; i++ ) {
		if ( bits[ i / 64 ] & 1ULL << i % 64 ) c++;
		else c--;
		assert(c > 0 || i == num_bits - 1);
	}
	assert(c == 0);
#endif

#ifdef DEBUG
	printf("First words: %016llx %016llx %016llx %016llx\n", bits[ 0 ], bits[ 1 ], bits[ 2 ], bits[ 3 ] );
#endif

	bal_paren bp( bits, num_bits );
#ifndef NDEBUG
	fprintf( stderr, "Completed structure.\n" );
#endif
	// Estimate average distance
	// uint64_t d = 0;
	// for( uint64_t i = 0;  i < num_bits; i++ ) if ( bits[ i / 64 ] & 1L << i % 64 ) d += bp.find_close( i ) - i;

	long long dummy = 0x12345678; // Just to keep the compiler from excising code.

	// Cache random positions
	uint64_t * const position = (uint64_t *)calloc( POSITIONS, sizeof *position );
	assert( num_bits != 0 );
	for( int i = POSITIONS; i-- != 0; ) {
		const bool far = xrand() & 1;
		do {
			position[ i ] = xrand() % num_bits;
		} while( ( bits[ position[ i ] / 64 ] & 1ULL << ( position[ i ] % 64 ) ) == 0 || ( far && bp.find_close( position[ i ] ) / 64 == position[ i ] / 64 ) );
	}

	long long start, elapsed;
	double s;

	start = getusertime();

	for( int k = REPEATS; k-- != 0; ) 
		for( int i = 0; i < POSITIONS; i ++ ) 
			dummy ^= bp.find_close( position[ i ] );

	elapsed = getusertime() - start;
	s = elapsed / 1E6;
	printf( "%f s, %.02f finds/s, %.02f ns/find\n", s, (REPEATS * POSITIONS) / s, 1E9 * s / (REPEATS * POSITIONS) );
	printf( "Far find close: %lld (%.02f%%)\n", far_find_close, ( far_find_close * 100.0 ) / ( REPEATS * POSITIONS ) );

	//printf( "Average distance: %d\n", d / ( num_bits / 2 ) );

	bp.print_counts();
	if ( dummy == 42 ) printf( "42" ); // To avoid excision

	return 0;
}
