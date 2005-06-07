SET(KIT IO)
SET(UKIT IO)
SET(KIT_TCL_LIBS vtkFilteringTCL)
IF(APPLE)
  SET(KIT_PYTHON_LIBS vtkFilteringPythonD)
ELSE(APPLE)
  SET(KIT_PYTHON_LIBS vtkFilteringPython)
ENDIF(APPLE)
SET(KIT_JAVA_LIBS vtkFilteringJava)
SET(KIT_LIBS vtkFiltering vtkDICOMParser
  ${VTK_PNG_LIBRARIES} ${VTK_ZLIB_LIBRARIES} ${VTK_JPEG_LIBRARIES}
  ${VTK_TIFF_LIBRARIES} ${VTK_EXPAT_LIBRARIES})


SET( Kit_SRCS
vtkAVSucdReader.cxx
vtkBMPReader.cxx
vtkBMPWriter.cxx
vtkBYUReader.cxx
vtkBYUWriter.cxx
vtkBase64InputStream.cxx
vtkBase64OutputStream.cxx
vtkBase64Utilities.cxx
vtkCGMWriter.cxx
vtkDEMReader.cxx
vtkDICOMImageReader.cxx
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
vtkFacetWriter.cxx
vtkGESignaReader.cxx
vtkGAMBITReader.cxx
vtkGaussianCubeReader.cxx
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
vtkMetaImageReader.cxx
vtkMetaImageWriter.cxx
vtkMoleculeReaderBase.cxx
vtkOBJReader.cxx
vtkOutputStream.cxx
vtkPDBReader.cxx
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
vtkXMLUtilities.cxx
vtkXMLWriter.cxx
vtkXMLWriterC.cxx
vtkXYZMolReader.cxx
vtkZLibDataCompressor.cxx
)

SET_SOURCE_FILES_PROPERTIES(
vtkDataCompressor
vtkEnSightReader
vtkMoleculeReaderBase
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

SET_SOURCE_FILES_PROPERTIES(
  vtkPLY
  vtkXMLWriterC
  WRAP_EXCLUDE
)
SET(Kit_EXTRA_SRCS)
SET(Kit_EXTRA_CMDS)
SET(Kit_TCL_EXTRA_SRCS)
SET(Kit_PYTHON_EXTRA_SRCS)
SET(Kit_JAVA_EXTRA_SRCS)

# -----------------------------------------------------------------
# Start of common section

# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR3(vtk${KIT}Instantiator KitInstantiator_SRCS
                       "${Kit_SRCS}"
                       VTK_${UKIT}_EXPORT
                       ${VTK_BINARY_DIR} "")

ADD_LIBRARY(vtk${KIT} ${Kit_SRCS} ${Kit_EXTRA_SRCS} ${KitInstantiator_SRCS})

# Allow the user to customize their build with some local options
#
SET(LOCALUSERMACRODEFINED 0)
INCLUDE (${VTK_BINARY_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)

# if we are wrapping into Tcl then add the library and extra
# source files
#
IF (VTK_WRAP_TCL)
  VTK_WRAP_TCL3(vtk${KIT}TCL KitTCL_SRCS 
                "${Kit_SRCS}"
                "${Kit_TCL_EXTRA_CMDS}")
  ADD_LIBRARY(vtk${KIT}TCL ${KitTCL_SRCS} ${Kit_TCL_EXTRA_SRCS})
  TARGET_LINK_LIBRARIES (vtk${KIT}TCL vtk${KIT} ${KIT_TCL_LIBS})
  IF(NOT VTK_INSTALL_NO_LIBRARIES)
    INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT}TCL)
  ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra
# source files
#
IF (VTK_WRAP_PYTHON)
  VTK_WRAP_PYTHON3(vtk${KIT}Python KitPython_SRCS "${Kit_SRCS}")
  IF (APPLE)
    ADD_LIBRARY(vtk${KIT}PythonD ${KitPython_SRCS} ${Kit_PYTHON_EXTRA_SRCS})
    ADD_LIBRARY(vtk${KIT}Python MODULE vtk${KIT}PythonInit.cxx)
    TARGET_LINK_LIBRARIES(vtk${KIT}PythonD vtk${KIT} ${KIT_PYTHON_LIBS}
                          ${VTK_PYTHON_LIBRARIES})
    TARGET_LINK_LIBRARIES(vtk${KIT}Python vtk${KIT}PythonD)
    IF(NOT VTK_INSTALL_NO_LIBRARIES)
      INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT}PythonD)
    ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
  ELSE (APPLE)
    ADD_LIBRARY(vtk${KIT}Python MODULE ${KitPython_SRCS} ${Kit_PYTHON_EXTRA_SRCS})
    TARGET_LINK_LIBRARIES (vtk${KIT}Python vtk${KIT} ${KIT_PYTHON_LIBS})
  ENDIF(APPLE)
  IF(WIN32)
    TARGET_LINK_LIBRARIES (vtk${KIT}Python ${VTK_PYTHON_LIBRARIES})
  ENDIF(WIN32)
  IF(NOT VTK_INSTALL_NO_LIBRARIES)
    INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT}Python)
  ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra
# source files
#
IF (VTK_WRAP_JAVA)
  VTK_WRAP_JAVA3(vtk${KIT}Java KitJava_SRCS "${Kit_SRCS}")
  ADD_LIBRARY(vtk${KIT}Java SHARED ${KitJava_SRCS} ${Kit_JAVA_EXTRA_SRCS})
  TARGET_LINK_LIBRARIES(vtk${KIT}Java vtk${KIT} ${KIT_JAVA_LIBS})
  IF(NOT VTK_INSTALL_NO_LIBRARIES)
    INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT}Java)
  ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
ENDIF (VTK_WRAP_JAVA)

TARGET_LINK_LIBRARIES(vtk${KIT} ${KIT_LIBS})

IF(NOT VTK_INSTALL_NO_LIBRARIES)
  INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtk${KIT})
ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} .h ${Kit_SRCS})
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

VTK_EXPORT_KIT("${KIT}" "${UKIT}" "${Kit_SRCS}")

# If the user defined a custom macro, execute it now and pass in all the srcs
#
IF(LOCALUSERMACRODEFINED)
  LocalUserOptionsMacro( "${Kit_SRCS}"       "${Kit_EXTRA_SRCS}"
                         "${KitTCL_SRCS}"    "${Kit_TCL_EXTRA_SRCS}"
                         "${KitJava_SRCS}"   "${Kit_JAVA_EXTRA_SRCS}"
                         "${KitPython_SRCS}" "${Kit_PYTHON_EXTRA_SRCS}")
ENDIF(LOCALUSERMACRODEFINED)

# End of common section
# -----------------------------------------------------------------
