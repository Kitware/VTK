This directory contains the sources for the GL2PS library.  The
complete GL2PS library with documentation is available from here:

  http://www.geuz.org/gl2ps/

We thank Christophe Geuzaine <geuzaine@acm.caltech.edu> for
distributing this library under a license compatible with VTK.

Please read the COPYING.GL2PS license before you make any
modifications to this copy of the GL2PS sources.

Modifications to the GL2PS library
----------------------------------

In order to avoid linking errors we have modified gl2ps to include vtk_zlib.h
and vtk_png.h instead of the {zlib,png}.h headers.
