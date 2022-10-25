
/*
 * Copyright (C) 1991, 1992, 1993, 2021, 2022 by Chris Thewalt (thewalt@ce.berkeley.edu)
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

/*
 * Note:  This version has been updated by Mike Gleason <mgleason@ncftp.com>
 */

#include "vtk_ioss_mangle.h"

#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32) || defined(_MSC_VER) ||                \
    defined(__MINGW32__) || defined(_WIN64)

#define __windows__ 1
#include <conio.h>
#include <io.h>
#define NOMINMAX
#include <windows.h>
#define sleep(a) Sleep(a * 1000)
#ifndef write
#define write _write
#define read  _read
#endif
#else

#ifndef __unix__
#define __unix__ 1
#endif

#include <termios.h>
struct termios new_termios, old_termios;
#endif

/********************* C library headers ********************************/
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include "Ioss_Getline.h"

#define IO_GL_BUF_SIZE 1024

/******************** external interface *********************************/

int         io_gl_filename_quoting_desired   = -1; /* default to unspecified */
const char *io_gl_filename_quote_characters  = " \t*?<>|;&()[]$`";
int         io_gl_ellipses_during_completion = 1;
char        io_gl_buf[IO_GL_BUF_SIZE]; /* input buffer */

/******************** internal interface *********************************/

static int         io_gl_init_done = -1;               /* terminal mode flag  */
static int         io_gl_termw     = 80;               /* actual terminal width */
static int         io_gl_scroll    = 27;               /* width of EOL scrolling region */
static int         io_gl_width     = 0;                /* net size available for input */
static int         io_gl_extent    = 0;                /* how far to redraw, 0 means all */
static int         io_gl_overwrite = 0;                /* overwrite mode */
static int         io_gl_pos, io_gl_cnt = 0;           /* position and size of input */
static char        io_gl_killbuf[IO_GL_BUF_SIZE] = ""; /* killed text */
static const char *io_gl_prompt;                       /* to save the prompt string */
static int         io_gl_search_mode = 0;              /* search mode flag */

static void io_gl_init(void);         /* prepare to edit a line */
static void io_gl_cleanup(void);      /* to undo io_gl_init */
static void io_gl_char_init(void);    /* get ready for no echo input */
static void io_gl_char_cleanup(void); /* undo io_gl_char_init */
                                      /* returns printable prompt width */

static void io_gl_addchar(int c);               /* install specified char */
static void io_gl_del(int loc, int);            /* del, either left (-1) or cur (0) */
static void io_gl_error(const char *const buf); /* write error msg and die */
static void io_gl_fixup(const char *prompt, int change,
                        int cursor);           /* fixup state variables and screen */
static int  io_gl_getc(void);                  /* read one char from terminal */
static void io_gl_kill(int pos);               /* delete to EOL */
static void io_gl_newline(void);               /* handle \n or \r */
static void io_gl_putc(int c);                 /* write one char to terminal */
static void io_gl_puts(const char *const buf); /* write a line to terminal */
static void io_gl_redraw(void);                /* issue \n and redraw all */
static void io_gl_transpose(void);             /* transpose two chars */
static void io_gl_yank(void);                  /* yank killed text */

static void  hist_init(void);          /* initializes hist pointers */
static char *hist_next(void);          /* return ptr to next item */
static char *hist_prev(void);          /* return ptr to prev item */
static char *hist_save(const char *p); /* makes copy of a string, without NL */

static void search_addchar(int c);       /* increment search string */
static void search_term(void);           /* reset with current contents */
static void search_back(int new_search); /* look back for current string */
static void search_forw(int new_search); /* look forw for current string */
static void io_gl_beep(void);            /* try to play a system beep sound */

static char *copy_string(char *dest, char const *source, long int elements)
{
  char *d;
  for (d = dest; d + 1 < dest + elements && *source; d++, source++) {
    *d = *source;
  }
  *d = '\0';
  return d;
}

/************************ nonportable part *********************************/

#ifdef MSDOS
#include <bios.h>
#endif

