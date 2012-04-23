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
/* This procedure is to provide two dummy entries in the list of
 * projections that can be used when developing a new projection
 * without recompiling the projection list until the new projection
 * is developed and vetted.  The new projection should be called "dummy1"
 * or "dummy2" during this test period and simply linked in with
 * with "make lproj T=PJ_new.o" exection.
 * 
 * If these projections are selected they will return a failed,
 * null P value.
*/
#define PROJ_PARMS__ \
  double  temp;
#define PROJ_LIB__
#include  <lib_proj.h>
PROJ_HEAD(dummy, "Dummy projection") "\n\tfor development purposes";
FREEUP; if (P) free(P); }
ENTRY0(dummy)
  E_ERROR_0;
ENDENTRY(0)
/*
** Log: proj_dummy.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
