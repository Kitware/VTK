SOURCE_FILES( IO_SRCS
vtk3DSImporter
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
vtkMCubesReader
vtkMCubesWriter
vtkOBJReader 
vtkPLOT3DReader
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

ABSTRACT_FILES(
vtkEnSightReader
vtkVolumeReader
vtkWriter
)


ADD_LIBRARY(vtkIO IO_SRCS)

# Allow the user to customize their build with some local options
#
INCLUDE (LocalUserOptions.cmake OPTIONAL)

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
  ADD_LIBRARY(vtkIOPython IOPython_SRCS)
  TARGET_LINK_LIBRARIES (vtkIOPython 
                         vtkFilteringPython 
                         vtkCommonPython)
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


INSTALL_TARGETS(/lib vtkIO)
INSTALL_FILES(/include/vtk .h IO_SRCS)