static void io_gl_char_init(void) /* turn off input echo */
{
#ifdef __unix__
  tcgetattr(0, &old_termios);
  new_termios = old_termios;
  new_termios.c_iflag &= ~(BRKINT | ISTRIP | IXON | IXOFF);
  new_termios.c_iflag |= (IGNBRK | IGNPAR);
  new_termios.c_lflag &= ~(ICANON | ISIG | IEXTEN | ECHO);
  new_termios.c_cc[VMIN]  = 1;
  new_termios.c_cc[VTIME] = 0;
  tcsetattr(0, TCSANOW, &new_termios);
#endif /* __unix__ */
}

static void io_gl_char_cleanup(void) /* undo effects of io_gl_char_init */
{
#ifdef __unix__
  tcsetattr(0, TCSANOW, &old_termios);
#endif /* __unix__ */
}

#if defined(MSDOS) || defined(__windows__)

#define K_UP     0x48
#define K_DOWN   0x50
#define K_LEFT   0x4B
#define K_RIGHT  0x4D
#define K_DELETE 0x53
#define K_INSERT 0x52
#define K_HOME   0x47
#define K_END    0x4F
#define K_PGUP   0x49
#define K_PGDN   0x51

int pc_keymap(int c)
{
  switch (c) {
  case K_UP:
  case K_PGUP:
    c = 16; /* up -> ^P */
    break;
  case K_DOWN:
  case K_PGDN:
    c = 14; /* down -> ^N */
    break;
  case K_LEFT:
    c = 2; /* left -> ^B */
    break;
  case K_RIGHT:
    c = 6; /* right -> ^F */
    break;
  case K_END:
    c = 5; /* end -> ^E */
    break;
  case K_HOME:
    c = 1; /* home -> ^A */
    break;
  case K_INSERT:
    c = 15; /* insert -> ^O */
    break;
  case K_DELETE:
    c = 4; /* del -> ^D */
    break;
  default: c = 0; /* make it garbage */
  }
  return c;
}
#endif /* defined(MSDOS) || defined(__windows__) */

static int io_gl_getc(void)
/* get a character without echoing it to screen */
{
#ifdef __unix__
  char ch;
  int  c;
  while ((c = read(0, &ch, 1)) == -1) {
    if (errno != EINTR) {
      break;
    }
  }
  c = (ch <= 0) ? -1 : ch;
#endif /* __unix__ */
#ifdef MSDOS
  int c = _bios_keybrd(_NKEYBRD_READ);
  if ((c & 0377) == 224) {
    c = pc_keymap((c >> 8) & 0377);
  }
  else {
    c &= 0377;
  }
#endif /* MSDOS */
#ifdef __windows__
  int c = (int)_getch();
  if ((c == 0) || (c == 0xE0)) {
    /* Read key code */
    c = (int)_getch();
    c = pc_keymap(c);
  }
  else if (c == '\r') {
    /* Note: we only get \r from the console,
     * and not a matching \n.
     */
    c = '\n';
  }
#endif
  return c;
}

static void io_gl_putc(int c)
{
  char ch = (char)(unsigned char)c;

  write(1, &ch, 1);
  if (ch == '\n') {
    ch = '\r';
    write(1, &ch, 1); /* RAW mode needs '\r', does not hurt */
  }
}

/******************** fairly portable part *********************************/

static void io_gl_puts(const char *const buf)
{
  if (buf) {
    int len = strlen(buf);
    write(1, buf, len);
  }
}

static void io_gl_error(const char *const buf)
{
  int len = strlen(buf);

  io_gl_cleanup();
  write(2, buf, len);
  exit(1);
}

static void io_gl_init(void)
/* set up variables and terminal */
{
  if (io_gl_init_done < 0) { /* -1 only on startup */
    const char *cp = (const char *)getenv("COLUMNS");
    if (cp != NULL) {
      int w = strtol(cp, NULL, 10);
      if (w > 20)
        io_gl_setwidth(w);
    }
    hist_init();
  }
  if (isatty(0) == 0 || isatty(1) == 0)
    io_gl_error("\n*** Error: getline(): not interactive, use stdio.\n");
  io_gl_char_init();
  io_gl_init_done = 1;
}

static void io_gl_cleanup(void)
/* undo effects of io_gl_init, as necessary */
{
  if (io_gl_init_done > 0)
    io_gl_char_cleanup();
  io_gl_init_done = 0;
#ifdef __windows__
  Sleep(40);
  FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
#endif
}

