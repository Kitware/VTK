
/*
 * Copyright (C) 1991, 1992, 1993, 2021, 2022, 2023, 2024 by Chris Thewalt (thewalt@ce.berkeley.edu)
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
#endif

/********************* C library headers ********************************/
#include <array>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#ifdef __EMSCRIPTEN__
#include <errno.h>
#elif __unix__
#include <sys/errno.h>
#endif

#include "Ioss_CodeTypes.h"
#include "Ioss_Getline.h"

namespace {
  const int GL_BUF_SIZE{1024};

  /******************** external interface *********************************/

  bool gl_ellipses_during_completion = true;
  char gl_buf[GL_BUF_SIZE]; /* input buffer */

  int         gl_init_done = -1;            /* terminal mode flag  */
  int         gl_termw     = 80;            /* actual terminal width */
  int         gl_scroll    = 27;            /* width of EOL scrolling region */
  int         gl_width     = 0;             /* net size available for input */
  int         gl_extent    = 0;             /* how far to redraw, 0 means all */
  int         gl_overwrite = 0;             /* overwrite mode */
  int         gl_pos, gl_cnt = 0;           /* position and size of input */
  char        gl_killbuf[GL_BUF_SIZE] = ""; /* killed text */
  const char *gl_prompt;                    /* to save the prompt string */
  int         gl_search_mode = 0;           /* search mode flag */

  void gl_init();         /* prepare to edit a line */
  void gl_cleanup();      /* to undo gl_init */
  void gl_char_init();    /* get ready for no echo input */
  void gl_char_cleanup(); /* undo gl_char_init */
                          /* returns printable prompt width */

  void gl_addchar(int c);    /* install specified char */
  void gl_del(int loc, int); /* del, either left (-1) or cur (0) */

  [[noreturn]] void gl_error(const char *const buf); /* write error msg and die */

  void gl_fixup(const char *prompt, int change, int cursor); /* fixup state variables and screen */
  int  gl_getc();                                            /* read one char from terminal */
  void gl_kill(int pos);                                     /* delete to EOL */
  void gl_newline();                                         /* handle \n or \r */
  void gl_putc(int c);                                       /* write one char to terminal */
  void gl_puts(const char *const buf);                       /* write a line to terminal */
  void gl_redraw();                                          /* issue \n and redraw all */
  void gl_transpose();                                       /* transpose two chars */
  void gl_yank();                                            /* yank killed text */

  void  hist_init();              /* initializes hist pointers */
  char *hist_next();              /* return ptr to next item */
  char *hist_prev();              /* return ptr to prev item */
  char *hist_save(const char *p); /* makes copy of a string, without NL */

  void search_addchar(int c);       /* increment search string */
  void search_term();               /* reset with current contents */
  void search_back(int new_search); /* look back for current string */
  void search_forw(int new_search); /* look forw for current string */
  void gl_beep();                   /* try to play a system beep sound */

  char *copy_string(char *dest, char const *source, long int elements)
  {
    char *d;
    for (d = dest; d + 1 < dest + elements && *source; d++, source++) {
      *d = *source;
    }
    *d = '\0';
    return d;
  }
} // namespace
/************************ nonportable part *********************************/

#ifdef MSDOS
#include <bios.h>
#endif

namespace {
#if defined(__EMSCRIPTEN__) || defined(__unix__)
#ifdef __EMSCRIPTEN__
#include <termios.h>
#elif __unix__
#include <sys/termios.h>
#endif
  struct termios io_new_termios;
  struct termios io_old_termios;
#endif

  void gl_char_init() /* turn off input echo */
  {
#ifdef __unix__
    tcgetattr(0, &io_old_termios);
    io_new_termios = io_old_termios;
    io_new_termios.c_iflag &= ~(BRKINT | ISTRIP | IXON | IXOFF);
    io_new_termios.c_iflag |= (IGNBRK | IGNPAR);
    io_new_termios.c_lflag &= ~(ICANON | ISIG | IEXTEN | ECHO);
    io_new_termios.c_cc[VMIN]  = 1;
    io_new_termios.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &io_new_termios);
#endif /* __unix__ */
  }

  void gl_char_cleanup() /* undo effects of gl_char_init */
  {
#ifdef __unix__
    tcsetattr(0, TCSANOW, &io_old_termios);
#endif /* __unix__ */
  }
} // namespace
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

