This directory contains a clone of the libharu repository for VTK.

You can search the code for "KITWARE_LIBHARU_CHANGE" to find
modifications, and this file contains a summary of changes.

* Added vtk_haru_mangle.h to mangle symbol names.
* Skip upstream install rules.
* Replace CMAKE_SOURCE_DIR/BINARY_DIR with PROJECT_ variants.
* Standardize library names for platform conventions.
* Set LIBHPDF_STATIC/SHARED based on BUILD_SHARED_LIBS.
* Force exception support.
* Force extra debug output off.
* Remove call to summary() command to silence configuration process.
* Remove unused MATH_LIB cmake variable.
* Renamed various cmake objects to have 'vtk' prefix.
* Link to / include vtk's png and zlib libraries.
* Add .gitattributes file to ignore whitespace from commit checks.
* Edited a comment in hpdf_objects.h to remove invalid UTF8 characters.
* Update API to add support for HPDF_Shading objects (Upstream MR #157).
* Add link to libm on unix.
