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
/*
** Convert DMS string to radians
*/
#include <lib_proj.h>
#include <string.h>
#include <ctype.h>

/* following should be sufficient for all but the rediculous */
#define MAX_WORK 64
  static const char
*sym = "NnEeSsWw";
  static const double
vm[] = {
  .0174532925199433,
  .0002908882086657216,
  .0000048481368110953599
};
  double
proj_dmstor(const char *is, char **rs) {
  int sign, n, nl;
  char *p, *s, work[MAX_WORK];
  double v, tv;

  if (rs)
    *rs = (char *)is;
  /* copy sting into work space */
  while (isspace(sign = *is)) ++is;
  for (n = MAX_WORK, s = work, p = (char *)is; isgraph(*p) && --n ; )
    *s++ = *p++;
  *s = '\0';
  /* it is possible that a really odd input (like lots of leading
    zeros) could be truncated in copying into work.  But ... */
  sign = *(s = work);
  if (sign == '+' || sign == '-') s++;
  else sign = '+';
  for (v = 0., nl = 0 ; nl < 3 ; nl = n + 1 ) {
    if (!(isdigit(*s) || *s == '.')) break;
    if ((tv = strtod(s, &s)) == HUGE_VAL)
      return tv;
    switch (*s) {
    case 'D': case 'd':
      n = 0; break;
    case '\'':
      n = 1; break;
    case '"':
      n = 2; break;
    case 'r': case 'R':
      if (nl) {
        proj_errno = -16;
        return HUGE_VAL;
      }
      ++s;
      v = tv;
      goto skip;
    default:
      v += tv * vm[nl];
    skip:  n = 4;
      continue;
    }
    if (n < nl) {
      proj_errno = -16;
      return HUGE_VAL;
    }
    v += tv * vm[n];
    ++s;
  }
    /* postfix sign */
  if (*s && (p = strchr(sym, *s))) {
    sign = (p - sym) >= 4 ? '-' : '+';
    ++s;
  }
  if (sign == '-')
    v = -v;
  if (rs) /* return point of next char after valid string */
    *rs = (char *)is + (s - work);
  return v;
}
/*
** Log: proj_dmstor.c
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