namespace {
  int gl_getc()
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

  void gl_putc(int c)
  {
    char ch = (char)(unsigned char)c;

    IOSS_MAYBE_UNUSED auto result = write(1, &ch, 1);
    IOSS_PAR_UNUSED(result);
    if (ch == '\n') {
      ch     = '\r';
      result = write(1, &ch, 1); /* RAW mode needs '\r', does not hurt */
      IOSS_PAR_UNUSED(result);
    }
  }

  /******************** fairly portable part *********************************/

  void gl_puts(const char *const buf)
  {
    if (buf) {
      int                    len    = strlen(buf);
      IOSS_MAYBE_UNUSED auto result = write(1, buf, len);
      IOSS_PAR_UNUSED(result);
    }
  }

  [[noreturn]] void gl_error(const char *const buf)
  {
    int len = strlen(buf);

    gl_cleanup();
    IOSS_MAYBE_UNUSED auto result = write(2, buf, len);
    IOSS_PAR_UNUSED(result);
    exit(1);
  }

  void gl_init()
  /* set up variables and terminal */
  {
    if (gl_init_done < 0) { /* -1 only on startup */
      const char *cp = (const char *)getenv("COLUMNS");
      if (cp != nullptr) {
        int w = strtol(cp, nullptr, 10);
        if (w > 20) {
          Ioss::gl_setwidth(w);
        }
      }
      hist_init();
    }
    if (isatty(0) == 0 || isatty(1) == 0) {
      gl_error("\n*** Error: getline(): not interactive, use stdio.\n");
    }
    gl_char_init();
    gl_init_done = 1;
  }

  void gl_cleanup()
  /* undo effects of gl_init, as necessary */
  {
    if (gl_init_done > 0) {
      gl_char_cleanup();
    }
    gl_init_done = 0;
#ifdef __windows__
    Sleep(40);
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
#endif
  }
} // namespace

namespace Ioss {
  void gl_setwidth(int w)
  {
    if (w > 250) {
      w = 250;
    }
    if (w > 20) {
      gl_termw  = w;
      gl_scroll = w / 3;
    }
    else {
      gl_error("\n*** Error: minimum screen width is 21\n");
    }
  }

  char *getline_int(const char *prompt)
  {
    gl_init();
    gl_prompt = (prompt) ? prompt : "";
    gl_buf[0] = '\0';
    gl_fixup(gl_prompt, -2, GL_BUF_SIZE);

#ifdef __windows__
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
#endif

    int c;
    while ((c = gl_getc()) >= 0) {
      gl_extent = 0; /* reset to full extent */
      if (isprint(c)) {
        if (gl_search_mode) {
          search_addchar(c);
        }
        else {
          gl_addchar(c);
        }
      }
      else {
        if (gl_search_mode) {
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
          gl_newline();
          gl_cleanup();
          return gl_buf;
        case '\001':
          gl_fixup(gl_prompt, -1, 0); /* ^A */
          break;
        case '\002':
          gl_fixup(gl_prompt, -1, gl_pos - 1); /* ^B */
          break;
        case '\004': /* ^D */
          if (gl_cnt == 0) {
            gl_buf[0] = '\0';
            gl_cleanup();
            gl_putc('\n');
            return gl_buf;
          }
          else {
            gl_del(0, 1);
          }
          break;
        case '\005':
          gl_fixup(gl_prompt, -1, gl_cnt); /* ^E */
          break;
        case '\006':
          gl_fixup(gl_prompt, -1, gl_pos + 1); /* ^F */
          break;
        case '\010':
        case '\177':
          gl_del(-1, 0); /* ^H and DEL */
          break;
        case '\t': /* TAB */ break;
        case '\013':
          gl_kill(gl_pos); /* ^K */
          break;
        case '\014':
          gl_redraw(); /* ^L */
          break;
        case '\016': /* ^N */
          copy_string(gl_buf, hist_next(), GL_BUF_SIZE);
          gl_fixup(gl_prompt, 0, GL_BUF_SIZE);
          break;
        case '\017':
          gl_overwrite = !gl_overwrite; /* ^O */
          break;
        case '\020': /* ^P */
          copy_string(gl_buf, hist_prev(), GL_BUF_SIZE);
          gl_fixup(gl_prompt, 0, GL_BUF_SIZE);
          break;
        case '\022':
          search_back(1); /* ^R */
          break;
        case '\023':
          search_forw(1); /* ^S */
          break;
        case '\024':
          gl_transpose(); /* ^T */
          break;
        case '\025':
          gl_kill(0); /* ^U */
          break;
        case '\031':
          gl_yank(); /* ^Y */
          break;
        default:
          if (c > 0) {
            gl_beep();
          }
          break;
        }
      }
    }
    gl_cleanup();
    gl_buf[0] = '\0';
    return gl_buf;
  }
} // namespace Ioss
namespace {
  void gl_addchar(int c)

