# Build kwsys
ADD_SUBDIRECTORY(kwsys)
SET_PROPERTY(GLOBAL APPEND PROPERTY VTK_TARGETS vtksys)

# Build third-party utilities.

VTK_THIRD_PARTY_SUBDIR(ZLIB vtkzlib)
IF(VTK_USE_GL2PS)
  VTK_THIRD_PARTY_SUBDIR(GL2PS vtkgl2ps)
ENDIF(VTK_USE_GL2PS)
VTK_THIRD_PARTY_SUBDIR(JPEG vtkjpeg)
VTK_THIRD_PARTY_SUBDIR(PNG  vtkpng)
VTK_THIRD_PARTY_SUBDIR(TIFF vtktiff)
VTK_THIRD_PARTY_SUBDIR(EXPAT vtkexpat)
VTK_THIRD_PARTY_SUBDIR(FREETYPE vtkfreetype)
VTK_THIRD_PARTY_SUBDIR(LIBXML2 vtklibxml2)
VTK_THIRD_PARTY_SUBDIR(DICOMParser DICOMParser)
VTK_THIRD_PARTY_SUBDIR(MATERIALLIBRARY MaterialLibrary)
VTK_THIRD_PARTY_SUBDIR(LIBPROJ4 vtklibproj4)
VTK_THIRD_PARTY_SUBDIR(MRMPI mrmpi)
IF(VTK_USE_OGGTHEORA_ENCODER)
  VTK_THIRD_PARTY_SUBDIR(OGGTHEORA vtkoggtheora)
ENDIF(VTK_USE_OGGTHEORA_ENCODER)

#########################################################################
# Configure HDF5
OPTION(VTK_USE_SYSTEM_HDF5 "Use system installed HDF5" OFF)
MARK_AS_ADVANCED(VTK_USE_SYSTEM_HDF5)
IF(VTK_USE_SYSTEM_HDF5)

  #INCLUDE(${ParaView_CMAKE_DIR}/FindHDF5.cmake)
  #INCLUDE(${ParaView_CMAKE_DIR}/FindZLIB.cmake)
  SET(VTK_HDF5_LIBRARIES ${HDF5_LIBRARIES})

