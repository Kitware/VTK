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

2) When the VTK build configuring with CMake, set the following options:

//Use mangled Mesa with OpenGL
VTK_USE_MANGLED_MESA:BOOL=ON

//What is the path where the file GL/gl_mangle.h can be found
MANGLED_MESA_INCLUDE_DIR:PATH=/home/berk/mesa/include

//Where can the MesaGL library be found
MANGLED_MESA_LIBRARY:FILEPATH=/home/berk/mesa/lib/libMGL.a

//Where can the OSMesa library be found
MANGLED_OSMESA_LIBRARY:FILEPATH=/home/berk/mesalib/libOSMesa.a

Note that the Mesa library is named differently than the OpenGL library
(which is usually something like libGL.a). MesaOS is the offscreen
library which exists on some Mesa distributions, it might not be
needed (in which case, you should leave it as NOTFOUND). 

Note:  The MANGLED_* variables will not be in the cache until 
VTK_USED_MANGLE_MESA has been turned on, and cmake has been run again.


3) Compile VTK.



   