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

#include <cstdio>
#include <ctime>
#include <cassert>
#include <cstdlib>
#include <climits>
#include <stdint.h>
#include <sys/time.h>
#include <sys/resource.h>
//#include <x86intrin.h>
#include "popcount.h"
#include "select.h"
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

long long getusertime() {
	struct rusage rusage;
	getrusage( 0, &rusage );
	return rusage.ru_utime.tv_sec * 1000000L + rusage.ru_utime.tv_usec;
}

__inline int select_in_word_bw( const uint64_t x, const int k ) {
	// Phase 1: sums by byte
	uint64_t byte_sums = x - ( x >> 1 & 0x5ULL * ONES_STEP_4 );
	byte_sums = ( byte_sums & 3ULL * ONES_STEP_4 ) + ( ( byte_sums >> 2 ) & 3ULL * ONES_STEP_4 );
	byte_sums = ( byte_sums + ( byte_sums >> 4 ) ) & 0x0fULL * ONES_STEP_8;
	byte_sums *= ONES_STEP_8;

	// Phase 2: compare each byte sum with k
	const uint64_t k_step_8 = k * ONES_STEP_8;
	// Giuseppe Ottaviano's improvement
	const int place = __builtin_popcountll( EASY_LEQ_STEP_8_MSBS( byte_sums, k_step_8 ) ) * 8;

	// Phase 3: Locate the relevant byte and look up the result in select_in_byte
	return place + select_in_byte[ x >> place & 0xFFULL | k - ( ( byte_sums << 8 ) >> place & 0xFFULL ) << 8 ];
}

// Borrowed from Philip Pronin's code for Facebook's folly library.
__inline int select_in_word_ctzll( uint64_t x, int rank ) {
#ifndef NDEBUG
	int result = select_in_word_bw( x, rank );
#endif
	const int half_count = __builtin_popcountll(x);
	if( rank >= half_count ) {
		rank -= half_count;
		x &= -1ULL << 32;
	}

	//for( int i = rank; i-- != 0; ) x = __blsr_u64( x );
	for( int i = rank; i-- != 0; ) x &= x - 1;
	assert( result ==  __builtin_ctzll( x ) );
	return __builtin_ctzll( x );
}

__inline int select_in_word_popcount( const uint64_t x, const int k ) {
#ifndef NDEBUG
	int result = select_in_word_bw( x, k );
#endif

	for( int i = 0, c = k; i < 64; i+=8 )
		if ( ( c -= popcount[ x >> i & 0xFF ] ) < 0 ) {
			c += popcount[ x >> i & 0xFF ];
			assert( result == i + select_in_byte[ x >> i & 0xFF | c << 8 ] );
			return i + select_in_byte[ x >> i & 0xFF | c << 8 ];
		}
	return -1;
}