  /* adds the character c to the input buffer at current location */
  {

    if (gl_overwrite == 0 || gl_pos == gl_cnt) {
      if (gl_cnt > GL_BUF_SIZE - 2) {
        gl_error("\n*** Error: getline(): input buffer overflow\n");
      }
      for (int i = gl_cnt; i >= gl_pos; i--) {
        gl_buf[i + 1] = gl_buf[i];
      }
      gl_buf[gl_pos] = (char)c;
      gl_fixup(gl_prompt, gl_pos, gl_pos + 1);
    }
    else {
      if (gl_pos > GL_BUF_SIZE - 1) {
        gl_error("\n*** Error: getline(): input buffer overflow\n");
      }
      gl_buf[gl_pos] = (char)c;
      gl_extent      = 1;
      gl_fixup(gl_prompt, gl_pos, gl_pos + 1);
    }
  }

  void gl_yank()
  /* adds the kill buffer to the input buffer at current location */
  {
    int len = strlen(gl_killbuf);
    if (len > 0) {
      if (gl_overwrite == 0) {
        if (gl_cnt + len >= GL_BUF_SIZE) {
          gl_error("\n*** Error: getline(): input buffer overflow\n");
        }
        for (int i = gl_cnt; i >= gl_pos; i--) {
          gl_buf[i + len] = gl_buf[i];
        }
        for (int i = 0; i < len; i++) {
          gl_buf[gl_pos + i] = gl_killbuf[i];
        }
        gl_fixup(gl_prompt, gl_pos, gl_pos + len);
      }
      else {
        if (gl_pos + len >= GL_BUF_SIZE) {
          gl_error("\n*** Error: getline(): input buffer overflow\n");
        }
        if (gl_pos + len > gl_cnt) {
          gl_buf[gl_pos + len] = '\0';
        }
        for (int i = 0; i < len; i++) {
          gl_buf[gl_pos + i] = gl_killbuf[i];
        }
        gl_extent = len;
        gl_fixup(gl_prompt, gl_pos, gl_pos + len);
      }
    }
    else {
      gl_beep();
    }
  }

  void gl_transpose()
  /* switch character under cursor and to left of cursor */
  {
    if (gl_pos > 0 && gl_cnt > gl_pos && gl_pos < GL_BUF_SIZE) {
      int c              = gl_buf[gl_pos - 1];
      gl_buf[gl_pos - 1] = gl_buf[gl_pos];
      gl_buf[gl_pos]     = (char)c;
      gl_extent          = 2;
      gl_fixup(gl_prompt, gl_pos - 1, gl_pos);
    }
    else {
      gl_beep();
    }
  }

  void gl_newline()
  /*
   * Cleans up entire line before returning to caller. A \n is appended.
   * If line longer than screen, we redraw starting at beginning
   */
  {
    int change = gl_cnt;
    int len    = gl_cnt;
    int loc    = gl_width - 5; /* shifts line back to start position */

    if (gl_cnt >= GL_BUF_SIZE - 1) {
      gl_error("\n*** Error: getline(): input buffer overflow\n");
    }
    if (loc > len) {
      loc = len;
    }
    gl_fixup(gl_prompt, change, loc); /* must do this before appending \n */
    gl_buf[len]     = '\n';
    gl_buf[len + 1] = '\0';
    gl_putc('\n');
  }

  void gl_del(int loc, int killsave)

