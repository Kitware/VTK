This directory contains the sources for the GL2PS library.  The
complete GL2PS library with documentation is available from here:

  http://www.geuz.org/gl2ps/

We thank Christophe Geuzaine <geuzaine@acm.caltech.edu> for
distributing this library under a license compatible with VTK.

Please read the COPYING.GL2PS license before you make any
modifications to this copy of the GL2PS sources.


Notes on how GL2PS is built in VTK
----------------------------------

CMakeLists.txt is conspicuous by its absence in this directory.
Currently, the GL2PS sources are in two files, one header and the
other the library.  The GL2PS code is only used in Rendering so we
simply compile gl2ps.c in the Rendering directory and build the
Rendering library along with the resulting gl2ps object file.  Since
VTK ships with its own ZLIB library, GL2PS compressed output is
enabled and GL2PS_HAVE_ZLIB is defined inside
Rendering/CMakeLists.txt.
