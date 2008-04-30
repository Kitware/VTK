IF(VTK_USE_METAIO)
  SET(_VTK_METAIO_LIB vtkmetaio)
  SET(_VTK_METAIO_SOURCES vtkMetaImageReader.cxx vtkMetaImageWriter.cxx )
ELSE(VTK_USE_METAIO)
  SET(_VTK_METAIO_LIB)
  SET(_VTK_METAIO_SOURCES)
ENDIF(VTK_USE_METAIO)

SET(KIT IO)
SET(UKIT IO)
SET(KIT_TCL_LIBS vtkFilteringTCL)
SET(KIT_PYTHON_LIBS vtkFilteringPythonD)
SET(KIT_JAVA_LIBS vtkFilteringJava)
SET(KIT_LIBS vtkFiltering vtkDICOMParser vtkNetCDF ${_VTK_METAIO_LIB} vtksqlite
  ${VTK_PNG_LIBRARIES} ${VTK_ZLIB_LIBRARIES} ${VTK_JPEG_LIBRARIES}
  ${VTK_TIFF_LIBRARIES} ${VTK_EXPAT_LIBRARIES}
  ${KWSYS_NAMESPACE})

SET( Kit_SRCS
vtkAbstractParticleWriter.cxx
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
vtkCosmoReader.cxx
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
vtkFLUENTReader.cxx
vtkFacetWriter.cxx
vtkGenericDataObjectReader.cxx
vtkGenericDataObjectWriter.cxx
vtkGESignaReader.cxx
vtkGAMBITReader.cxx
vtkGaussianCubeReader.cxx
vtkGenericEnSightReader.cxx
vtkGenericMovieWriter.cxx
vtkGlobFileNames.cxx
vtkGraphReader.cxx
vtkGraphWriter.cxx
vtkIVWriter.cxx
vtkImageReader.cxx
vtkImageReader2.cxx
vtkImageReader2Collection.cxx
vtkImageReader2Factory.cxx
vtkImageWriter.cxx
vtkInputStream.cxx
vtkJPEGReader.cxx
vtkJPEGWriter.cxx
vtkMFIXReader.cxx
vtkMaterialLibrary.cxx
vtkMCubesReader.cxx
vtkMCubesWriter.cxx
vtkMedicalImageProperties.cxx
vtkMedicalImageReader2.cxx
${_VTK_METAIO_SOURCES}
vtkMINCImageAttributes.cxx
vtkMINCImageReader.cxx
vtkMINCImageWriter.cxx
vtkMultiBlockPLOT3DReader.cxx
vtkMoleculeReaderBase.cxx
vtkOBJReader.cxx
vtkOutputStream.cxx
vtkOpenFOAMReader.cxx
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
vtkRTXMLPolyDataReader.cxx
vtkRowQuery.cxx
vtkSESAMEReader.cxx
vtkShaderCodeLibrary.cxx
vtkSQLDatabase.cxx
vtkSQLDatabaseSchema.cxx
vtkSQLiteDatabase.cxx
vtkSQLiteQuery.cxx
vtkSQLQuery.cxx
vtkSLCReader.cxx
vtkSTLReader.cxx
vtkSTLWriter.cxx
vtkSimplePointsReader.cxx
vtkSortFileNames.cxx
vtkStructuredGridReader.cxx
vtkStructuredGridWriter.cxx
vtkStructuredPointsReader.cxx
vtkStructuredPointsWriter.cxx
vtkTableReader.cxx
vtkTableWriter.cxx
vtkTIFFReader.cxx
vtkTIFFWriter.cxx
vtkTreeReader.cxx
vtkTreeWriter.cxx
vtkUGFacetReader.cxx
vtkUnstructuredGridReader.cxx
vtkUnstructuredGridWriter.cxx
vtkVolume16Reader.cxx
vtkVolumeReader.cxx
vtkWriter.cxx
vtkXMLCompositeDataReader.cxx
vtkXMLDataElement.cxx
vtkXMLDataParser.cxx
vtkXMLDataReader.cxx
vtkXMLDataSetWriter.cxx
vtkXMLFileReadTester.cxx
vtkXMLHierarchicalBoxDataReader.cxx
vtkXMLHierarchicalDataReader.cxx
vtkXMLImageDataReader.cxx
vtkXMLImageDataWriter.cxx
vtkXMLHyperOctreeReader.cxx
vtkXMLHyperOctreeWriter.cxx
vtkXMLMaterial.cxx
vtkXMLMaterialParser.cxx
vtkXMLMaterialReader.cxx
vtkXMLMultiBlockDataReader.cxx
vtkXMLMultiGroupDataReader.cxx
vtkXMLCompositeDataWriter.cxx
vtkXMLMultiBlockDataWriter.cxx
vtkXMLHierarchicalBoxDataWriter.cxx
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
vtkXMLShader.cxx
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
vtkAbstractParticleWriter
vtkDataCompressor
vtkEnSightReader
vtkGenericMovieWriter
vtkMoleculeReaderBase
vtkRowQuery
vtkSQLDatabase
vtkSQLQuery
vtkVolumeReader
vtkWriter
vtkXMLCompositeDataReader.cxx
vtkXMLCompositeDataWriter
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

INSTALL(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/vtkMINC.h
  DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}
  COMPONENT Development
  )

