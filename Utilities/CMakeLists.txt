# Build third-party utilities.
VTK_THIRD_PARTY_SUBDIR(ZLIB zlib)
VTK_THIRD_PARTY_SUBDIR(JPEG jpeg)
VTK_THIRD_PARTY_SUBDIR(PNG  png)
VTK_THIRD_PARTY_SUBDIR(TIFF tiff)
VTK_THIRD_PARTY_SUBDIR(EXPAT expat)
SUBDIRS(Doxygen)

IF(VTK_USE_RENDERING)
  SUBDIRS(freetype ftgl)
ENDIF(VTK_USE_RENDERING)

# This is the installed VTKConfig.cmake file configured at the top
# level.
INSTALL_FILES(/lib/vtk .cmake VTKConfig)
