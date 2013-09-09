/*
 * Copyright (C) 2011 UChicago Argonne, LLC
 * All Rights Reserved
 *
 * Permission to use, reproduce, prepare derivative works, and to redistribute
 * to others this software, derivatives of this software, and future versions
 * of this software as well as its documentation is hereby granted, provided
 * that this notice is retained thereon and on all copies or modifications.
 * This permission is perpetual, world-wide, and provided on a royalty-free
 * basis. UChicago Argonne, LLC and all other contributors make no
 * representations as to the suitability and operability of this software for
 * any purpose. It is provided "as is" without express or implied warranty. 
 *
 * Portions of this software are copyright by UChicago Argonne, LLC. Argonne
 * National Laboratory with facilities in the state of Illinois, is owned by
 * The United States Government, and operated by UChicago Argonne, LLC under
 * provision of a contract with the Department of Energy. 
 *
 * PORTIONS OF THIS SOFTWARE  WERE PREPARED AS AN ACCOUNT OF WORK SPONSORED BY
 * AN AGENCY OF THE UNITED STATES GOVERNMENT. NEITHER THE UNITED STATES
 * GOVERNMENT NOR ANY AGENCY THEREOF, NOR THE UNIVERSITY OF CHICAGO, NOR ANY OF
 * THEIR EMPLOYEES OR OFFICERS, MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR
 * ASSUMES ANY LEGAL LIABILITY OR RESPONSIBILITY FOR THE ACCURACY,
 * COMPLETENESS, OR USEFULNESS OF ANY INFORMATION, APPARATUS, PRODUCT, OR
 * PROCESS DISCLOSED, OR REPRESENTS THAT ITS USE WOULD NOT INFRINGE PRIVATELY
 * OWNED RIGHTS. REFERENCE HEREIN TO ANY SPECIFIC COMMERCIAL PRODUCT, PROCESS,
 * OR SERVICE BY TRADE NAME, TRADEMARK, MANUFACTURER, OR OTHERWISE, DOES NOT
 * NECESSARILY CONSTITUTE OR IMPLY ITS ENDORSEMENT, RECOMMENDATION, OR FAVORING
 * BY THE UNITED STATES GOVERNMENT OR ANY AGENCY THEREOF. THE VIEW AND OPINIONS
 * OF AUTHORS EXPRESSED HEREIN DO NOT NECESSARILY STATE OR REFLECT THOSE OF THE
 * UNITED STATES GOVERNMENT OR ANY AGENCY THEREOF. 
 *
 * Author: Hal Finkel <hfinkel@anl.gov>
 */

#include "bigchunk.h"
#include <stdio.h>

static void *_bigchunk_ptr = (void *) 0;
static size_t _bigchunk_last_alloc = (size_t) -1;
static size_t _bigchunk_sz = 0;
static size_t _bigchunk_used = 0;
static size_t _bigchunk_total = 0;
static const size_t min_alloc = 32; /* for alignment; must be 2^n */

void *bigchunk_malloc(size_t sz)
{
	if (sz < min_alloc)
		sz = min_alloc;
	else {
		size_t e = sz - (sz & ~(min_alloc-1));
		if (e != 0) sz += min_alloc - e;
	}

	if (_bigchunk_sz - _bigchunk_used >= sz) {
		/* this fits in the big chunk */
		char *myBigChunkPtr = reinterpret_cast<char*>(_bigchunk_ptr);
		void *r = myBigChunkPtr + _bigchunk_used;
		_bigchunk_last_alloc = _bigchunk_used;
		_bigchunk_used += sz;
		_bigchunk_total += sz;
		return r;
	} else if (_bigchunk_used == 0 && _bigchunk_sz > 0) {
		/* this is smaller than the big chunk, but nothing
		   is currently using the big chunk, so just make
		   the big chunk bigger.
		*/

		void *new_chuck = realloc(_bigchunk_ptr, sz);
		if (new_chuck) {
			_bigchunk_ptr = new_chuck;
			_bigchunk_last_alloc = 0;
			_bigchunk_sz = sz;
			_bigchunk_used = sz;
			_bigchunk_total += sz;
			return _bigchunk_ptr;
		}
        }

	if (_bigchunk_sz > 0)
		fprintf(stderr, "WARNING: bigchunk: allocation of %zu bytes has been requested, only %zu of %zu remain!\n",
				sz, _bigchunk_sz - _bigchunk_used, _bigchunk_sz);

	void *ptr = malloc(sz);
	if (ptr) _bigchunk_total += sz;
	return ptr;
}

void bigchunk_free(void *ptr)
{
	// Cast to char* so that we can do pointer arithmetic
	char *myPtr 		= reinterpret_cast<char*>(ptr);
	char *myBigChunkPtr = reinterpret_cast<char*>(_bigchunk_ptr);

	if (myPtr < myBigChunkPtr || myPtr >= myBigChunkPtr + _bigchunk_sz) {
		free(ptr);
	} else if (_bigchunk_last_alloc != (size_t) -1 &&
             myPtr == myBigChunkPtr + _bigchunk_last_alloc) {
		/* this is the last allocation, so we can undo that easily... */
		_bigchunk_used = _bigchunk_last_alloc;
		_bigchunk_last_alloc = (size_t) -1;
	}
}

void bigchunk_reset()
{
	_bigchunk_used = 0;
	_bigchunk_total = 0;
	_bigchunk_last_alloc = (size_t) -1;
}

void bigchunk_init(size_t sz)
{
	_bigchunk_ptr = malloc(sz);
	if (_bigchunk_ptr) {
		_bigchunk_sz = sz;
		_bigchunk_used = 0;
		_bigchunk_last_alloc = (size_t) -1;
	}
}

void bigchunk_cleanup()
{
	free(_bigchunk_ptr);
	_bigchunk_ptr = 0;
	_bigchunk_sz = 0;
	_bigchunk_used = 0;
	_bigchunk_total = 0;
	_bigchunk_last_alloc = (size_t) -1;
}

size_t bigchunk_get_size()
{
	return _bigchunk_sz;
}


size_t bigchunk_get_total()
{
	return _bigchunk_total;
}

size_t bigchunk_get_used()
{
	return _bigchunk_used;
}