# Set up dependencies on the header files generated by 
# Utilities/MaterialLibrary
SET_SOURCE_FILES_PROPERTIES(vtkXMLShader.cxx PROPERTIES OBJECT_DEPENDS
  ${MATERIAL_LIBRARY_SHADER_MACRO_HEADER})
SET_SOURCE_FILES_PROPERTIES(vtkXMLMaterial.cxx PROPERTIES OBJECT_DEPENDS
  ${MATERIAL_LIBRARY_MATERIAL_MACRO_HEADER})

IF(WIN32)
  IF(VTK_USE_VIDEO_FOR_WINDOWS)
    SET(Kit_SRCS ${Kit_SRCS} vtkAVIWriter.cxx)
    SET(KIT_LIBS ${KIT_LIBS} vfw32)
  ENDIF(VTK_USE_VIDEO_FOR_WINDOWS)
ENDIF(WIN32)

#-----------------------------------------------------------------------------
# FFMPEG
# If the ffmpeg library is available, compile vtkFFMPEGWriter.
IF (VTK_USE_FFMPEG_ENCODER)
  INCLUDE_DIRECTORIES(${FFMPEG_INCLUDE_DIR})
  SET(Kit_SRCS ${Kit_SRCS} vtkFFMPEGWriter.cxx)
  SET(KIT_LIBS ${KIT_LIBS}  
    ${FFMPEG_avcodec_LIBRARY} 
    ${FFMPEG_avformat_LIBRARY}
    ${FFMPEG_avutil_LIBRARY})
ENDIF (VTK_USE_FFMPEG_ENCODER)

#-----------------------------------------------------------------------------
# MPEG2
#
# Only include the vtkMPEG2Writer if the user has explicitly turned on
# VTK_USE_MPEG2_ENCODER. If so, user should also have set
# vtkMPEG2Encode_INCLUDE_PATH and vtkMPEG2Encode_LIBRARIES.
#
IF (VTK_USE_MPEG2_ENCODER)
  INCLUDE_DIRECTORIES(${vtkMPEG2Encode_INCLUDE_PATH})
  SET(Kit_SRCS ${Kit_SRCS} vtkMPEG2Writer.cxx)
  SET(KIT_LIBS ${KIT_LIBS} ${vtkMPEG2Encode_LIBRARIES})
ENDIF (VTK_USE_MPEG2_ENCODER)

#-----------------------------------------------------------------------------
# PostgreSQL
# build the PostgreSQL database backend (if the PostgresSQL and pqxx libraries are available)
#
OPTION( VTK_USE_POSTGRES "Build the PostgreSQL driver for vtkSQLDatabase." OFF)
MARK_AS_ADVANCED(VTK_USE_POSTGRES)
IF ( VTK_USE_POSTGRES )
  FIND_PACKAGE( PQXX REQUIRED )
  IF ( BUILD_TESTING )
    SET ( VTK_PSQL_TEST_URL "" CACHE STRING "A URL for a PostgreSQL server of the form psql://[[username[:password]@]hostname[:port]]/[dbname]" ) 
  ENDIF ( BUILD_TESTING )
