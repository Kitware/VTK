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
  SUBDIRS(freetype ftgl ParseOGLExt)
ENDIF(VTK_USE_RENDERING)

# DISABLING UNTIL A TRY-COMPILE CAN DETERMINE IF IT IS SAFE
# See internal_stdio_core.h.in for explanation.
#CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/internal_stdio_core.h.in
#               ${CMAKE_CURRENT_BINARY_DIR}/internal/stdio_core.h
#               @ONLY IMMEDIATE)

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  # This is the installed VTKConfig.cmake file configured at the top
  # level.
  INSTALL_FILES(${VTK_INSTALL_LIB_DIR} .cmake VTKConfig)
  INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} .h
                vtk_expat vtk_jpeg vtk_png vtk_zlib vtk_tiff)
#  INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR}/internal FILES
#                ${CMAKE_CURRENT_BINARY_DIR}/internal/stdio_core.h)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
