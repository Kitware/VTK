# Build kwsys
SUBDIRS(kwsys)

# Build third-party utilities.

VTK_THIRD_PARTY_SUBDIR(ZLIB vtkzlib)
VTK_THIRD_PARTY_SUBDIR(JPEG vtkjpeg)
VTK_THIRD_PARTY_SUBDIR(PNG  vtkpng)
VTK_THIRD_PARTY_SUBDIR(TIFF vtktiff)
VTK_THIRD_PARTY_SUBDIR(EXPAT vtkexpat)
VTK_THIRD_PARTY_SUBDIR(FREETYPE vtkfreetype)
VTK_THIRD_PARTY_SUBDIR(DICOMParser DICOMParser)
VTK_THIRD_PARTY_SUBDIR(MATERIALLIBRARY MaterialLibrary)

SUBDIRS(vtkmpeg2encode)

IF(VTK_HAS_EXODUS)
  SUBDIRS(vtknetcdf)
  SUBDIRS(vtkexodus2)
ENDIF(VTK_HAS_EXODUS)

SUBDIRS(Doxygen)
IF(VTK_INCLUDE_NEED_TCL)
  SUBDIRS(TclTk)
ENDIF(VTK_INCLUDE_NEED_TCL)

IF(VTK_USE_RENDERING)
  SUBDIRS(ftgl ParseOGLExt)
ENDIF(VTK_USE_RENDERING)

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} .h
                vtk_expat vtk_jpeg vtk_png vtk_zlib vtk_tiff vtk_freetype)
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
    INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR}/internal FILES
                  ${CMAKE_CURRENT_BINARY_DIR}/internal/stdio_core.h)
  ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
ENDIF(NOT VTK_COMPILER_NO_SGI_STDARG_WORKAROUND)
