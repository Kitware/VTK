# Options to enable and disable internal codecs
#
# Copyright © 2015 Open Microscopy Environment / University of Dundee
# Copyright © 2021 Roger Leigh <rleigh@codelibre.net>
# Written by Roger Leigh <rleigh@codelibre.net>
#
# Permission to use, copy, modify, distribute, and sell this software and
# its documentation for any purpose is hereby granted without fee, provided
# that (i) the above copyright notices and this permission notice appear in
# all copies of the software and related documentation, and (ii) the names of
# Sam Leffler and Silicon Graphics may not be used in any advertising or
# publicity relating to the software without the specific, prior written
# permission of Sam Leffler and Silicon Graphics.
#
# THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
# EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
# WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
#
# IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
# ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
# OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
# WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
# LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
# OF THIS SOFTWARE.


if (FALSE) # XXX(kitware): hardcode settings
option(ccitt "support for CCITT Group 3 & 4 algorithms" ON)
else ()
set(ccitt ON)
endif ()
set(CCITT_SUPPORT ${ccitt})

if (FALSE) # XXX(kitware): hardcode settings
option(packbits "support for Macintosh PackBits algorithm" ON)
else ()
set(packbits ON)
endif ()
set(PACKBITS_SUPPORT ${packbits})

if (FALSE) # XXX(kitware): hardcode settings
option(lzw "support for LZW algorithm" ON)
else ()
set(lze ON)
endif ()
set(LZW_SUPPORT ${lzw})

if (FALSE) # XXX(kitware): hardcode settings
option(thunder "support for ThunderScan 4-bit RLE algorithm" ON)
else ()
set(thunder ON)
endif ()
set(THUNDER_SUPPORT ${thunder})

if (FALSE) # XXX(kitware): hardcode settings
option(next "support for NeXT 2-bit RLE algorithm" ON)
else ()
set(next ON)
endif ()
set(NEXT_SUPPORT ${next})

if (FALSE) # XXX(kitware): hardcode settings
option(logluv "support for LogLuv high dynamic range algorithm" ON)
else ()
set(loglub ON)
endif ()
set(LOGLUV_SUPPORT ${logluv})

if (FALSE) # XXX(kitware): hardcode settings
option(mdi "support for Microsoft Document Imaging" ON)
else ()
set(mdi ON)
endif ()
set(MDI_SUPPORT ${mdi})
