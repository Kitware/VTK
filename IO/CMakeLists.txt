SOURCE_FILES( IO_SRCS
vtkBMPReader
vtkBMPWriter
vtkBYUReader
vtkBYUWriter
vtkDataObjectReader
vtkDataObjectWriter
vtkDataReader
vtkDataSetReader
vtkDataSetWriter
vtkDataWriter
vtkDEMReader 
vtkEnSight6BinaryReader
vtkEnSight6Reader
vtkEnSightGoldBinaryReader
vtkEnSightGoldReader
vtkEnSightReader
vtkGenericEnSightReader
vtkImageReader
vtkImageReader2
vtkImageWriter
vtkJPEGWriter
vtkMCubesReader
vtkMCubesWriter
vtkOBJReader
vtkPLOT3DReader
vtkPNGReader
vtkPNGWriter
vtkPNMReader
vtkPNMWriter
vtkPolyDataReader
vtkPolyDataWriter
vtkPostScriptWriter
vtkRectilinearGridReader
vtkRectilinearGridWriter
vtkSLCReader
vtkSTLReader
vtkSTLWriter
vtkStructuredGridReader
vtkStructuredGridWriter
vtkStructuredPointsReader
vtkStructuredPointsWriter
vtkTIFFReader
vtkTIFFWriter
vtkUGFacetReader
vtkUnstructuredGridReader
vtkUnstructuredGridWriter
vtkVolume16Reader
vtkVolumeReader
vtkWriter
)

# Optional Stanford University PLY file format
IF(PLY_LIBRARY)
  IF(PLY_PATH)
    SOURCE_FILES( IO_SRCS vtkPLYReader vtkPLYWriter )
    INCLUDE_DIRECTORIES( ${PLY_PATH} )
  ENDIF(PLY_PATH)
ENDIF(PLY_LIBRARY)

ABSTRACT_FILES(
vtkEnSightReader
vtkVolumeReader
vtkWriter
)

ADD_LIBRARY(vtkIO IO_SRCS)
LINK_LIBRARIES ( png zlib jpeg )

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/IO/LocalUserOptions.cmake OPTIONAL) 
INCLUDE (${VTK_SOURCE_DIR}/IO/LocalUserOptions.cmake OPTIONAL) 

# if we are wrapping into Tcl then add the library and extra 
# source files
#
IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL(vtkIOTCL IOTCL_SRCS IO_SRCS)
  ADD_LIBRARY(vtkIOTCL IOTCL_SRCS)
  TARGET_LINK_LIBRARIES (vtkIOTCL 
                         vtkFilteringTCL 
                         vtkCommonTCL  
                         ${TCL_LIBRARY})
  INSTALL_TARGETS(/lib vtkIOTCL)
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
  TARGET_LINK_LIBRARIES (vtkIOPython 
                         debug ${PYTHON_DEBUG_LIBRARY} 
                         optimized ${PYTHON_LIBRARY})
  INSTALL_TARGETS(/lib vtkIOPython)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra 
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA(vtkIOJava IOJava_SRCS IO_SRCS)
  ADD_LIBRARY(vtkIOJava IOJava_SRCS)
  TARGET_LINK_LIBRARIES (vtkIOJava 
                         vtkCommonJava 
                         vtkFilteringJava)
  INSTALL_TARGETS(/lib vtkIOJava)
ENDIF (VTK_WRAP_JAVA)

LINK_LIBRARIES (
vtkCommon 
vtkFiltering
vtkIO
)

IF(WIN32)
  IF(UNIX)
    IF(NOT BUILD_SHARED_LIBS)
      ADD_DEFINITIONS(-DPNG_STATIC)
    ENDIF(NOT BUILD_SHARED_LIBS)
  ENDIF(UNIX)
ENDIF(WIN32)



INSTALL_TARGETS(/lib vtkIO)
INSTALL_FILES(/include/vtk .h IO_SRCS)
