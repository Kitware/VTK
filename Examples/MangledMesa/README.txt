Here is how to compile VTK with mangled Mesa:

1) Build and install mangled Mesa. Since the included file names are
   identical to the files provided by OpenGL, it is required to install
   Mesa such that the include directory is not the same as some other
   package used by VTK. For example, if Tcl/Tk headers are in 
   /usr/local/include and if you install Mesa in /usr/local, VTK will
   wrongly use the Mesa headers even for the OpenGL classes. To avoid
   this, install Mesa away from any other package used by VTK. I 
   recommend creating a directory under your home and installing it there.
   It also important that you name the Mesa library differently. I
   prefer to name it MesaGL.

2) Modify the Mesa include files (gl.h glx.h glext.h osmesa.h and depending
   on the Mesa distribution maybe some others) so that they include from
   the same directory. For example, replace
#include <GL/gl.h> 
   in osmesa.h, with
#include "gl.h"
   This is required because otherwise Mesa is likely to include OpenGL
   header files. The following shell script can be used for this:
#!/bin/sh

sed -e 's/include[ \t][<"]GL\/\(.*\)[">]/include "\1"/' $1 > _tmp.txt
mv _tmp.txt $1

   I use something like
for name in *.h
do
replace_includes.sh *.h
done

3) In your build directory, create two header files: mesagl.h, mesaglx.h.
   In these, include the Mesa header files, providing the full path:
mesagl.h:
#include "/home/berk/mesa/include/GL/gl.h"
#include "/home/berk/mesa/include/GL/osmesa.h"

mesaglx.h:
#include "/home/berk/mesa/include/GL/glx.h"
#include "/home/berk/mesa/include/GL/osmesa.h"

4) When configuring VTK, set the following entries in your CMakeCache.txt:
//Where can the MesaGL library be found
MESA_LIBRARY:FILEPATH=/home/berk/mesa/lib/libMesaGL.a

//Where can the MesaOS library be found
MESA_OS_LIBRARY:FILEPATH=/home/berk/mesa/lib/libMesaOS.a

//Use mangled Mesa with OpenGL
VTK_MANGLE_MESA:BOOL=ON

Note that the Mesa library is named differently than the OpenGL library
(which is usually something like libGL.a). MesaOS is the offscreen
library which exists on some Mesa distributions, it might not be
needed (in which case, you should leave it as NOTFOUND). 

5) Compile VTK.



   