void io_gl_setwidth(int w)
{
  if (w > 250)
    w = 250;
  if (w > 20) {
    io_gl_termw  = w;
    io_gl_scroll = w / 3;
  }
  else {
    io_gl_error("\n*** Error: minimum screen width is 21\n");
  }
}

char *io_getline_int(const char *prompt)
{
  io_gl_init();
  io_gl_prompt = (prompt) ? prompt : "";
  io_gl_buf[0] = '\0';
  io_gl_fixup(io_gl_prompt, -2, IO_GL_BUF_SIZE);

#ifdef __windows__
  FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
#endif

  int c;
  while ((c = io_gl_getc()) >= 0) {
    io_gl_extent = 0; /* reset to full extent */
    if (isprint(c)) {
      if (io_gl_search_mode) {
        search_addchar(c);
      }
      else {
        io_gl_addchar(c);
      }
    }
    else {
      if (io_gl_search_mode) {
        if (c == '\033' || c == '\016' || c == '\020') {
          search_term();
          c = 0; /* ignore the character */
        }
        else if (c == '\010' || c == '\177') {
          search_addchar(-1); /* unwind search string */
          c = 0;
        }
        else if (c != '\022' && c != '\023') {
          search_term(); /* terminate and handle char */
        }
      }
      switch (c) {
      case '\n':
      case '\r': /* newline */
        io_gl_newline();
        io_gl_cleanup();
        return io_gl_buf;
      case '\001':
        io_gl_fixup(io_gl_prompt, -1, 0); /* ^A */
        break;
      case '\002':
        io_gl_fixup(io_gl_prompt, -1, io_gl_pos - 1); /* ^B */
        break;
      case '\004': /* ^D */
        if (io_gl_cnt == 0) {
          io_gl_buf[0] = '\0';
          io_gl_cleanup();
          io_gl_putc('\n');
          return io_gl_buf;
        }
        else {
          io_gl_del(0, 1);
        }
        break;
      case '\005':
        io_gl_fixup(io_gl_prompt, -1, io_gl_cnt); /* ^E */
        break;
      case '\006':
        io_gl_fixup(io_gl_prompt, -1, io_gl_pos + 1); /* ^F */
        break;
      case '\010':
      case '\177':
        io_gl_del(-1, 0); /* ^H and DEL */
        break;
      case '\t': /* TAB */ break;
      case '\013':
        io_gl_kill(io_gl_pos); /* ^K */
        break;
      case '\014':
        io_gl_redraw(); /* ^L */
        break;
      case '\016': /* ^N */
        copy_string(io_gl_buf, hist_next(), IO_GL_BUF_SIZE);
        io_gl_fixup(io_gl_prompt, 0, IO_GL_BUF_SIZE);
        break;
      case '\017':
        io_gl_overwrite = !io_gl_overwrite; /* ^O */
        break;
      case '\020': /* ^P */
        copy_string(io_gl_buf, hist_prev(), IO_GL_BUF_SIZE);
        io_gl_fixup(io_gl_prompt, 0, IO_GL_BUF_SIZE);
        break;
      case '\022':
        search_back(1); /* ^R */
        break;
      case '\023':
        search_forw(1); /* ^S */
        break;
      case '\024':
        io_gl_transpose(); /* ^T */
        break;
      case '\025':
        io_gl_kill(0); /* ^U */
        break;
      case '\031':
        io_gl_yank(); /* ^Y */
        break;
      default:
        if (c > 0)
          io_gl_beep();
        break;
      }
    }
  }
  io_gl_cleanup();
  io_gl_buf[0] = '\0';
  return io_gl_buf;
}

static void io_gl_addchar(int c)

/* adds the character c to the input buffer at current location */
{
  if (io_gl_cnt >= IO_GL_BUF_SIZE - 1)
    io_gl_error("\n*** Error: getline(): input buffer overflow\n");
  if (io_gl_overwrite == 0 || io_gl_pos == io_gl_cnt) {
    int i;
    for (i = io_gl_cnt; i >= io_gl_pos; i--)
      io_gl_buf[i + 1] = io_gl_buf[i];
    io_gl_buf[io_gl_pos] = (char)c;
    io_gl_fixup(io_gl_prompt, io_gl_pos, io_gl_pos + 1);
  }
  else {
    io_gl_buf[io_gl_pos] = (char)c;
    io_gl_extent         = 1;
    io_gl_fixup(io_gl_prompt, io_gl_pos, io_gl_pos + 1);
  }
}

