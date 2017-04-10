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

const int REPEATS = 100 * 1000 * 1000;

#define ONES_STEP_4 ( 0x1111111111111111ULL )
#define ONES_STEP_8 ( 0x0101010101010101ULL )

const unsigned char popcount[] = {
0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,
};

long long getusertime() {
	struct rusage rusage;
	getrusage( 0, &rusage );
	return rusage.ru_utime.tv_sec * 1000000L + rusage.ru_utime.tv_usec;
}

__inline int count( const uint64_t x ) {
	uint64_t byte_sums = x - ( ( x & 0xa * ONES_STEP_4 ) >> 1 );
	byte_sums = ( byte_sums & 0x3333333333333333ULL ) + ( ( byte_sums >> 2 ) & 3 * ONES_STEP_4 );
	byte_sums = ( byte_sums + ( byte_sums >> 4 ) ) & 0x0f * ONES_STEP_8;
	return byte_sums * ONES_STEP_8 >> 56;
}

__inline int count_no_mul( const uint64_t x ) {
	uint64_t byte_sums = x - ( ( x & 0xa * ONES_STEP_4 ) >> 1 );
	byte_sums = ( byte_sums & 0x3333333333333333ULL ) + ( ( byte_sums >> 2 ) & 3 * ONES_STEP_4 );
	byte_sums = ( byte_sums + ( byte_sums >> 4 ) ) & 0x0f * ONES_STEP_8;
	byte_sums = byte_sums + (byte_sums >> 8);
	byte_sums = byte_sums + (byte_sums >> 16);
	return ( byte_sums + (byte_sums >> 32) ) & 0x7f;
}

__inline int count_popcount( uint64_t x ) {
	int c = 0;
	for( int i = 8; i-- != 0; ) {
			c += popcount[ x & 0xFF ];
			x >>= 8;
	}
	return c;
}

__inline int count_popcount_unrolled( const uint64_t x ) {
	return popcount[ x & 0xFF ] + popcount[ x >> 8 & 0xFF ] + popcount[ x >> 16 & 0xFF ] + popcount[ x >> 24 & 0xFF ] +
		popcount[ x >> 32 & 0xFF ] + popcount[ x >> 40 & 0xFF ] + popcount[ x >> 48 & 0xFF ] + popcount[ x >> 56 & 0xFF ];
}


int main( int argc, char *argv[] ) {
	assert( sizeof(int) == 4 );
	assert( sizeof(long long) == 8 );

	uint64_t dummy = 0xAAAAAAAAAAAAAAAAULL; // Just to keep the compiler from excising code.
	long long start, elapsed;
	double s;

	start = getusertime();

	for( int k = REPEATS; k-- != 0; ) dummy ^= __builtin_popcountll( dummy );

	elapsed = getusertime() - start;
	s = elapsed / 1E6;
	printf( "%f s, %f ranks/s, %f ns/count [extended instruction set]\n", s, REPEATS / s, 1E9 * s / REPEATS );

	start = getusertime();

	for( int k = REPEATS; k-- != 0; ) dummy ^= count( dummy );

	elapsed = getusertime() - start;
	s = elapsed / 1E6;
	printf( "%f s, %f ranks/s, %f ns/count [broadword]\n", s, REPEATS / s, 1E9 * s / REPEATS );

	start = getusertime();

	for( int k = REPEATS; k-- != 0; ) dummy ^= count_no_mul( dummy );

	elapsed = getusertime() - start;
	s = elapsed / 1E6;
	printf( "%f s, %f ranks/s, %f ns/count [no mul]\n", s, REPEATS / s, 1E9 * s / REPEATS );

	start = getusertime();

	for( int k = REPEATS; k-- != 0; ) dummy ^= count_popcount( dummy );

	elapsed = getusertime() - start;
	s = elapsed / 1E6;
	printf( "%f s, %f pranks/s, %f ns/rank [popcount]\n", s, REPEATS / s, 1E9 * s / REPEATS );

	start = getusertime();

	for( int k = REPEATS; k-- != 0; ) dummy ^= count_popcount_unrolled( dummy );

	elapsed = getusertime() - start;
	s = elapsed / 1E6;
	printf( "%f s, %f puranks/s, %f ns/rank [popcount, unrolled]\n", s, REPEATS / s, 1E9 * s / REPEATS );
	if ( !dummy ) putchar( 0 ); // To avoid excision

	return 0;
}
