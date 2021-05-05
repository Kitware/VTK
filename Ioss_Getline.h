
/*
 * Copyright (C) 1991, 1992, 1993, 2020 by Chris Thewalt (thewalt@ce.berkeley.edu)
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

#ifndef IOSS_GETLINE_H
#define IOSS_GETLINE_H

#include "vtk_ioss_mangle.h"

/* unix systems can #define POSIX to use termios, otherwise
 * the bsd or sysv interface will be used
 */

#define IO_GL_BUF_SIZE 1024

#ifdef __cplusplus
extern "C" {
#endif

typedef size_t (*io_gl_strwidth_proc)(char *);
typedef int (*io_gl_in_hook_proc)(char *);
typedef int (*io_gl_out_hook_proc)(char *);
typedef int (*io_gl_tab_hook_proc)(char *, int, int *, size_t);
typedef size_t (*io_gl_strlen_proc)(const char *);
typedef char *(*io_gl_tab_completion_proc)(const char *, int);

char *io_getline_int(const char *);        /* read a line of input */
void  io_gl_setwidth(int);                 /* specify width of screen */
void  io_gl_histadd(const char *);         /* adds entries to hist */
void  io_gl_strwidth(io_gl_strwidth_proc); /* to bind io_gl_strlen */
void  io_gl_tab_completion(io_gl_tab_completion_proc);
char *io_gl_local_filename_completion_proc(const char *, int);
void  io_gl_set_home_dir(const char *homedir);
void  io_gl_histsavefile(const char *const path);
void  io_gl_histloadfile(const char *const path);
char *io_gl_win_getpass(const char *const prompt, char *const pass, int dsize);

#ifndef _io_getline_c_

extern io_gl_in_hook_proc        io_gl_in_hook;
extern io_gl_out_hook_proc       io_gl_out_hook;
extern io_gl_tab_hook_proc       io_gl_tab_hook;
extern io_gl_strlen_proc         io_gl_strlen;
extern io_gl_tab_completion_proc io_gl_completion_proc;
extern int                       io_gl_filename_quoting_desired;
extern const char *              io_gl_filename_quote_characters;
extern int                       io_gl_ellipses_during_completion;
extern int                       io_gl_completion_exact_match_extra_char;
extern char                      io_gl_buf[IO_GL_BUF_SIZE];

#endif /* ! io_getline_c_ */

#ifdef __cplusplus
} /* close brackets on extern "C" declaration */
#endif

#endif /* IOSS_GETLINE_H */
