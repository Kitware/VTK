SOURCE_FILES( IO_SRCS
vtkBMPReader
vtkBMPWriter
vtkBYUReader
vtkBYUWriter
vtkDataObjectReader
vtkDataObjectWriter
vtkDataSetReader
vtkDataSetWriter
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
vtkPLOT3DReader
vtkPNMReader
vtkPNMWriter
vtkPolyDataReader
vtkPolyDataWriter
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
vtkVectorText
vtkVolume16Reader
vtkVolumeReader
)

ABSTRACT_FILES(
vtkEnSightReader
vtkVolumeReader
)


ADD_LIBRARY(vtkIO IO_SRCS)

# if we are wrapping into Tcl then add the library and extra 
# source files
#
IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL(vtkIOTCL IOTCL_SRCS IO_SRCS)
  ADD_LIBRARY(vtkIOTCL IOTCL_SRCS)
  TARGET_LINK_LIBRARIES (vtkIOTCL vtkFilteringTCL vtkCommonTCL )
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Tcl then add the library and extra 
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON(vtkIOPython IOPython_SRCS IO_SRCS)
  ADD_LIBRARY(vtkIOPython IOPython_SRCS)
  TARGET_LINK_LIBRARIES (vtkIOPython vtkFilteringPython vtkCommonPython )
ENDIF (VTK_WRAP_PYTHON)

LINK_LIBRARIES (
vtkCommon 
vtkFiltering
vtkIO 
)