  /*
   * Delete a character.  The loc variable can be:
   *    -1 : delete character to left of cursor
   *     0 : delete character under cursor
   */
  {
    if ((loc == -1 && gl_pos > 0) || (loc == 0 && gl_pos < gl_cnt)) {
      int j = 0;
      for (int i = gl_pos + loc; i < gl_cnt; i++) {
        if (i < GL_BUF_SIZE - 1) {
          if ((j == 0) && (killsave != 0)) {
            gl_killbuf[0] = gl_buf[i];
            gl_killbuf[1] = '\0';
            j             = 1;
          }
          gl_buf[i] = gl_buf[i + 1];
        }
        else {
          gl_error("\n*** Error: getline(): logic error in gl_del().\n");
        }
      }
      gl_fixup(gl_prompt, gl_pos + loc, gl_pos + loc);
    }
    else {
      gl_beep();
    }
  }

  void gl_kill(int pos)

  /* delete from pos to the end of line */
  {
    if (pos < gl_cnt && pos < GL_BUF_SIZE) {
      copy_string(gl_killbuf, gl_buf + pos, GL_BUF_SIZE);
      gl_buf[pos] = '\0';
      gl_fixup(gl_prompt, pos, pos);
    }
    else {
      gl_beep();
    }
  }

  void gl_redraw()
  /* emit a newline, reset and redraw prompt and current input line */
  {
    if (gl_init_done > 0) {
      gl_putc('\n');
      gl_fixup(gl_prompt, -2, gl_pos);
    }
  }

  void gl_fixup(const char *prompt, int change, int cursor)

  /*
   * This function is used both for redrawing when input changes or for
   * moving within the input line.  The parameters are:
   *   prompt:  compared to last_prompt[] for changes;
   *   change : the index of the start of changes in the input buffer,
   *            with -1 indicating no changes, -2 indicating we're on
   *            a new line, redraw everything.
   *   cursor : the desired location of the cursor after the call.
   *            A value of GL_BUF_SIZE can be used  to indicate the cursor should
   *            move just past the end of the input line.
   */
  {
    static int  gl_shift;  /* index of first on screen character */
    static int  off_right; /* true if more text right of screen */
    static int  off_left;  /* true if more text left of screen */
    static char last_prompt[80] = "";
    int         left = 0, right = -1; /* bounds for redraw */
    int         new_right = -1;       /* alternate right bound, using gl_extent */

    if (change == -2) { /* reset */
      gl_pos = gl_cnt = gl_shift = off_right = off_left = 0;
      gl_putc('\r');
      gl_puts(prompt);
      copy_string(last_prompt, prompt, 80);
      change   = 0;
      gl_width = gl_termw - strlen(prompt);
    }
    else if (strcmp(prompt, last_prompt) != 0) {
      int l1 = strlen(last_prompt);
      int l2 = strlen(prompt);
      gl_cnt = gl_cnt + l1 - l2;
      copy_string(last_prompt, prompt, 80);
      gl_putc('\r');
      gl_puts(prompt);
      gl_pos   = gl_shift;
      gl_width = gl_termw - l2;
      change   = 0;
    }
    /* how much to erase at end of line */
    int pad    = (off_right) ? gl_width - 1 : gl_cnt - gl_shift; /* old length */
    int backup = gl_pos - gl_shift; /* how far to backup before fixing */
    if (change >= 0) {
      gl_cnt = strlen(gl_buf);
      if (change > gl_cnt) {
        change = gl_cnt;
      }
    }
    if (cursor > gl_cnt) {
      if (cursor != GL_BUF_SIZE) { /* GL_BUF_SIZE means end of line */
        if (gl_ellipses_during_completion) {
          gl_beep();
        }
      }
      cursor = gl_cnt;
    }
    if (cursor < 0) {
      gl_beep();
      cursor = 0;
    }
    int extra = 0; /* adjusts when shift (scroll) happens */
    if (off_right || (off_left && cursor < gl_shift + gl_width - gl_scroll / 2)) {
      extra = 2; /* shift the scrolling boundary */
    }
    int new_shift = cursor + extra + gl_scroll - gl_width;
    if (new_shift > 0) {
      new_shift /= gl_scroll;
      new_shift *= gl_scroll;
    }
    else {
      new_shift = 0;
    }
    if (new_shift != gl_shift) { /* scroll occurs */
      gl_shift  = new_shift;
      off_left  = (gl_shift) ? 1 : 0;
      off_right = (gl_cnt > gl_shift + gl_width - 1) ? 1 : 0;
      left      = gl_shift;
      new_right = right = (off_right) ? gl_shift + gl_width - 2 : gl_cnt;
    }
    else if (change >= 0) { /* no scroll, but text changed */
      if (change < gl_shift + off_left) {
        left = gl_shift;
      }
      else {
        left   = change;
        backup = gl_pos - change;
      }
      off_right = (gl_cnt > gl_shift + gl_width - 1) ? 1 : 0;
      right     = (off_right) ? gl_shift + gl_width - 2 : gl_cnt;
      new_right = (gl_extent && (right > left + gl_extent)) ? left + gl_extent : right;
    }
    pad -= (off_right) ? gl_width - 1 : gl_cnt - gl_shift;
    pad = (pad < 0) ? 0 : pad;
    if (left <= right) { /* clean up screen */
      for (int i = 0; i < backup; i++) {
        gl_putc('\b');
      }
      if (left == gl_shift && off_left) {
        gl_putc('$');
        left++;
      }
      for (int i = left; i < new_right; i++) {
        gl_putc(gl_buf[i]);
      }
      gl_pos = new_right;
      if (off_right && new_right == right) {
        gl_putc('$');
        gl_pos++;
      }
      else {
        for (int i = 0; i < pad; i++) { /* erase remains of prev line */
          gl_putc(' ');
        }
        gl_pos += pad;
      }
    }
    int i = gl_pos - cursor; /* move to final cursor location */
    if (i > 0) {
      while (i--) {
        gl_putc('\b');
      }
    }
    else {
      if (cursor < GL_BUF_SIZE) {
        for (int ii = gl_pos; ii < cursor; ii++) {
          gl_putc(gl_buf[ii]);
        }
      }
      else {
        gl_error("\n*** Error: getline(): logic error in gl_fixup().\n");
      }
    }
    gl_pos = cursor;
  }

