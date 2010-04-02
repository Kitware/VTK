# Build kwsys
SUBDIRS(kwsys)

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
SUBDIRS(verdict)

SUBDIRS(vtknetcdf)
IF(VTK_USE_METAIO)
  SUBDIRS(vtkmetaio)
ENDIF(VTK_USE_METAIO)
SUBDIRS(vtksqlite)

IF(VTK_HAS_EXODUS)
  SUBDIRS(vtkexodus2)
ENDIF(VTK_HAS_EXODUS)

SUBDIRS(vtkalglib)

SUBDIRS(Doxygen)
IF(VTK_INCLUDE_NEED_TCL)
  SUBDIRS(TclTk)
ENDIF(VTK_INCLUDE_NEED_TCL)

SUBDIRS(EncodeString)

IF(VTK_USE_PARALLEL)
  # vtkParallel uses source hashes during network handshakes.
  SUBDIRS(HashSource)
  
  # helper classes for vtkVPICReader
  VTK_THIRD_PARTY_SUBDIR(VPIC VPIC)
  
  # helper classes for vtkPCosmoReader and vtkPCosmoHaloFinder
  VTK_THIRD_PARTY_SUBDIR(Cosmo Cosmo)
ENDIF(VTK_USE_PARALLEL)

IF(VTK_USE_RENDERING)
  SUBDIRS(ftgl ParseOGLExt)
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
