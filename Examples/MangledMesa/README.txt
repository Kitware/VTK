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

2) When configuring VTK, set the following entries in your CMakeCache.txt:

//Use mangled Mesa with OpenGL
VTK_MANGLE_MESA:BOOL=ON

//Where can the MesaGL library be found
MESA_LIBRARY:FILEPATH=/home/berk/mesa/lib/libMesaGL.a

//Where can the MesaOS library be found
MESA_OS_LIBRARY:FILEPATH=/home/berk/mesa/lib/libMesaOS.a

//What is the path where the file gl_mangle.h can be found
MESA_MANGLE_PATH:PATH=/home/berk/mesa/include/GL



Note that the Mesa library is named differently than the OpenGL library
(which is usually something like libGL.a). MesaOS is the offscreen
library which exists on some Mesa distributions, it might not be
needed (in which case, you should leave it as NOTFOUND). 

Note:  The MESA_* variables will not be in the cache until 
VTK_MANGLE_MESA has been turned on, and cmake has been run again.


3) Compile VTK.



   