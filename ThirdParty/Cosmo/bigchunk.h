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

#ifndef BIGCHUNK_H
#define BIGCHUNK_H

#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
#include <new>
extern "C" {
#endif

/*
 * Allocates memory from the big chunck, falling back to the system's allocator.
 */

void *bigchunk_malloc(size_t sz);

/*
 * Frees memory, this does nothing if the memory comes from the big chunk.
 */

void bigchunk_free(void *ptr);

/*
 * Resets the state of the big chunk (marks all memory in the chunk as free).
 */

void bigchunk_reset();

/*
 * Initialize the big chunk (to the specified size).
 */

void bigchunk_init(size_t sz);

/*
 * Free the big chunk (all memory within it should already be free).
 */

void bigchunk_cleanup();

/*
 * Get the size of the big chunk.
 */

size_t bigchunk_get_size();

/*
 * Get the total amount of memory allocated
 * (freed memory is not subtracted, so this measures the total of all allocation
 * requests in between calls to bigchunk_reset().
 */

size_t bigchunk_get_total();

/*
 * Get the amount of the big chunk used.
 */

size_t bigchunk_get_used();

#ifdef __cplusplus
}

namespace cosmologytools {


template <typename T>
class bigchunk_allocator
{
public:
  typedef T value_type;
  typedef T *pointer;
  typedef T &reference;
  typedef const T *const_pointer;
  typedef const T &const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  template <typename U>
  struct rebind {
  	typedef bigchunk_allocator<U> other;
  };

public:
  bigchunk_allocator() throw() {};
  bigchunk_allocator(const bigchunk_allocator&) throw() {};

  template <typename U>
  bigchunk_allocator(const bigchunk_allocator<U>&) throw() {};

public:
  ~bigchunk_allocator() throw () {};

public:
  pointer address(reference x) const { return &x; }
  const_pointer address (const_reference x) const { return &x; }

  size_type max_size() const throw() { return size_t(-1) / sizeof(T); }

  void construct(pointer p, const_reference val) { ::new ((void*)p) T(val); }
  void destroy(pointer p) { ((T*)p)->~T(); }

public:
  pointer allocate(size_type n,
                   const void * /*hint*/ = 0)
  {
    pointer p = (pointer) ::bigchunk_malloc(n*sizeof(T));
    if (p) return p;
    throw std::bad_alloc();
  }

  void deallocate(pointer p, size_type n)
  {
    ::bigchunk_free((void *) p);
  }
};

}
#endif // __cplusplus
#endif // BIGCHUNK_H

