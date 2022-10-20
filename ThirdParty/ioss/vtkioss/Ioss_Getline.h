
/*
 * Copyright (C) 1991, 1992, 1993, 2022 by Chris Thewalt (thewalt@ce.berkeley.edu)
 *
 * Permission to use, copy, modify, and distribute this software
 * for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  This software is provided "as is" without express or
 * implied warranty.
 *
 * Thanks to the following people who have provided enhancements and fixes:
 *   Ron Ueberschaer, Christoph Keller, Scott Schwartz, Steven List,
 *   DaviD W. Sanderson, Goran Bostrom, Michael Gleason, Glenn Kasten,
 *   Edin Hodzic, Eric J Bivona, Kai Uwe Rommel, Danny Quah, Ulrich Betzler
 */

#pragma once

#include "ioss_export.h"

#ifdef __cplusplus
extern "C" {
#endif

IOSS_EXPORT char *io_getline_int(const char *); /* read a line of input */
IOSS_EXPORT void  io_gl_setwidth(int);          /* specify width of screen */
IOSS_EXPORT void  io_gl_histadd(const char *);  /* adds entries to hist */

#ifdef __cplusplus
} /* close brackets on extern "C" declaration */
#endif