ENDIF ( VTK_USE_POSTGRES )
#
# Only include the Postgres backend if the user has explicitly turned on
# VTK_USE_POSTGRES. If so, user must also have set POSTGRES_LIBRARIES
# and POSTGRES_INCLUDE_DIRECTORIES to valid values.
IF ( VTK_USE_POSTGRES AND PQXX_FOUND )
  INCLUDE_DIRECTORIES( ${PQXX_INCLUDE_DIRECTORIES} )
  SET( Kit_SRCS
    ${Kit_SRCS}
    vtkPostgreSQLDatabase.cxx
    vtkPostgreSQLQuery.cxx
  )
  LINK_LIBRARIES( ${PQXX_LIBRARIES} )
ENDIF ( VTK_USE_POSTGRES AND PQXX_FOUND )

#-----------------------------------------------------------------------------
# MySQL
# build the MySQL database backend (if the MySQL libraries are available)
#
OPTION( VTK_USE_MYSQL "Build the MySQL driver for vtkSQLDatabase." OFF)
MARK_AS_ADVANCED(VTK_USE_MYSQL)
IF ( VTK_USE_MYSQL )
  FIND_PACKAGE( MySQL REQUIRED )
  IF ( BUILD_TESTING )
    SET ( VTK_MYSQL_TEST_URL "" CACHE STRING "A URL for a MySQL server of the form mysql://[[username[:password]@]hostname[:port]]/[dbname]" ) 
  ENDIF ( BUILD_TESTING )
ENDIF ( VTK_USE_MYSQL )
#
# Only include the MySQL backend if the user has explicitly turned on
# VTK_USE_MYSQL. If so, user must also have set MYSQL_LIBRARIES
# and MYSQL_INCLUDE_DIRECTORIES to valid values.
IF ( VTK_USE_MYSQL AND MYSQL_FOUND )
  INCLUDE_DIRECTORIES( ${MYSQL_INCLUDE_DIRECTORIES} )
  SET( Kit_SRCS
    ${Kit_SRCS}
    vtkMySQLDatabase.cxx
    vtkMySQLQuery.cxx
  )
  LINK_LIBRARIES( ${MYSQL_LIBRARY}  ${MYSQL_EXTRA_LIBRARIES} )
ENDIF ( VTK_USE_MYSQL AND MYSQL_FOUND )

#-----------------------------------------------------------------------------
# Add these classes only if vtkTypeUInt64Array exists
SET(VTK_HAS_UINT64_ARRAY 0)
IF(VTK_SIZEOF_LONG_LONG MATCHES "^8$")
  SET(VTK_HAS_UINT64_ARRAY 1)
ELSE(VTK_SIZEOF_LONG_LONG MATCHES "^8$")
  IF(VTK_SIZEOF_LONG MATCHES "^8$")
    SET(VTK_HAS_UINT64_ARRAY 1)
  ELSE(VTK_SIZEOF_LONG MATCHES "^8$")
    IF(VTK_SIZEOF___INT64 MATCHES "^8$")
      IF(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
        SET(VTK_HAS_UINT64_ARRAY 1)
      ENDIF(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
    ENDIF(VTK_SIZEOF___INT64 MATCHES "^8$")
  ENDIF(VTK_SIZEOF_LONG MATCHES "^8$")
ENDIF(VTK_SIZEOF_LONG_LONG MATCHES "^8$")
IF(VTK_HAS_UINT64_ARRAY)
  SET(Kit_SRCS
    ${Kit_SRCS}
    vtkRowQueryToTable.cxx
  )
ENDIF(VTK_HAS_UINT64_ARRAY)


#-----------------------------------------------------------------------------
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
INCLUDE(${VTK_CMAKE_DIR}/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------
# Add target dependency.
ADD_DEPENDENCIES(vtkIO
  vtkMaterialLibraryConfiguredFiles)
#-----------------------------------------------------------------------------