static void io_gl_yank(void)
/* adds the kill buffer to the input buffer at current location */
{
  int len = strlen(io_gl_killbuf);
  if (len > 0) {
    if (io_gl_overwrite == 0) {
      if (io_gl_cnt + len >= IO_GL_BUF_SIZE - 1)
        io_gl_error("\n*** Error: getline(): input buffer overflow\n");
      for (int i = io_gl_cnt; i >= io_gl_pos; i--)
        io_gl_buf[i + len] = io_gl_buf[i];
      for (int i = 0; i < len; i++)
        io_gl_buf[io_gl_pos + i] = io_gl_killbuf[i];
      io_gl_fixup(io_gl_prompt, io_gl_pos, io_gl_pos + len);
    }
    else {
      if (io_gl_pos + len > io_gl_cnt) {
        if (io_gl_pos + len >= IO_GL_BUF_SIZE - 1)
          io_gl_error("\n*** Error: getline(): input buffer overflow\n");
        io_gl_buf[io_gl_pos + len] = '\0';
      }
      for (int i = 0; i < len; i++)
        io_gl_buf[io_gl_pos + i] = io_gl_killbuf[i];
      io_gl_extent = len;
      io_gl_fixup(io_gl_prompt, io_gl_pos, io_gl_pos + len);
    }
  }
  else
    io_gl_beep();
}

static void io_gl_transpose(void)
/* switch character under cursor and to left of cursor */
{
  if (io_gl_pos > 0 && io_gl_cnt > io_gl_pos) {
    int c                    = io_gl_buf[io_gl_pos - 1];
    io_gl_buf[io_gl_pos - 1] = io_gl_buf[io_gl_pos];
    io_gl_buf[io_gl_pos]     = (char)c;
    io_gl_extent             = 2;
    io_gl_fixup(io_gl_prompt, io_gl_pos - 1, io_gl_pos);
  }
  else
    io_gl_beep();
}

static void io_gl_newline(void)
/*
 * Cleans up entire line before returning to caller. A \n is appended.
 * If line longer than screen, we redraw starting at beginning
 */
{
  int change = io_gl_cnt;
  int len    = io_gl_cnt;
  int loc    = io_gl_width - 5; /* shifts line back to start position */

  if (io_gl_cnt >= IO_GL_BUF_SIZE - 1)
    io_gl_error("\n*** Error: getline(): input buffer overflow\n");
  if (loc > len)
    loc = len;
  io_gl_fixup(io_gl_prompt, change, loc); /* must do this before appending \n */
  io_gl_buf[len]     = '\n';
  io_gl_buf[len + 1] = '\0';
  io_gl_putc('\n');
}

static void io_gl_del(int loc, int killsave)

/*
 * Delete a character.  The loc variable can be:
 *    -1 : delete character to left of cursor
 *     0 : delete character under cursor
 */
{
  if ((loc == -1 && io_gl_pos > 0) || (loc == 0 && io_gl_pos < io_gl_cnt)) {
    int j = 0;
    for (int i = io_gl_pos + loc; i < io_gl_cnt; i++) {
      if ((j == 0) && (killsave != 0)) {
        io_gl_killbuf[0] = io_gl_buf[i];
        io_gl_killbuf[1] = '\0';
        j                = 1;
      }
      io_gl_buf[i] = io_gl_buf[i + 1];
    }
    io_gl_fixup(io_gl_prompt, io_gl_pos + loc, io_gl_pos + loc);
  }
  else
    io_gl_beep();
}

static void io_gl_kill(int pos)

/* delete from pos to the end of line */
{
  if (pos < io_gl_cnt) {
    copy_string(io_gl_killbuf, io_gl_buf + pos, IO_GL_BUF_SIZE);
    io_gl_buf[pos] = '\0';
    io_gl_fixup(io_gl_prompt, pos, pos);
  }
  else
    io_gl_beep();
}

