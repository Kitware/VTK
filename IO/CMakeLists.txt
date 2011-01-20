IF(VTK_USE_METAIO)
  SET(_VTK_METAIO_LIB vtkmetaio)
  SET(_VTK_METAIO_SOURCES vtkMetaImageReader.cxx vtkMetaImageWriter.cxx )
ELSE(VTK_USE_METAIO)
  SET(_VTK_METAIO_LIB)
  SET(_VTK_METAIO_SOURCES)
ENDIF(VTK_USE_METAIO)

IF(VTK_USE_OGGTHEORA_ENCODER)
  set(_VTK_OGGTHEORA_SOURCES vtkOggTheoraWriter.cxx)
ELSE(VTK_USE_OGGTHEORA_ENCODER)
  set(_VTK_OGGTHEORA_SOURCES)
ENDIF(VTK_USE_OGGTHEORA_ENCODER)

SET(KIT IO)
SET(UKIT IO)
SET(KIT_TCL_LIBS vtkFilteringTCL)
SET(KIT_PYTHON_LIBS vtkFilteringPythonD)
SET(KIT_JAVA_LIBS vtkFilteringJava)
SET(KIT_INTERFACE_LIBRARIES vtkFiltering)
SET(KIT_LIBS vtkDICOMParser vtkNetCDF vtkNetCDF_cxx
  ${_VTK_METAIO_LIB} vtksqlite
  ${VTK_PNG_LIBRARIES} ${VTK_ZLIB_LIBRARIES} ${VTK_JPEG_LIBRARIES}
  ${VTK_TIFF_LIBRARIES} ${VTK_EXPAT_LIBRARIES} ${VTK_OGGTHEORA_LIBRARIES}
  ${KWSYS_NAMESPACE})

SET( Kit_SRCS
vtkAbstractParticleWriter.cxx
vtkASCIITextCodec.cxx
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
vtkCompositeDataReader.cxx
vtkCompositeDataWriter.cxx
vtkDatabaseToTableReader.cxx
vtkDEMReader.cxx
vtkDICOMImageReader.cxx
vtkDataCompressor.cxx
vtkDataObjectReader.cxx
vtkDataObjectWriter.cxx
vtkDataReader.cxx
vtkDataSetReader.cxx
vtkDataSetWriter.cxx
vtkDataWriter.cxx
vtkDelimitedTextWriter.cxx
vtkEnSight6BinaryReader.cxx
vtkEnSight6Reader.cxx
vtkEnSightGoldBinaryReader.cxx
vtkEnSightGoldReader.cxx
vtkEnSightMasterServerReader.cxx
vtkEnSightReader.cxx
vtkFLUENTReader.cxx
vtkFacetWriter.cxx
vtkGESignaReader.cxx
vtkGAMBITReader.cxx
vtkGaussianCubeReader.cxx
vtkGenericDataObjectReader.cxx
vtkGenericDataObjectWriter.cxx
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
vtkJavaScriptDataWriter.cxx
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
vtkMPASReader.cxx
vtkMultiBlockPLOT3DReader.cxx
vtkMoleculeReaderBase.cxx
vtkNetCDFCFReader.cxx
vtkNetCDFPOPReader.cxx
vtkNetCDFReader.cxx
vtkOBJReader.cxx
${_VTK_OGGTHEORA_SOURCES}
vtkOpenFOAMReader.cxx
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
vtkProStarReader.cxx
vtkRectilinearGridReader.cxx
vtkRectilinearGridWriter.cxx
vtkRTXMLPolyDataReader.cxx
vtkRowQuery.cxx
vtkSESAMEReader.cxx
vtkShaderCodeLibrary.cxx
vtkSLACParticleReader.cxx
vtkSLACReader.cxx
vtkSQLDatabase.cxx
vtkSQLDatabaseSchema.cxx
vtkSQLiteDatabase.cxx
vtkSQLiteToTableReader.cxx
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
vtkTableToDatabaseWriter.cxx
vtkTableToSQLiteWriter.cxx
vtkTecplotReader.cxx
vtkTextCodec.cxx
vtkTextCodecFactory.cxx
vtkTIFFReader.cxx
vtkTIFFWriter.cxx
vtkTreeReader.cxx
vtkTreeWriter.cxx
vtkUGFacetReader.cxx
vtkUnstructuredGridReader.cxx
vtkUnstructuredGridWriter.cxx
vtkUTF8TextCodec.cxx
vtkUTF16TextCodec.cxx
vtkVolume16Reader.cxx
vtkVolumeReader.cxx
vtkWriter.cxx
vtkXMLCompositeDataReader.cxx
vtkXMLDataParser.cxx
vtkXMLDataReader.cxx
vtkXMLDataSetWriter.cxx
vtkXMLFileReadTester.cxx
vtkXMLGenericDataObjectReader.cxx
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
vtkDatabaseToTableReader
vtkDataCompressor
vtkEnSightReader
vtkGenericMovieWriter
vtkMoleculeReaderBase
vtkRowQuery
vtkSQLDatabase
vtkSQLQuery
vtkTableToDatabaseWriter
vtkTextCodec
vtkTextCodecFactory
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

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/vtkMINC.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}
    COMPONENT Development
    )
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

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
  INCLUDE(${VTK_CMAKE_DIR}/vtkTestFFMPEG.cmake)

  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/vtkFFMPEGConfig.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/vtkFFMPEGConfig.h @ONLY)
  INCLUDE_DIRECTORIES(${VTK_BINARY_DIR}/IO)

  IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/vtkFFMPEGConfig.h
      DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}
      COMPONENT Development)
  ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

  INCLUDE_DIRECTORIES(${FFMPEG_INCLUDE_DIR})
  SET(Kit_SRCS ${Kit_SRCS} vtkFFMPEGWriter.cxx)

  SET_SOURCE_FILES_PROPERTIES( vtkFFMPEGWriter.cxx
    PROPERTIES COMPILE_DEFINITIONS __STDC_CONSTANT_MACROS )

  SET(KIT_LIBS ${KIT_LIBS}
    ${FFMPEG_avformat_LIBRARY}
    ${FFMPEG_avcodec_LIBRARY}
    ${FFMPEG_avutil_LIBRARY})
  IF (NOT VTK_FFMPEG_HAS_IMG_CONVERT)
    SET(KIT_LIBS ${KIT_LIBS}
      ${FFMPEG_swscale_LIBRARY})
  ENDIF (NOT VTK_FFMPEG_HAS_IMG_CONVERT)
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
  FIND_PACKAGE( POSTGRES REQUIRED )
  IF ( BUILD_TESTING )
    SET ( VTK_PSQL_TEST_URL "" CACHE STRING "A URL for a PostgreSQL server of the form psql://[[username[:password]@]hostname[:port]]/[dbname]" ) 
  ENDIF ( BUILD_TESTING )
