This directory contains the sources for the GL2PS library.  The
complete GL2PS library with documentation is available from here:

  http://www.geuz.org/gl2ps/

We thank Christophe Geuzaine <geuzaine@acm.caltech.edu> for
distributing this library under a license compatible with VTK.

Please read the COPYING.GL2PS license before you make any
modifications to this copy of the GL2PS sources.

Modifications to the GL2PS library
----------------------------------

The gl2psTextOpt function has been modified to accept a color argument. The
default mechanism for coloring text in GL2PS querys the GL current raster color,
which is not required to be valid during feedback rendering. By passing the
color directly to GL2PS, this ambiguity is avoided.

The glSpecial mechanism for inserting PDF drawing instructions has been
fixed/implemented.

In order to avoid linking errors we have modified gl2ps to include vtk_zlib.h
and vtk_png.h instead of the {zlib,png}.h headers.

To aid with regression testing, which is currently performed using MD5 hashes,
an additional option for gl2psEnable/Disable is added, GL2PS_TIMESTAMP, which,
if enabled, replaces the output file's timestamp with a static string.

An additional function, gl2psGetFileFormat(), has been added to the GL2PS API
to allow VTK utilities to query the type of file being produced during GL2PS
export.