static void io_gl_redraw(void)
/* emit a newline, reset and redraw prompt and current input line */
{
  if (io_gl_init_done > 0) {
    io_gl_putc('\n');
    io_gl_fixup(io_gl_prompt, -2, io_gl_pos);
  }
}

static void io_gl_fixup(const char *prompt, int change, int cursor)

/*
 * This function is used both for redrawing when input changes or for
 * moving within the input line.  The parameters are:
 *   prompt:  compared to last_prompt[] for changes;
 *   change : the index of the start of changes in the input buffer,
 *            with -1 indicating no changes, -2 indicating we're on
 *            a new line, redraw everything.
 *   cursor : the desired location of the cursor after the call.
 *            A value of IO_GL_BUF_SIZE can be used  to indicate the cursor should
 *            move just past the end of the input line.
 */
{
  static int  io_gl_shift; /* index of first on screen character */
  static int  off_right;   /* true if more text right of screen */
  static int  off_left;    /* true if more text left of screen */
  static char last_prompt[80] = "";
  int         left = 0, right = -1; /* bounds for redraw */
  int         new_right = -1;       /* alternate right bound, using io_gl_extent */

  if (change == -2) { /* reset */
    io_gl_pos = io_gl_cnt = io_gl_shift = off_right = off_left = 0;
    io_gl_putc('\r');
    io_gl_puts(prompt);
    copy_string(last_prompt, prompt, 80);
    change      = 0;
    io_gl_width = io_gl_termw - strlen(prompt);
  }
  else if (strcmp(prompt, last_prompt) != 0) {
    int l1    = strlen(last_prompt);
    int l2    = strlen(prompt);
    io_gl_cnt = io_gl_cnt + l1 - l2;
    copy_string(last_prompt, prompt, 80);
    io_gl_putc('\r');
    io_gl_puts(prompt);
    io_gl_pos   = io_gl_shift;
    io_gl_width = io_gl_termw - l2;
    change      = 0;
  }
  /* how much to erase at end of line */
  int pad    = (off_right) ? io_gl_width - 1 : io_gl_cnt - io_gl_shift; /* old length */
  int backup = io_gl_pos - io_gl_shift; /* how far to backup before fixing */
  if (change >= 0) {
    io_gl_cnt = strlen(io_gl_buf);
    if (change > io_gl_cnt)
      change = io_gl_cnt;
  }
  if (cursor > io_gl_cnt) {
    if (cursor != IO_GL_BUF_SIZE) { /* IO_GL_BUF_SIZE means end of line */
      if (io_gl_ellipses_during_completion == 0) {
        io_gl_beep();
      }
    }
    cursor = io_gl_cnt;
  }
  if (cursor < 0) {
    io_gl_beep();
    cursor = 0;
  }
  int extra = 0; /* adjusts when shift (scroll) happens */
  if (off_right || (off_left && cursor < io_gl_shift + io_gl_width - io_gl_scroll / 2)) {
    extra = 2; /* shift the scrolling boundary */
  }
  int new_shift = cursor + extra + io_gl_scroll - io_gl_width;
  if (new_shift > 0) {
    new_shift /= io_gl_scroll;
    new_shift *= io_gl_scroll;
  }
  else
    new_shift = 0;
  if (new_shift != io_gl_shift) { /* scroll occurs */
    io_gl_shift = new_shift;
    off_left    = (io_gl_shift) ? 1 : 0;
    off_right   = (io_gl_cnt > io_gl_shift + io_gl_width - 1) ? 1 : 0;
    left        = io_gl_shift;
    new_right = right = (off_right) ? io_gl_shift + io_gl_width - 2 : io_gl_cnt;
  }
  else if (change >= 0) { /* no scroll, but text changed */
    if (change < io_gl_shift + off_left) {
      left = io_gl_shift;
    }
    else {
      left   = change;
      backup = io_gl_pos - change;
    }
    off_right = (io_gl_cnt > io_gl_shift + io_gl_width - 1) ? 1 : 0;
    right     = (off_right) ? io_gl_shift + io_gl_width - 2 : io_gl_cnt;
    new_right = (io_gl_extent && (right > left + io_gl_extent)) ? left + io_gl_extent : right;
  }
  pad -= (off_right) ? io_gl_width - 1 : io_gl_cnt - io_gl_shift;
  pad = (pad < 0) ? 0 : pad;
  if (left <= right) { /* clean up screen */
    for (int i = 0; i < backup; i++)
      io_gl_putc('\b');
    if (left == io_gl_shift && off_left) {
      io_gl_putc('$');
      left++;
    }
    for (int i = left; i < new_right; i++)
      io_gl_putc(io_gl_buf[i]);
    io_gl_pos = new_right;
    if (off_right && new_right == right) {
      io_gl_putc('$');
      io_gl_pos++;
    }
    else {
      for (int i = 0; i < pad; i++) /* erase remains of prev line */
        io_gl_putc(' ');
      io_gl_pos += pad;
    }
  }
  int i = io_gl_pos - cursor; /* move to final cursor location */
  if (i > 0) {
    while (i--)
      io_gl_putc('\b');
  }
  else {
    for (int ii = io_gl_pos; ii < cursor; ii++)
      io_gl_putc(io_gl_buf[ii]);
  }
  io_gl_pos = cursor;
}

