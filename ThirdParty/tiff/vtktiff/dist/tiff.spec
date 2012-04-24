#	Header
#
# TIFF Software
#
# Copyright (c) 1994-1997 Sam Leffler
# Copyright (c) 1994-1997 Silicon Graphics, Inc.
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
#
define CUR_MAJ_VERS	1006		# Major Version number
define CUR_MIN_VERS	001		# Minor Version number
define CUR_VERS		${CUR_MAJ_VERS}${CUR_MIN_VERS}${ALPHA}
define TIFF_NAME	"TIFF"

include tiff.version
include tiff.alpha

product tiff
    id	"${TIFF_NAME} Tools, Version ${TIFF_VNUM}"
    inplace

    image sw
	id	"${TIFF_NAME} Software"
	version	"${CUR_VERS}"
	subsys tools default
	    id	"${TIFF_NAME} Tools & Library DSO"
	    exp	"tiff.sw.tools"
	endsubsys
	subsys dev
	    id	"${TIFF_NAME} Developement Software"
	    exp	"tiff.sw.dev"
	endsubsys
    endimage

    image man
	id	"${TIFF_NAME} Documentation"
	version	"${CUR_VERS}"
	subsys tools default
	    id	"${TIFF_NAME} Tools Manual Pages"
	    exp	"tiff.man.tools"
	endsubsys
	subsys dev
	    id	"${TIFF_NAME} Library Manual Pages"
	    exp	"tiff.man.dev"
	endsubsys
	subsys html
	    id	"${TIFF_NAME} HTML Materials"
	    exp	"tiff.man.html"
	endsubsys
    endimage
endproduct
