This directory contains a subset of the Freetype library (2.1.1).
We only include enough of distribution to build it.

We'd like to thank the Freetype team for distributing this library.
http://www.freetype.org

Modifications
-------------
freetype\builds\win32\freetype\config\ftoption.h: 
  new file used to support DLL build for Windows

src\base\ftmac.c:
  added: 
#if TARGET_API_MAC_CARBON
#include <Carbon/Carbon.h>
#else
[...]
#endif
