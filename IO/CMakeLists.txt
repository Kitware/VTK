SOURCE_FILES( IO_SRCS
vtkBMPReader.cxx
vtkBMPWriter.cxx
vtkBYUReader.cxx
vtkBYUWriter.cxx
vtkDataObjectReader.cxx
vtkDataObjectWriter.cxx
vtkDataReader.cxx
vtkDataSetReader.cxx
vtkDataSetWriter.cxx
vtkDataWriter.cxx
vtkDEMReader.cxx
vtkEnSight6BinaryReader.cxx
vtkEnSight6Reader.cxx
vtkEnSightGoldBinaryReader.cxx
vtkEnSightGoldReader.cxx
vtkEnSightReader.cxx
vtkGESignaReader.cxx
vtkGenericEnSightReader.cxx
vtkImageReader.cxx
vtkImageReader2.cxx
vtkImageReader2Factory.cxx
vtkImageReader2Collection.cxx
vtkImageWriter.cxx
vtkIVWriter.cxx
vtkJPEGReader.cxx
vtkJPEGWriter.cxx
vtkMCubesReader.cxx
vtkMCubesWriter.cxx
vtkMedicalImageReader2.cxx
vtkOBJReader.cxx
vtkParticleReader.cxx
vtkPLOT3DReader.cxx
vtkPLY.cxx
vtkPLYReader.cxx
vtkPLYWriter.cxx
vtkPNGReader.cxx
vtkPNGWriter.cxx
vtkPNMReader.cxx
vtkPNMWriter.cxx
vtkPolyDataReader.cxx
vtkPolyDataWriter.cxx
vtkPostScriptWriter.cxx
vtkRectilinearGridReader.cxx
vtkRectilinearGridWriter.cxx
vtkSLCReader.cxx
vtkSTLReader.cxx
vtkSTLWriter.cxx
vtkStructuredGridReader.cxx
vtkStructuredGridWriter.cxx
vtkStructuredPointsReader.cxx
vtkStructuredPointsWriter.cxx
vtkTIFFReader.cxx
vtkTIFFWriter.cxx
vtkUGFacetReader.cxx
vtkUnstructuredGridReader.cxx
vtkUnstructuredGridWriter.cxx
vtkVolume16Reader.cxx
vtkVolumeReader.cxx
vtkWriter.cxx
vtkXMLParser.cxx
)

# for cmake version 1.1 and later
SET(WINDOWS 0)
IF (WIN32)
   IF (NOT UNIX)
   SET(WINDOWS 1)
   ENDIF (NOT UNIX)
ENDIF (WIN32)
IF(NOT WINDOWS)
  IF (CMAKE_MAJOR_VERSION MATCHES [1-9][0-9]*)
    IF (CMAKE_MINOR_VERSION MATCHES [2-9][0-9]*)
      SET(TIFF_RIGHT_VERSION 1)
    ENDIF (CMAKE_MINOR_VERSION MATCHES [2-9][0-9]*)
  ENDIF (CMAKE_MAJOR_VERSION MATCHES [1-9][0-9]*)

  IF(NOT TIFF_RIGHT_VERSION)
    MESSAGE("The new TIFF reader requires CMake 1.2 or newer. Please get the new version of CMake from www.cmake.org.")
  ENDIF(NOT TIFF_RIGHT_VERSION)
ENDIF(NOT WINDOWS)

ABSTRACT_FILES(
vtkEnSightReader
vtkVolumeReader
vtkWriter
vtkXMLParser
)

LINK_LIBRARIES( vtkpng vtkzlib vtkjpeg vtktiff vtkexpat )

# if we are wrapping into Tcl then add the library and extra
# source files
#
WRAP_EXCLUDE_FILES(vtkPLY)

IF(VTK_MAKE_INSTANTIATOR)
  # Setup vtkInstantiator registration for this library's classes.
  VTK_MAKE_INSTANTIATOR(vtkIOInstantiator IOInstantiator_SRCS
                        IO_SRCS
                        EXPORT_MACRO VTK_IO_EXPORT
                        HEADER_LOCATION ${VTK_BINARY_DIR})
  ADD_LIBRARY(vtkIO IO_SRCS IOInstantiator_SRCS)
  INSTALL_FILES(/include/vtk .h ${VTK_BINARY_DIR}/vtkIOInstantiator)
ELSE(VTK_MAKE_INSTANTIATOR)
  ADD_LIBRARY(vtkIO IO_SRCS)
ENDIF(VTK_MAKE_INSTANTIATOR)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/IO/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/IO/LocalUserOptions.cmake OPTIONAL)

IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL(vtkIOTCL IOTCL_SRCS IO_SRCS)
  IF (APPLE)
    ADD_LIBRARY(vtkIOTCL SHARED IOTCL_SRCS)
  ELSE (APPLE)
    ADD_LIBRARY(vtkIOTCL IOTCL_SRCS)
  ENDIF (APPLE)
  TARGET_LINK_LIBRARIES (vtkIOTCL
                         vtkFilteringTCL
                         vtkCommonTCL 
                         ${TCL_LIBRARY})
  INSTALL_TARGETS(/lib/vtk vtkIOTCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON(vtkIOPython IOPython_SRCS IO_SRCS)
  IF (APPLE)
    ADD_LIBRARY(vtkIOPython MODULE IOPython_SRCS)
  ELSE (APPLE)
    ADD_LIBRARY(vtkIOPython SHARED IOPython_SRCS)
    TARGET_LINK_LIBRARIES (vtkIOPython
                           vtkFilteringPython
                           vtkCommonPython)
  ENDIF (APPLE)
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtkIOPython
                           debug ${PYTHON_DEBUG_LIBRARY}
                           optimized ${PYTHON_LIBRARY})
  ELSE(WIN32)
    TARGET_LINK_LIBRARIES (vtkIOPython ${PYTHON_LIBRARY})
  ENDIF(WIN32)
  INSTALL_TARGETS(/lib/vtk vtkIOPython)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA(vtkIOJava IOJava_SRCS IO_SRCS)
  ADD_LIBRARY(vtkIOJava SHARED IOJava_SRCS)
  TARGET_LINK_LIBRARIES (vtkIOJava
                         vtkCommonJava
                         vtkFilteringJava)
  INSTALL_TARGETS(/lib/vtk vtkIOJava)
ENDIF (VTK_WRAP_JAVA)

LINK_LIBRARIES (
vtkCommon
vtkFiltering
vtkIO
)

INSTALL_TARGETS(/lib/vtk vtkIO)
INSTALL_FILES(/include/vtk .h IO_SRCS)
