# Build third-party utilities.

VTK_THIRD_PARTY_SUBDIR(ZLIB vtkzlib)
VTK_THIRD_PARTY_SUBDIR(JPEG vtkjpeg)
VTK_THIRD_PARTY_SUBDIR(PNG  vtkpng)
VTK_THIRD_PARTY_SUBDIR(TIFF vtktiff)
VTK_THIRD_PARTY_SUBDIR(EXPAT vtkexpat)
VTK_THIRD_PARTY_SUBDIR(DICOMParser DICOMParser)

SUBDIRS(Doxygen)
IF(VTK_INCLUDE_NEED_TCL)
  SUBDIRS(TclTk)
ENDIF(VTK_INCLUDE_NEED_TCL)

IF(VTK_USE_RENDERING)
  SUBDIRS(freetype ftgl)
ENDIF(VTK_USE_RENDERING)

# This is the installed VTKConfig.cmake file configured at the top
# level.
INSTALL_FILES(/lib/vtk .cmake VTKConfig)
INSTALL_FILES(/include/vtk .h vtk_expat vtk_jpeg vtk_png vtk_zlib vtk_tiff)