ELSE(VTK_USE_SYSTEM_HDF5)

  # Tell hdf5 that we are manually overriding certain settings
  SET(HDF5_EXTERNALLY_CONFIGURED ON)
  # Avoid duplicating names of installed libraries
  SET(HDF5_EXTERNAL_LIB_PREFIX "vtk")
  # Export configuration to this export variable
  SET(HDF5_EXPORTED_TARGETS ${VTK_INSTALL_EXPORT_NAME})

  # Silence HDF5's warnings. We'll let them get fixed upstream
  # and merge in updates as necessary.
  SET(HDF5_DISABLE_COMPILER_WARNINGS ON CACHE BOOL "Disable HDF5 warnings" FORCE)

  SET(HDF5_INSTALL_NO_DEVELOPMENT ${VTK_INSTALL_NO_DEVELOPMENT})
  SET(HDF5_INSTALL_BIN_DIR ${VTK_INSTALL_BIN_DIR})
  SET(HDF5_INSTALL_LIB_DIR ${VTK_INSTALL_LIB_DIR})
  SET(HDF5_INSTALL_INCLUDE_DIR ${VTK_INSTALL_INCLUDE_DIR})

  SET(HDF5_ENABLE_Z_LIB_SUPPORT ON CACHE BOOL "Enable Zlib Filters" FORCE)

  # Setup all necessary overrides for zlib so that HDF5 uses our
  # internally compiled zlib rather than any other version
  IF(HDF5_ENABLE_Z_LIB_SUPPORT)
    # We must tell the main HDF5 library that it depends on our zlib
    SET(HDF5_LIB_DEPENDENCIES vtkzlib)
    # Override the zlib header file
    IF(VTK_USE_SYSTEM_ZLIB)
      SET(H5_ZLIB_HEADER "zlib.h")
    ELSE(VTK_USE_SYSTEM_ZLIB)
      SET(H5_ZLIB_HEADER "vtk_zlib.h")
      # Set vars that FindZlib would have set if used in sub project
      SET(ZLIB_INCLUDE_DIRS "${VTK_ZLIB_INCLUDE_DIRS}")
      SET(ZLIB_LIBRARIES vtkzlib)
    ENDIF(VTK_USE_SYSTEM_ZLIB)
  ENDIF(HDF5_ENABLE_Z_LIB_SUPPORT)

  # we don't want to build HDF5's tests.
  SET (__vtk_build_testing ${BUILD_TESTING})
  SET (BUILD_TESTING OFF)

  # Add the sub project
  ADD_SUBDIRECTORY(vtkhdf5)

  # restore BUILD_TESTING
  SET (BUILD_TESTING ${__vtk_build_testing})

  SET(HDF5_INCLUDE_DIR
     ${VTK_SOURCE_DIR}/Utilities/vtkhdf5/src
     ${VTK_BINARY_DIR}/Utilities/vtkhdf5
     ${VTK_SOURCE_DIR}/Utilities/vtkhdf5/hl/src
     CACHE INTERNAL "")

  # Some other modules use these vars to get the hdf5 lib name(s)
  SET(VTK_HDF5_LIBRARIES
    vtkhdf5
#    ${HDF5_CPP_LIB_NAME}
#    ${HDF5_HL_LIB_NAME}
#    ${HDF5_HL_CPP_LIB_NAME}
    CACHE INTERNAL ""
    )
  SET(HDF5_LIBRARIES ${VTK_HDF5_LIBRARIES} CACHE INTERNAL "")

  # Add the HDF5 dirs to our include path
  SET(HDF5_INCLUDE_DIR
    ${VTK_SOURCE_DIR}/Utilities/vtkhdf5/src
    ${VTK_BINARY_DIR}/Utilities/vtkhdf5)

  MARK_AS_ADVANCED(
    H5_SET_LIB_OPTIONS
    H5_LEGACY_NAMING
    HDF5_ENABLE_COVERAGE
    HDF5_DISABLE_COMPILER_WARNINGS
    HDF5_ENABLE_PARALLEL
    HDF5_USE_16_API_DEFAULT
    HDF5_USE_FILTER_FLETCHER32
    HDF5_USE_FILTER_NBIT
    HDF5_USE_FILTER_SCALEOFFSET
    HDF5_USE_FILTER_SHUFFLE
    HDF5_ENABLE_Z_LIB_SUPPORT
    HDF5_ENABLE_SZIP_SUPPORT
    HDF5_ENABLE_SZIP_ENCODING
    HDF5_ENABLE_THREADSAFE
    HDF5_ENABLE_TRACE
    HDF5_USE_H5DUMP_PACKED_BITS
    HDF5_BUILD_FORTRAN
    HDF5_BUILD_EXAMPLES
    HDF5_BUILD_CPP_LIB
    HDF5_BUILD_TOOLS
    HDF5_BUILD_HL_LIB
    HDF5_Enable_Clear_File_Buffers
    HDF5_Enable_Instrument
    HDF5_STRICT_FORMAT_CHECKS
    HDF5_METADATA_TRACE_FILE
    HDF5_WANT_DATA_ACCURACY
    HDF5_WANT_DCONV_EXCEPTION
    HDF5_ENABLE_LARGE_FILE
    HDF5_STREAM_VFD
    HDF5_ENABLE_HSIZET
    H5_SET_LIB_OPTIONS
    HDF5_BUILD_WITH_INSTALL_NAME
    HDF5_PACKAGE_EXTLIBS
    )

ENDIF(VTK_USE_SYSTEM_HDF5)

# Force build options for verdict
SET( VERDICT_USE_FLOAT OFF CACHE BOOL "VTK requires doubles" FORCE )
SET( VERDICT_ENABLE_TESTING OFF CACHE BOOL "Verdict testing is currently broken." FORCE )
SET( VERDICT_NO_LIBRARY_VERSION 1 )
SET( VERDICT_LIBRARY_PROPERTIES ${VTK_LIBRARY_PROPERTIES} )
MARK_AS_ADVANCED( VERDICT_USE_FLOAT  VERDICT_ENABLE_TESTING VERDICT_BUILD_DOC )
set( VERDICT_MANGLE ON CACHE BOOL "VTK requires verdict to be mangled." FORCE )
set( VERDICT_MANGLE_PREFIX "vtk" CACHE STRING "VTK requires the verdict prefix to be vtk" FORCE )
SET( verdict_INSTALL_DOC_DIR ${VTK_INSTALL_DOC_DIR_CM24})
SET( verdict_INSTALL_INCLUDE_DIR ${VTK_INSTALL_INCLUDE_DIR_CM24})
SET( verdict_INSTALL_BIN_DIR ${VTK_INSTALL_BIN_DIR_CM24})
SET( verdict_INSTALL_LIB_DIR ${VTK_INSTALL_LIB_DIR_CM24})
ADD_SUBDIRECTORY(verdict)

ADD_SUBDIRECTORY(vtknetcdf)
IF(VTK_USE_METAIO)
  ADD_SUBDIRECTORY(vtkmetaio)
  SET_PROPERTY(GLOBAL APPEND PROPERTY VTK_TARGETS vtkmetaio)