/******************* History stuff **************************************/

#ifndef HIST_SIZE
#define HIST_SIZE 100
#endif

static int   hist_pos = 0, hist_last = 0;
static char *hist_buf[HIST_SIZE];
static char  hist_empty_elem[2] = "";

static void hist_init(void)
{
  hist_buf[0] = hist_empty_elem;
  for (int i = 1; i < HIST_SIZE; i++)
    hist_buf[i] = NULL;
}

void io_gl_histadd(const char *buf)
{
  static char *prev = NULL;

  /* in case we call io_gl_histadd() before we call getline() */
  if (io_gl_init_done < 0) { /* -1 only on startup */
    hist_init();
    io_gl_init_done = 0;
  }
  const char *p = buf;
  while (*p == ' ' || *p == '\t' || *p == '\n')
    p++;
  if (*p) {
    int len = strlen(buf);
    if (strchr(p, '\n')) /* previously line already has NL stripped */
      len--;
    if ((prev == NULL) || ((int)strlen(prev) != len) || strncmp(prev, buf, (size_t)len) != 0) {
      hist_buf[hist_last] = hist_save(buf);
      prev                = hist_buf[hist_last];
      hist_last           = (hist_last + 1) % HIST_SIZE;
      if (hist_buf[hist_last] && *hist_buf[hist_last]) {
        free(hist_buf[hist_last]);
      }
      hist_buf[hist_last] = hist_empty_elem;
    }
  }
  hist_pos = hist_last;
}

static char *hist_prev(void)
/* loads previous hist entry into input buffer, sticks on first */
{
  char *p    = NULL;
  int   next = (hist_pos - 1 + HIST_SIZE) % HIST_SIZE;

  if (hist_buf[hist_pos] != NULL && next != hist_last) {
    hist_pos = next;
    p        = hist_buf[hist_pos];
  }
  if (p == NULL) {
    p = hist_empty_elem;
    io_gl_beep();
  }
  return p;
}

static char *hist_next(void)
/* loads next hist entry into input buffer, clears on last */
{
  char *p = NULL;

  if (hist_pos != hist_last) {
    hist_pos = (hist_pos + 1) % HIST_SIZE;
    p        = hist_buf[hist_pos];
  }
  if (p == NULL) {
    p = hist_empty_elem;
    io_gl_beep();
  }
  return p;
}

static char *hist_save(const char *p)

/* makes a copy of the string */
{
  char  *s   = NULL;
  size_t len = strlen(p);
  char  *nl  = strpbrk(p, "\n\r");

  if (nl) {
    if ((s = (char *)malloc(len)) != NULL) {
      copy_string(s, p, len);
      s[len - 1] = '\0';
    }
  }
  else {
    if ((s = (char *)malloc(len + 1)) != NULL) {
      copy_string(s, p, len + 1);
    }
  }
  if (s == NULL)
    io_gl_error("\n*** Error: hist_save() failed on malloc\n");
  return s;
}

/******************* Search stuff **************************************/

static char search_prompt[101]; /* prompt includes search string */
static char search_string[100];
static int  search_pos      = 0; /* current location in search_string */
static int  search_forw_flg = 0; /* search direction flag */
static int  search_last     = 0; /* last match found */

