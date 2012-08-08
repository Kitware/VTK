#ifndef vtk_netcdf_mangle_h
#define vtk_netcdf_mangle_h

/*

This header file mangles all symbols exported from the netcdf library.
It is included in all files while building the netcdf library.  Due to
namespace pollution, no netcdf headers should be included in .h files in
VTK.

The following command was used to obtain the symbol list:

nm libvtkNetCDF.a |grep " [TRD] "

This is the way to recreate the whole list:

nm bin/libvtkNetCDF.so |grep " [TRD] " | awk '{ print "#define "$3" vtk_netcdf_"$3 }' | \
        grep -v vtk_netcdf__fini | grep -v vtk_netcdf__init | sort

Note that _fini and _init should be excluded because they are not functions
implemented by the library but are rather created by the linker and
used when the shared library is loaded/unloaded from an executable.

*/

#include <vtknetcdf/ncconfig.h>

#endif /* vtk_netcdf_mangle_h */