ENDIF(VTK_USE_METAIO)
ADD_SUBDIRECTORY(vtksqlite)

IF(VTK_HAS_EXODUS)
  ADD_SUBDIRECTORY(vtkexodus2)
ENDIF(VTK_HAS_EXODUS)

ADD_SUBDIRECTORY(vtkalglib)

ADD_SUBDIRECTORY(Doxygen)
IF(VTK_INCLUDE_NEED_TCL)
  ADD_SUBDIRECTORY(TclTk)
ENDIF(VTK_INCLUDE_NEED_TCL)

ADD_SUBDIRECTORY(EncodeString)

IF(VTK_USE_PARALLEL)
  # vtkParallel uses source hashes during network handshakes.
  ADD_SUBDIRECTORY(HashSource)
  
  # helper classes for vtkVPICReader
  VTK_THIRD_PARTY_SUBDIR(VPIC VPIC)
  
  # helper classes for vtkPCosmoReader and vtkPCosmoHaloFinder
  VTK_THIRD_PARTY_SUBDIR(Cosmo Cosmo)
ENDIF(VTK_USE_PARALLEL)

IF(VTK_USE_RENDERING)
  ADD_SUBDIRECTORY(ftgl)
  ADD_SUBDIRECTORY(ParseOGLExt)
ENDIF(VTK_USE_RENDERING)

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    metaIOConfig.h
    vtk_expat.h
    vtk_jpeg.h
    vtk_png.h
    vtk_zlib.h
    vtk_gl2ps.h
    vtk_tiff.h
    vtk_freetype.h
    vtk_libxml2.h
    vtk_oggtheora.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}
    COMPONENT Development
    )
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

#-----------------------------------------------------------------------------
# Check for stdarg problem on SGI.
IF(NOT CMAKE_COMPILER_IS_GNUCXX)
  IF(CMAKE_SYSTEM MATCHES "IRIX.*")
    IF("VTK_COMPILER_NO_SGI_STDARG_WORKAROUND" MATCHES "^VTK_COMPILER_NO_SGI_STDARG_WORKAROUND$")
      MESSAGE(STATUS "Checking whether SGI MIPSpro 7.4 stdarg work-around is needed")
      TRY_COMPILE(VTK_COMPILER_NO_SGI_STDARG_WORKAROUND
        ${VTK_BINARY_DIR}/CMakeTmp
        ${VTK_CMAKE_DIR}/vtkTestSGIstdarg.cxx
        OUTPUT_VARIABLE OUTPUT)
      IF(VTK_COMPILER_NO_SGI_STDARG_WORKAROUND)
        MESSAGE(STATUS "Checking whether SGI MIPSpro 7.4 stdarg work-around is needed -- no")
        SET(VTK_COMPILER_NO_SGI_STDARG_WORKAROUND 1 CACHE INTERNAL "Whether SGI MIPSpro 7.4 stdarg work-around is needed")
        WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
          "Determining whether SGI MIPSpro does not have stdarg problems "
          "passed with the following output:\n"
          "${OUTPUT}\n" APPEND)
      ELSE(VTK_COMPILER_NO_SGI_STDARG_WORKAROUND)
        MESSAGE(STATUS "Checking whether SGI MIPSpro 7.4 stdarg work-around is needed -- yes")
        SET(VTK_COMPILER_NO_SGI_STDARG_WORKAROUND 0 CACHE INTERNAL "Whether SGI MIPSpro 7.4 stdarg work-around is needed")
        WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
          "Determining whether SGI MIPSpro does not have stdarg problems "
          "failed with the following output:\n"
          "${OUTPUT}\n" APPEND)
      ENDIF(VTK_COMPILER_NO_SGI_STDARG_WORKAROUND)
    ENDIF("VTK_COMPILER_NO_SGI_STDARG_WORKAROUND" MATCHES "^VTK_COMPILER_NO_SGI_STDARG_WORKAROUND$")
  ENDIF(CMAKE_SYSTEM MATCHES "IRIX.*")
ENDIF(NOT CMAKE_COMPILER_IS_GNUCXX)
IF(NOT VTK_COMPILER_NO_SGI_STDARG_WORKAROUND)
  # See internal_stdio_core.h.in for explanation.
  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/internal_stdio_core.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/internal/stdio_core.h
                 @ONLY IMMEDIATE)

  # Install the work-around header.
  IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/internal/stdio_core.h
      DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}/internal
      COMPONENT Development)
  ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
ENDIF(NOT VTK_COMPILER_NO_SGI_STDARG_WORKAROUND)