static void search_update(int c)
{
  if (c == 0) {
    search_pos       = 0;
    search_string[0] = '\0';
    search_prompt[0] = '?';
    search_prompt[1] = ' ';
    search_prompt[2] = '\0';
  }
  else if (c > 0) {
    search_string[search_pos]     = (char)c;
    search_string[search_pos + 1] = (char)0;
    search_prompt[search_pos]     = (char)c;
    search_prompt[search_pos + 1] = (char)'?';
    search_prompt[search_pos + 2] = (char)' ';
    search_prompt[search_pos + 3] = (char)0;
    search_pos++;
  }
  else {
    if (search_pos > 0) {
      search_pos--;
      search_string[search_pos]     = (char)0;
      search_prompt[search_pos]     = (char)'?';
      search_prompt[search_pos + 1] = (char)' ';
      search_prompt[search_pos + 2] = (char)0;
    }
    else {
      io_gl_beep();
      hist_pos = hist_last;
    }
  }
}

static void search_addchar(int c)
{
  search_update(c);
  if (c < 0) {
    if (search_pos > 0) {
      hist_pos = search_last;
    }
    else {
      io_gl_buf[0] = '\0';
      hist_pos     = hist_last;
    }
    copy_string(io_gl_buf, hist_buf[hist_pos], IO_GL_BUF_SIZE);
  }

  char *loc = NULL;
  if ((loc = strstr(io_gl_buf, search_string)) != NULL) {
    io_gl_fixup(search_prompt, 0, loc - io_gl_buf);
  }
  else if (search_pos > 0) {
    if (search_forw_flg) {
      search_forw(0);
    }
    else {
      search_back(0);
    }
  }
  else {
    io_gl_fixup(search_prompt, 0, 0);
  }
}

static void search_term(void)
{
  io_gl_search_mode = 0;
  if (io_gl_buf[0] == 0) /* not found, reset hist list */
    hist_pos = hist_last;
  io_gl_fixup(io_gl_prompt, 0, io_gl_pos);
}

static void search_back(int new_search)
{
  search_forw_flg = 0;
  if (io_gl_search_mode == 0) {
    search_last = hist_pos = hist_last;
    search_update(0);
    io_gl_search_mode = 1;
    io_gl_buf[0]      = '\0';
    io_gl_fixup(search_prompt, 0, 0);
  }
  else if (search_pos > 0) {
    int found = 0;
    while (!found) {
      char *loc;
      char *p = hist_prev();
      if (*p == 0) { /* not found, done looking */
        io_gl_buf[0] = '\0';
        io_gl_fixup(search_prompt, 0, 0);
        found = 1;
      }
      else if ((loc = strstr(p, search_string)) != NULL) {
        copy_string(io_gl_buf, p, IO_GL_BUF_SIZE);
        io_gl_fixup(search_prompt, 0, loc - p);
        if (new_search)
          search_last = hist_pos;
        found = 1;
      }
    }
  }
  else {
    io_gl_beep();
  }
}

static void search_forw(int new_search)
{
  char *loc;

  search_forw_flg = 1;
  if (io_gl_search_mode == 0) {
    search_last = hist_pos = hist_last;
    search_update(0);
    io_gl_search_mode = 1;
    io_gl_buf[0]      = '\0';
    io_gl_fixup(search_prompt, 0, 0);
  }
  else if (search_pos > 0) {
    int found = 0;
    while (!found) {
      char *p = hist_next();
      if (*p == 0) { /* not found, done looking */
        io_gl_buf[0] = '\0';
        io_gl_fixup(search_prompt, 0, 0);
        found = 1;
      }
      else if ((loc = strstr(p, search_string)) != NULL) {
        copy_string(io_gl_buf, p, IO_GL_BUF_SIZE);
        io_gl_fixup(search_prompt, 0, loc - p);
        if (new_search)
          search_last = hist_pos;
        found = 1;
      }
    }
  }
  else {
    io_gl_beep();
  }
}

static void io_gl_beep(void)
{
#ifdef __windows__
  MessageBeep(MB_OK);
#else
  io_gl_putc('\007');
#endif
} /* io_gl_beep */