  /******************* History stuff **************************************/

#ifndef HIST_SIZE
#define HIST_SIZE 100
#endif

  int                           hist_pos = 0, hist_last = 0;
  std::array<char *, HIST_SIZE> hist_buf;
  char                          hist_empty_elem[2] = "";

  void hist_init()
  {
    hist_buf[0] = hist_empty_elem;
    for (int i = 1; i < HIST_SIZE; i++) {
      hist_buf[i] = nullptr;
    }
  }
} // namespace

namespace Ioss {
  void gl_histadd(const char *buf)
  {
    static char *prev = nullptr;

    /* in case we call gl_histadd() before we call getline() */
    if (gl_init_done < 0) { /* -1 only on startup */
      hist_init();
      gl_init_done = 0;
    }
    const char *p = buf;
    while (*p == ' ' || *p == '\t' || *p == '\n') {
      p++;
    }
    if (*p) {
      int len = strlen(buf);
      if (strchr(p, '\n')) { /* previously line already has NL stripped */
        len--;
      }
      if ((prev == nullptr) || ((int)strlen(prev) != len) || strncmp(prev, buf, (size_t)len) != 0) {
        hist_buf[hist_last] = hist_save(buf);
        prev                = hist_buf[hist_last];
        hist_last           = (hist_last + 1) % HIST_SIZE;
        if (hist_buf[hist_last] && *hist_buf[hist_last]) {
          delete[] hist_buf[hist_last];
        }
        hist_buf[hist_last] = hist_empty_elem;
      }
    }
    hist_pos = hist_last;
  }
} // namespace Ioss
namespace {
  char *hist_prev()
  /* loads previous hist entry into input buffer, sticks on first */
  {
    char *p    = nullptr;
    int   next = (hist_pos - 1 + HIST_SIZE) % HIST_SIZE;

    if (hist_buf[hist_pos] != nullptr && next != hist_last) {
      hist_pos = next;
      p        = hist_buf[hist_pos];
    }
    if (p == nullptr) {
      p = hist_empty_elem;
      gl_beep();
    }
    return p;
  }

