SET( IO_SRCS
vtkBMPReader.cxx
vtkBMPWriter.cxx
vtkBYUReader.cxx
vtkBYUWriter.cxx
vtkBase64InputStream.cxx
vtkBase64OutputStream.cxx
vtkBase64Utilities.cxx
vtkDEMReader.cxx
vtkDataCompressor.cxx
vtkDataObjectReader.cxx
vtkDataObjectWriter.cxx
vtkDataReader.cxx
vtkDataSetReader.cxx
vtkDataSetWriter.cxx
vtkDataWriter.cxx
vtkEnSight6BinaryReader.cxx
vtkEnSight6Reader.cxx
vtkEnSightGoldBinaryReader.cxx
vtkEnSightGoldReader.cxx
vtkEnSightMasterServerReader.cxx
vtkEnSightReader.cxx
vtkGESignaReader.cxx
vtkGenericEnSightReader.cxx
vtkIVWriter.cxx
vtkImageReader.cxx
vtkImageReader2.cxx
vtkImageReader2Collection.cxx
vtkImageReader2Factory.cxx
vtkImageWriter.cxx
vtkInputStream.cxx
vtkJPEGReader.cxx
vtkJPEGWriter.cxx
vtkMCubesReader.cxx
vtkMCubesWriter.cxx
vtkMedicalImageReader2.cxx
vtkOBJReader.cxx
vtkOutputStream.cxx
vtkPLOT3DReader.cxx
vtkPLY.cxx
vtkPLYReader.cxx
vtkPLYWriter.cxx
vtkPNGReader.cxx
vtkPNGWriter.cxx
vtkPNMReader.cxx
vtkPNMWriter.cxx
vtkParticleReader.cxx
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
vtkXMLDataElement.cxx
vtkXMLDataParser.cxx
vtkXMLDataReader.cxx
vtkXMLDataSetWriter.cxx
vtkXMLFileReadTester.cxx
vtkXMLImageDataReader.cxx
vtkXMLImageDataWriter.cxx
vtkXMLPDataReader.cxx
vtkXMLPDataSetWriter.cxx
vtkXMLPDataWriter.cxx
vtkXMLPImageDataReader.cxx
vtkXMLPImageDataWriter.cxx
vtkXMLPPolyDataReader.cxx
vtkXMLPPolyDataWriter.cxx
vtkXMLPRectilinearGridReader.cxx
vtkXMLPRectilinearGridWriter.cxx
vtkXMLPStructuredDataReader.cxx
vtkXMLPStructuredDataWriter.cxx
vtkXMLPStructuredGridReader.cxx
vtkXMLPStructuredGridWriter.cxx
vtkXMLPUnstructuredDataReader.cxx
vtkXMLPUnstructuredDataWriter.cxx
vtkXMLPUnstructuredGridReader.cxx
vtkXMLPUnstructuredGridWriter.cxx
vtkXMLParser.cxx
vtkXMLPolyDataReader.cxx
vtkXMLPolyDataWriter.cxx
vtkXMLReader.cxx
vtkXMLRectilinearGridReader.cxx
vtkXMLRectilinearGridWriter.cxx
vtkXMLStructuredDataReader.cxx
vtkXMLStructuredDataWriter.cxx
vtkXMLStructuredGridReader.cxx
vtkXMLStructuredGridWriter.cxx
vtkXMLUnstructuredDataReader.cxx
vtkXMLUnstructuredDataWriter.cxx
vtkXMLUnstructuredGridReader.cxx
vtkXMLUnstructuredGridWriter.cxx
vtkXMLWriter.cxx
vtkZLibDataCompressor.cxx
)

SET_SOURCE_FILES_PROPERTIES(
vtkDataCompressor
vtkEnSightReader
vtkVolumeReader
vtkWriter
vtkXMLDataReader
vtkXMLPDataReader
vtkXMLPDataWriter
vtkXMLPStructuredDataReader
vtkXMLPStructuredDataWriter
vtkXMLPUnstructuredDataReader
vtkXMLPUnstructuredDataWriter
vtkXMLReader
vtkXMLStructuredDataReader
vtkXMLStructuredDataWriter
vtkXMLUnstructuredDataReader
vtkXMLUnstructuredDataWriter
vtkXMLWriter

ABSTRACT
)

# if we are wrapping into Tcl then add the library and extra
# source files
#
SET_SOURCE_FILES_PROPERTIES(vtkPLY WRAP_EXCLUDE)

# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR2(vtkIOInstantiator IOInstantiator_SRCS
                       ${IO_SRCS}
                       EXPORT_MACRO VTK_IO_EXPORT
                       HEADER_LOCATION ${VTK_BINARY_DIR})

ADD_LIBRARY(vtkIO ${IO_SRCS} ${IOInstantiator_SRCS})

IF(VTK_APPLE_SHARED_FLAGS_NEEDED)
  SET_TARGET_PROPERTIES(vtkIO PROPERTIES LINK_FLAGS "-init _oft_initIO")
ENDIF(VTK_APPLE_SHARED_FLAGS_NEEDED)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/IO/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/IO/LocalUserOptions.cmake OPTIONAL)

TARGET_LINK_LIBRARIES (vtkIO vtkFiltering 
  ${VTK_PNG_LIBRARIES} ${VTK_ZLIB_LIBRARIES} ${VTK_JPEG_LIBRARIES}
  ${VTK_TIFF_LIBRARIES} ${VTK_EXPAT_LIBRARIES})

IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL2(vtkIOTCL IOTCL_SRCS ${IO_SRCS})
  ADD_LIBRARY(vtkIOTCL ${IOTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkIOTCL
                         vtkFilteringTCL
                         vtkIO
                         ${TCL_LIBRARY})
  INSTALL_TARGETS(/lib/vtk vtkIOTCL)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON2(vtkIOPython IOPython_SRCS ${IO_SRCS})
  ADD_LIBRARY(vtkIOPython MODULE ${IOPython_SRCS})
  IF(NOT APPLE)
    TARGET_LINK_LIBRARIES (vtkIOPython
                           vtkFilteringPython
                           vtkCommonPython)
  ENDIF(NOT APPLE)
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtkIOPython
                           debug ${PYTHON_DEBUG_LIBRARY}
                           optimized ${PYTHON_LIBRARY})
  ENDIF(WIN32)
  TARGET_LINK_LIBRARIES(vtkIOPython vtkIO)

  INSTALL_TARGETS(/lib/vtk vtkIOPython)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA2(vtkIOJava IOJava_SRCS ${IO_SRCS})
  ADD_LIBRARY(vtkIOJava SHARED ${IOJava_SRCS})
  TARGET_LINK_LIBRARIES (vtkIOJava
                         vtkCommonJava
                         vtkFilteringJava
			 vtkIO)
  INSTALL_TARGETS(/lib/vtk vtkIOJava)
ENDIF (VTK_WRAP_JAVA)


INSTALL_TARGETS(/lib/vtk vtkIO)
INSTALL_FILES(/include/vtk .h ${IO_SRCS})