ENDIF ( VTK_USE_POSTGRES )
#
# Only include the Postgres backend if the user has explicitly turned on
# VTK_USE_POSTGRES. If so, user must also have set POSTGRES_LIBRARIES
# and POSTGRES_INCLUDE_DIRECTORIES to valid values.
IF ( VTK_USE_POSTGRES AND POSTGRES_FOUND )
  INCLUDE_DIRECTORIES( ${POSTGRES_INCLUDE_DIRECTORIES} )
  SET( Kit_SRCS
    ${Kit_SRCS}
    vtkPostgreSQLDatabase.cxx
    vtkPostgreSQLQuery.cxx
    vtkPostgreSQLToTableReader.cxx
    vtkTableToPostgreSQLWriter.cxx
  )
  LINK_LIBRARIES( ${POSTGRES_LIBRARIES} )
ENDIF ( VTK_USE_POSTGRES AND POSTGRES_FOUND )

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
    vtkMySQLToTableReader.cxx
    vtkTableToMySQLWriter.cxx
  )
  LINK_LIBRARIES( ${MYSQL_LIBRARY}  ${MYSQL_EXTRA_LIBRARIES} )
ENDIF ( VTK_USE_MYSQL AND MYSQL_FOUND )

#-----------------------------------------------------------------------------
# ODBC
# Build the ODBC database backend only if ODBC driver manager is available
#
OPTION (VTK_USE_ODBC "Build the ODBC database interface" OFF)
MARK_AS_ADVANCED(VTK_USE_ODBC)
IF ( VTK_USE_ODBC )
  FIND_PACKAGE( ODBC REQUIRED )
  IF ( BUILD_TESTING )
    SET ( VTK_ODBC_TEST_DSN "" CACHE STRING "A data source name (DSN) for an ODBC database connection to use for testing." )
  ENDIF ( BUILD_TESTING )
ENDIF ( VTK_USE_ODBC )

IF ( VTK_USE_ODBC AND ODBC_FOUND )
  INCLUDE_DIRECTORIES( ${ODBC_INCLUDE_DIRECTORIES} )
  SET( Kit_SRCS
    ${Kit_SRCS}
    vtkODBCDatabase.cxx
    vtkODBCQuery.cxx
    )
  LINK_LIBRARIES( ${ODBC_LIBRARIES} )
  # Note: Strange things happen on 64-bit machines.  We must compile
  # ODBC code with -DBUILD_REAL_64_BIT_MODE in order to have the driver
  # work properly.
  INCLUDE(CheckTypeSize)
  CHECK_TYPE_SIZE("unsigned long" CMAKE_SIZEOF_LONG)
  IF ( "${CMAKE_SIZEOF_LONG}" STREQUAL "8" )
#    MESSAGE( STATUS "Building ODBC driver with 64-bit flag." )
    ADD_DEFINITIONS( -DBUILD_REAL_64_BIT_MODE )
  ENDIF ( "${CMAKE_SIZEOF_LONG}"  STREQUAL "8" )
ENDIF ( VTK_USE_ODBC AND ODBC_FOUND )

#-----------------------------------------------------------------------------
# Add these classes only if vtkTypeUInt64Array exists
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

#-----------------------------------------------------------------------------
# N-WAY ARRAYS READER AND WRITER
IF(VTK_USE_N_WAY_ARRAYS)

  SET(Kit_SRCS
    ${Kit_SRCS}
    vtkArrayReader.cxx
    vtkArrayWriter.cxx
    )

ENDIF(VTK_USE_N_WAY_ARRAYS)
#-----------------------------------------------------------------------------

INCLUDE_DIRECTORIES(${VTKNETCDF_BINARY_DIR})

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
