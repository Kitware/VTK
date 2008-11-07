/*
** libproj -- library of cartographic projections
**
** Copyright (c) 2003, 2006   Gerald I. Evenden
*/
static const char
LIBPROJ_ID[] = "Id";
/*
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "proj_config.h"
#if PROJ_HAVE_PTHREADS
#include <pthread.h>
/* this makes libproj4 thread friendly */
  static pthread_once_t
key_once = PTHREAD_ONCE_INIT;
  static pthread_key_t
key;
  static void
destroy(void *buf) { free(buf); }
  static void
create(void) { pthread_key_create(&key, destroy); }

PROJ_EXPORT int *
proj_errno_loc(void) {
  void *p;

  pthread_once(&key_once, create);
  if (!(p = pthread_getspecific(key))) {
    p = (void *)malloc(sizeof(int));
    pthread_setspecific(key, p);
  }
  return (int *)p;
}
#else
/* threads unfriendly */
  static int
proj_errno;

PROJ_EXPORT int *
proj_errno_loc(void) { return &proj_errno; }
#endif
/*
** Log: proj_errno.c
** Revision 3.2  2006/01/15 15:11:20  gie
** include pthread.h added
**
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
