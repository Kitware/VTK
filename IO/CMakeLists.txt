SET(KIT IO)
SET(UKIT IO)
SET(KIT_TCL_LIBS vtkFilteringTCL)
SET(KIT_PYTHON_LIBS vtkFilteringPythonD)
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
vtkChacoReader.cxx
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
vtkGenericMovieWriter.cxx
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
vtkMedicalImageProperties.cxx
vtkMedicalImageReader2.cxx
vtkMetaImageReader.cxx
vtkMetaImageWriter.cxx
vtkMultiBlockPLOT3DReader.cxx
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
vtkGenericMovieWriter
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

IF(WIN32)
  IF(VTK_USE_VIDEO_FOR_WINDOWS)
    SET(Kit_SRCS ${Kit_SRCS} vtkAVIWriter.cxx)
    SET(KIT_LIBS ${KIT_LIBS} vfw32)
  ENDIF(VTK_USE_VIDEO_FOR_WINDOWS)
ENDIF(WIN32)

# Include mpeg2 writer if the mpeg2 lib has been defined

IF(DEFINED vtkMPEG2Encode_INCLUDE_PATH)
  SET(Kit_SRCS ${Kit_SRCS} vtkMPEG2Writer.cxx)
  INCLUDE_DIRECTORIES(${vtkMPEG2Encode_INCLUDE_PATH})
  SET(KIT_LIBS ${KIT_LIBS} vtkMPEG2Encode)
ENDIF(DEFINED vtkMPEG2Encode_INCLUDE_PATH)

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
SET(KIT_TCL_DEPS)
SET(KIT_PYTHON_DEPS)
SET(KIT_JAVA_DEPS)

#-----------------------------------------------------------------------------
# Include CMake code common to all kits.
INCLUDE(${VTK_SOURCE_DIR}/CMake/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------