  char *hist_next()
  /* loads next hist entry into input buffer, clears on last */
  {
    char *p = nullptr;

    if (hist_pos != hist_last) {
      hist_pos = (hist_pos + 1) % HIST_SIZE;
      p        = hist_buf[hist_pos];
    }
    if (p == nullptr) {
      p = hist_empty_elem;
      gl_beep();
    }
    return p;
  }

  char *hist_save(const char *p)

  /* makes a copy of the string */
  {
    char       *s   = nullptr;
    size_t      len = strlen(p);
    const char *nl  = strpbrk(p, "\n\r");

    if (nl) {
      s = new char[len];
      copy_string(s, p, len);
      s[len - 1] = '\0';
    }
    else {
      s = new char[len + 1];
      copy_string(s, p, len + 1);
    }
    return s;
  }

  /******************* Search stuff **************************************/

  char search_prompt[101]; /* prompt includes search string */
  char search_string[100];
  int  search_pos      = 0; /* current location in search_string */
  int  search_forw_flg = 0; /* search direction flag */
  int  search_last     = 0; /* last match found */

  void search_update(int c)
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
        search_string[search_pos] = (char)0;

        search_prompt[search_pos]     = (char)'?';
        search_prompt[search_pos + 1] = (char)' ';
        search_prompt[search_pos + 2] = (char)0;
      }
      else {
        gl_beep();
        hist_pos = hist_last;
      }
    }
  }

  void search_addchar(int c)
  {
    search_update(c);
    if (c < 0) {
      if (search_pos > 0) {
        hist_pos = search_last;
      }
      else {
        gl_buf[0] = '\0';
        hist_pos  = hist_last;
      }
      copy_string(gl_buf, hist_buf[hist_pos], GL_BUF_SIZE);
    }

    char *loc = nullptr;
    if ((loc = strstr(gl_buf, search_string)) != nullptr) {
      gl_fixup(search_prompt, 0, loc - gl_buf);
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
      gl_fixup(search_prompt, 0, 0);
    }
  }

  void search_term()
  {
    gl_search_mode = 0;
    if (gl_buf[0] == 0) { /* not found, reset hist list */
      hist_pos = hist_last;
    }
    gl_fixup(gl_prompt, 0, gl_pos);
  }

  void search_back(int new_search)
  {
    search_forw_flg = 0;
    if (gl_search_mode == 0) {
      search_last = hist_pos = hist_last;
      search_update(0);
      gl_search_mode = 1;
      gl_buf[0]      = '\0';
      gl_fixup(search_prompt, 0, 0);
    }
    else if (search_pos > 0) {
      bool found = false;
      while (!found) {
        char *loc;
        char *p = hist_prev();
        if (*p == 0) { /* not found, done looking */
          gl_buf[0] = '\0';
          gl_fixup(search_prompt, 0, 0);
          found = true;
        }
        else if ((loc = strstr(p, search_string)) != nullptr) {
          copy_string(gl_buf, p, GL_BUF_SIZE);
          gl_fixup(search_prompt, 0, loc - p);
          if (new_search) {
            search_last = hist_pos;
          }
          found = true;
        }
      }
    }
    else {
      gl_beep();
    }
  }

  void search_forw(int new_search)
  {
    char *loc;

    search_forw_flg = 1;
    if (gl_search_mode == 0) {
      search_last = hist_pos = hist_last;
      search_update(0);
      gl_search_mode = 1;
      gl_buf[0]      = '\0';
      gl_fixup(search_prompt, 0, 0);
    }
    else if (search_pos > 0) {
      bool found = false;
      while (!found) {
        char *p = hist_next();
        if (*p == 0) { /* not found, done looking */
          gl_buf[0] = '\0';
          gl_fixup(search_prompt, 0, 0);
          found = true;
        }
        else if ((loc = strstr(p, search_string)) != nullptr) {
          copy_string(gl_buf, p, GL_BUF_SIZE);
          gl_fixup(search_prompt, 0, loc - p);
          if (new_search) {
            search_last = hist_pos;
          }
          found = true;
        }
      }
    }
    else {
      gl_beep();
    }
  }

  void gl_beep()
  {
#ifdef __windows__
    MessageBeep(MB_OK);
#else
    gl_putc('\007');
#endif
  } /* gl_beep */
} // namespace
