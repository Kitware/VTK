# Build third-party utilities.
VTK_THIRD_PARTY_SUBDIR(ZLIB zlib)
VTK_THIRD_PARTY_SUBDIR(JPEG jpeg)
VTK_THIRD_PARTY_SUBDIR(PNG  png)
VTK_THIRD_PARTY_SUBDIR(TIFF tiff)
VTK_THIRD_PARTY_SUBDIR(EXPAT expat)

# DICOM support is disabled until the following are fixed by the authors:
#  - Make it build with and without VTK_USE_ANSI_STDLIB.
#  - Remove all build warnings from dashboard.
#  - Add a test to bring up coverage of vtkDICOMImageReader.cxx
#    and actually read a DICOM file.
#IF(VTK_USE_ANSI_STDLIB)
#  VTK_THIRD_PARTY_SUBDIR(DICOMParser DICOMParser)
#ENDIF(VTK_USE_ANSI_STDLIB)

SUBDIRS(Doxygen)

IF(VTK_USE_RENDERING)
  SUBDIRS(freetype ftgl)
ENDIF(VTK_USE_RENDERING)

# This is the installed VTKConfig.cmake file configured at the top
# level.
INSTALL_FILES(/lib/vtk .cmake VTKConfig)