__inline int select_gog_petri( uint64_t x, int i ) {
#ifndef NDEBUG
	int result = select_in_word_bw( x, i );
#endif

	uint64_t s = x, b;
	s = s-((s>>1) & 0x5555555555555555ULL);
	s = (s & 0x3333333333333333ULL) + ((s >> 2) & 0x3333333333333333ULL);
	s = (s + (s >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
	s = 0x0101010101010101ULL*s;
// now s contains 8 bytes s[7],...,s[0], s[i] contains the cumulative sum
// of (i+1)*8 least significant bits of s
	b = (s+overflow[i]) & 0x8080808080808080ULL;
// overflow contains a bit mask x consisting of 8 bytes
// x[7],...,x[0] and x[i] is set to 128-i
// => a byte b[i] in b is >= 128 if cum sum >= i

// __builtin_ctzll returns the number of trailing zeros, if b!=0
	int byte_nr = __builtin_ctzll(b) >> 3;   // byte nr in [0..7]
	s <<= 8;
	i -= (s >> (byte_nr<<3)) & 0xFFULL;

	assert( result == (byte_nr << 3) + select_in_byte[ (i << 8) + ((x>>(byte_nr<<3))&0xFFULL) ] );

	return (byte_nr << 3) + select_in_byte[(i << 8) + ((x>>(byte_nr<<3))&0xFFULL) ];
}

__inline int select_gog_petri2( const uint64_t x, const int i ) {
#ifndef NDEBUG
	int result = select_in_word_bw( x, i );
#endif

	// Phase 1: sums by byte
	uint64_t byte_sums = x - ( x >> 1 & 0x5ULL * ONES_STEP_4 );
	byte_sums = ( byte_sums & 3ULL * ONES_STEP_4 ) + ( ( byte_sums >> 2 ) & 3ULL * ONES_STEP_4 );
	byte_sums = ( byte_sums + ( byte_sums >> 4 ) ) & 0x0fULL * ONES_STEP_8;
	byte_sums *= ONES_STEP_8;

	// Phase 2: find the right byte shift
	const int place = ( __builtin_ctzll( byte_sums + overflow[ i ] & 0x8080808080808080ULL ) >> 3 ) << 3; 

	// Phase 3: Locate the relevant byte and look up the result in select_in_byte

	assert( result == place + select_in_byte[ x >> place & 0xFFULL | ( i - ( ( byte_sums << 8 ) >> place & 0xFF ) ) << 8 ] );

	return place + select_in_byte[ x >> place & 0xFFULL | ( i - ( ( byte_sums << 8 ) >> place & 0xFF ) ) << 8 ];
}

int main( int argc, char *argv[] ) {
	assert( sizeof(int) == 4 );
	assert( sizeof(long long) == 8 );

	long long start, elapsed;
	double s;
	int dummy = 0; // Just to keep the compiler from excising code.

	uint64_t * const position = (uint64_t *)calloc( POSITIONS / 10, sizeof *position );
	for( int i = POSITIONS / 10; i-- != 0; ) while( __builtin_popcountll( position[ i ] = xrand() ) < 32 ); // This shouldn't happen often
	
	start = getusertime();

	for( int k = 10 * REPEATS; k-- != 0; ) {
		for( int i = POSITIONS / 10; i-- != 0; ) {
			const uint64_t w = position[ i ];
			dummy ^= select_in_word_bw( w, 0 );
			dummy ^= select_in_word_bw( w, 3 );
			dummy ^= select_in_word_bw( w, 6 );
			dummy ^= select_in_word_bw( w, 9 );
			dummy ^= select_in_word_bw( w, 12 );
			dummy ^= select_in_word_bw( w, 15 );
			dummy ^= select_in_word_bw( w, 18 );
			dummy ^= select_in_word_bw( w, 21 );
			dummy ^= select_in_word_bw( w, 24 );
			dummy ^= select_in_word_bw( w, 27 );
		}
	}

	elapsed = getusertime() - start;
	s = elapsed / 1E6;
	printf( "%f s, %f selects/s, %f ns/select [broadword]\n", s, (10 * REPEATS * POSITIONS) / s, 1E9 * s / (10 * REPEATS * POSITIONS) );

	start = getusertime();

	for( int k = 10 * REPEATS; k-- != 0; ) {
		for( int i = POSITIONS / 10; i-- != 0; ) {
			const uint64_t w = position[ i ];
			dummy ^= select_gog_petri( w, 0 );
			dummy ^= select_gog_petri( w, 3 );
			dummy ^= select_gog_petri( w, 6 );
			dummy ^= select_gog_petri( w, 9 );
			dummy ^= select_gog_petri( w, 12 );
			dummy ^= select_gog_petri( w, 15 );
			dummy ^= select_gog_petri( w, 18 );
			dummy ^= select_gog_petri( w, 21 );
			dummy ^= select_gog_petri( w, 24 );
			dummy ^= select_gog_petri( w, 27 );
		}
	}

	elapsed = getusertime() - start;
	s = elapsed / 1E6;
	printf( "%f s, %f selects/s, %f ns/select [Gog & Petri]\n", s, (10 * REPEATS * POSITIONS) / s, 1E9 * s / (10 * REPEATS * POSITIONS) );

	start = getusertime();

	for( int k = 10 * REPEATS; k-- != 0; ) {
		for( int i = POSITIONS / 10; i-- != 0; ) {
			const uint64_t w = position[ i ];
			dummy ^= select_gog_petri2( w, 0 );
			dummy ^= select_gog_petri2( w, 3 );
			dummy ^= select_gog_petri2( w, 6 );
			dummy ^= select_gog_petri2( w, 9 );
			dummy ^= select_gog_petri2( w, 12 );
			dummy ^= select_gog_petri2( w, 15 );
			dummy ^= select_gog_petri2( w, 18 );
			dummy ^= select_gog_petri2( w, 21 );
			dummy ^= select_gog_petri2( w, 24 );
			dummy ^= select_gog_petri2( w, 27 );
		}
	}

	elapsed = getusertime() - start;
	s = elapsed / 1E6;
	printf( "%f s, %f selects/s, %f ns/select [Gog & Petri 2]\n", s, (10 * REPEATS * POSITIONS) / s, 1E9 * s / (10 * REPEATS * POSITIONS) );

	start = getusertime();

	for( int k = 10 * REPEATS; k-- != 0; ) {
		for( int i = POSITIONS / 10; i-- != 0; ) {
			const uint64_t w = position[ i ];
			dummy ^= select_in_word_ctzll( w, 0 );
			dummy ^= select_in_word_ctzll( w, 3 );
			dummy ^= select_in_word_ctzll( w, 6 );
			dummy ^= select_in_word_ctzll( w, 9 );
			dummy ^= select_in_word_ctzll( w, 12 );
			dummy ^= select_in_word_ctzll( w, 15 );
			dummy ^= select_in_word_ctzll( w, 18 );
			dummy ^= select_in_word_ctzll( w, 21 );
			dummy ^= select_in_word_ctzll( w, 24 );
			dummy ^= select_in_word_ctzll( w, 27 );
		}
	}

	elapsed = getusertime() - start;
	s = elapsed / 1E6;
	printf( "%f s, %f selects/s, %f ns/select [prunin]\n", s, (10 * REPEATS * POSITIONS) / s, 1E9 * s / (10 * REPEATS * POSITIONS) );

	start = getusertime();

	for( int k = 10 * REPEATS; k-- != 0; ) {
		for( int i = POSITIONS / 10; i-- != 0; ) {
			const uint64_t w = position[ i ];
			dummy ^= select_in_word_popcount( w, 0 );
			dummy ^= select_in_word_popcount( w, 3 );
			dummy ^= select_in_word_popcount( w, 6 );
			dummy ^= select_in_word_popcount( w, 9 );
			dummy ^= select_in_word_popcount( w, 12 );
			dummy ^= select_in_word_popcount( w, 15 );
			dummy ^= select_in_word_popcount( w, 18 );
			dummy ^= select_in_word_popcount( w, 21 );
			dummy ^= select_in_word_popcount( w, 24 );
			dummy ^= select_in_word_popcount( w, 27 );
		}
	}

	elapsed = getusertime() - start;
	s = elapsed / 1E6;
	printf( "%f s, %f selects/s, %f ns/select [popcount]\n", s, (10 * REPEATS * POSITIONS) / s, 1E9 * s / (10 * REPEATS * POSITIONS) );

	if (!dummy) putchar(0); // To avoid excision

	return 0;
}
