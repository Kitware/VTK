CMAKE_MINIMUM_REQUIRED(VERSION 1.6)
PROJECT(VTK)
INCLUDE_REGULAR_EXPRESSION("^(lex|vtk|png|j).*$")

# Don't build anything unless the version of CMake is high enough.
# The matching ELSE/ENDIF should be the last lines in the file.
IF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.5)

#-----------------------------------------------------------------------------
# VTK version number.  An even minor number corresponds to releases.
SET(VTK_MAJOR_VERSION 4)
SET(VTK_MINOR_VERSION 1)
SET(VTK_BUILD_VERSION 1)
SET(VTK_VERSION
    "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.${VTK_BUILD_VERSION}")

#-----------------------------------------------------------------------------
# Output directories.
SET (LIBRARY_OUTPUT_PATH ${VTK_BINARY_DIR}/bin CACHE PATH "Single output directory for building all libraries.")
SET (EXECUTABLE_OUTPUT_PATH ${VTK_BINARY_DIR}/bin CACHE PATH "Single output directory for building all executables.")
MARK_AS_ADVANCED(LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH)
SET(VTK_LIBRARY_DIR ${LIBRARY_OUTPUT_PATH}/${CMAKE_CFG_INTDIR})
SET(VTK_EXECUTABLE_DIR ${EXECUTABLE_OUTPUT_PATH}/${CMAKE_CFG_INTDIR})

#-----------------------------------------------------------------------------
# Platform configuration tests.
INCLUDE (${CMAKE_ROOT}/Modules/CMakeBackwardCompatibilityC.cmake)
INCLUDE (${CMAKE_ROOT}/Modules/CMakeBackwardCompatibilityCXX.cmake)

SET(VTK_SIZEOF_CHAR   ${CMAKE_SIZEOF_CHAR})
SET(VTK_SIZEOF_DOUBLE ${CMAKE_SIZEOF_DOUBLE})
SET(VTK_SIZEOF_FLOAT  ${CMAKE_SIZEOF_FLOAT})
SET(VTK_SIZEOF_INT    ${CMAKE_SIZEOF_INT})
SET(VTK_SIZEOF_LONG   ${CMAKE_SIZEOF_LONG})
SET(VTK_SIZEOF_SHORT  ${CMAKE_SIZEOF_SHORT})

IF("VTK_COMPILER_HAS_BOOL" MATCHES "^VTK_COMPILER_HAS_BOOL$")
  MESSAGE(STATUS "Checking support for C++ type bool")
  TRY_COMPILE(VTK_COMPILER_HAS_BOOL
              ${VTK_BINARY_DIR}
              ${VTK_SOURCE_DIR}/CMake/vtkTestBoolType.cxx
              OUTPUT_VARIABLE OUTPUT)
  IF(VTK_COMPILER_HAS_BOOL)
    MESSAGE(STATUS "Checking support for C++ type bool -- yes")
    SET(VTK_COMPILER_HAS_BOOL 1 CACHE INTERNAL "Support for C++ type bool")
  ELSE(VTK_COMPILER_HAS_BOOL)
    MESSAGE(STATUS "Checking support for C++ type bool -- no")
    SET(VTK_COMPILER_HAS_BOOL 1 CACHE INTERNAL "Support for C++ type bool")
    WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeError.log 
      "Determining if the C++ compiler supports type bool "
      "failed with the following output:\n"
      "${OUTPUT}\n" APPEND)
  ENDIF(VTK_COMPILER_HAS_BOOL)
ENDIF("VTK_COMPILER_HAS_BOOL" MATCHES "^VTK_COMPILER_HAS_BOOL$")

#-----------------------------------------------------------------------------
# Load the VTK CMake extension module.

# Import the VTK_LOAD_CMAKE_EXTENSIONS macro.
INCLUDE(${VTK_SOURCE_DIR}/CMake/vtkLoadCMakeExtensions.cmake)

# Compile the extensions into the build tree and load them.
VTK_LOAD_CMAKE_EXTENSIONS(${VTK_SOURCE_DIR}/CMake ${VTK_BINARY_DIR}/CMake)

# Install cmake extension directory so user projects can build the commands.
INSTALL_FILES(/lib/vtk/CMake FILES
              ${VTK_SOURCE_DIR}/CMake/CMakeLists.txt
              ${VTK_SOURCE_DIR}/CMake/vtkLoadCMakeExtensions.cmake
              ${VTK_SOURCE_DIR}/CMake/cmVTKWrapTcl2Command.c
              ${VTK_SOURCE_DIR}/CMake/cmVTKWrapPython2Command.c
              ${VTK_SOURCE_DIR}/CMake/cmVTKWrapJava2Command.c
              ${VTK_SOURCE_DIR}/CMake/cmVTKMakeInstantiator2Command.c)

#-----------------------------------------------------------------------------
# Determine the set of VTK kits that should be built.
OPTION (VTK_USE_RENDERING "Build the vtkRendering kit.  Needed for displaying data." ON)
OPTION (VTK_USE_HYBRID "Build the vtkHybrid kit." OFF)
OPTION (VTK_USE_PARALLEL "Build the vtkParallel kit." OFF)
OPTION (VTK_USE_PATENTED
        "Build the vtkPatented kit.  These classes are patented and may require a license to use."
        OFF)

SET(VTK_KITS COMMON FILTERING IO GRAPHICS IMAGING)
IF(VTK_USE_RENDERING)
  SET(VTK_KITS ${VTK_KITS} RENDERING)
ENDIF(VTK_USE_RENDERING)
IF(VTK_USE_HYBRID)
  SET(VTK_KITS ${VTK_KITS} HYBRID)
ENDIF(VTK_USE_HYBRID)
IF(VTK_USE_PARALLEL)
  SET(VTK_KITS ${VTK_KITS} PARALLEL)
ENDIF(VTK_USE_PARALLEL)
IF(VTK_USE_PATENTED)
  SET(VTK_KITS ${VTK_KITS} PATENTED)
ENDIF(VTK_USE_PATENTED)

#-----------------------------------------------------------------------------
# Determine the set of language wrappers that should be built.
OPTION(VTK_WRAP_TCL "Wrap VTK classes into the TCL language.")
OPTION(VTK_WRAP_PYTHON "Wrap VTK classes into the Python language.")
OPTION(VTK_WRAP_JAVA "Wrap VTK classes into the Java language.")

SET(VTK_LANGUAGES "")
IF(VTK_WRAP_TCL)
  SET(VTK_LANGUAGES ${VTK_LANGUAGES} TCL)
ENDIF(VTK_WRAP_TCL)
IF(VTK_WRAP_PYTHON)
  SET(VTK_LANGUAGES ${VTK_LANGUAGES} PYTHON)
ENDIF(VTK_WRAP_PYTHON)
IF(VTK_WRAP_JAVA)
  SET(VTK_LANGUAGES ${VTK_LANGUAGES} JAVA)
ENDIF(VTK_WRAP_JAVA)

#-----------------------------------------------------------------------------
# VTK requires special compiler flags on some platforms.
IF(CMAKE_COMPILER_IS_GNUCXX)
  IF(WIN32)
    # The platform is gcc on cygwin.
    SET(VTK_REQUIRED_CXX_FLAGS "${VTK_REQUIRED_CXX_FLAGS} -mwin32")
    SET(VTK_REQUIRED_C_FLAGS "${VTK_REQUIRED_C_FLAGS} -mwin32")
  ENDIF(WIN32)
ELSE(CMAKE_COMPILER_IS_GNUCXX)
  IF(CMAKE_ANSI_CFLAGS)
    SET(VTK_REQUIRED_C_FLAGS "${VTK_REQUIRED_C_FLAGS} ${CMAKE_ANSI_CFLAGS}")
  ENDIF(CMAKE_ANSI_CFLAGS)
  IF(CMAKE_SYSTEM MATCHES "OSF1-V.*")
     SET(VTK_REQUIRED_CXX_FLAGS
         "${VTK_REQUIRED_CXX_FLAGS} -timplicit_local -no_implicit_include")
  ENDIF(CMAKE_SYSTEM MATCHES "OSF1-V.*")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

IF(APPLE)
  IF (VTK_USE_CARBON)
    SET(VTK_REQUIRED_CXX_FLAGS "${VTK_REQUIRED_CXX_FLAGS} -fpascal-strings")
  ENDIF (VTK_USE_CARBON)
  IF(BUILD_SHARED_LIBS)
    SET(VTK_APPLE_SHARED_FLAGS_NEEDED 1)
  ENDIF(BUILD_SHARED_LIBS)
ENDIF(APPLE)

IF(VTK_USE_X)
  IF(CMAKE_X_CFLAGS)
    SET(VTK_REQUIRED_C_FLAGS "${VTK_REQUIRED_C_FLAGS} ${CMAKE_X_CFLAGS}")
    SET(VTK_REQUIRED_CXX_FLAGS "${VTK_REQUIRED_CXX_FLAGS} ${CMAKE_X_CFLAGS}")
  ENDIF(CMAKE_X_CFLAGS)
ENDIF(VTK_USE_X)

#-----------------------------------------------------------------------------
# VTK utility script locations.
SET(VTK_DOXYGEN_HOME ${VTK_SOURCE_DIR}/Utilities/Doxygen)
SET(VTK_HEADER_TESTING_PY ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py)
SET(VTK_FIND_STRING_TCL ${VTK_SOURCE_DIR}/Common/Testing/Tcl/FindString.tcl)
SET(VTK_PRINT_SELF_CHECK_TCL ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl)
SET(VTK_RT_IMAGE_TEST_TCL ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl)
IF(VTK_USE_PARALLEL)
  SET(VTK_PRT_IMAGE_TEST_TCL ${VTK_SOURCE_DIR}/Common/Testing/Tcl/prtImageTest.tcl)
ENDIF(VTK_USE_PARALLEL)

#-----------------------------------------------------------------------------
# VTK wrapper package locations
IF(VTK_WRAP_TCL)
  SET(VTK_TCL_HOME ${VTK_SOURCE_DIR}/Wrapping/Tcl)
ENDIF(VTK_WRAP_TCL)
IF(VTK_WRAP_JAVA)
  SET(VTK_JAVA_JAR ${LIBRARY_OUTPUT_PATH}/vtk.jar)
ENDIF(VTK_WRAP_JAVA)

#-----------------------------------------------------------------------------
OPTION(BUILD_SHARED_LIBS "Build VTK with shared libraries." OFF)
OPTION(BUILD_EXAMPLES "Build VTK examples." OFF)
OPTION(VTK_USE_64BIT_IDS "Build VTK with 64 bit ids" OFF)
MARK_AS_ADVANCED(VTK_USE_64BIT_IDS)
SET(VTK_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

SUBDIRS(Wrapping Utilities Common Filtering Imaging Graphics IO)
INSTALL_FILES(/include/vtk .h vtkCommonInstantiator vtkFilteringInstantiator 
  vtkIOInstantiator vtkImagingInstantiator vtkGraphicsInstantiator)

OPTION (VTK_DEBUG_LEAKS "Build leak checking support into vtk." OFF)
MARK_AS_ADVANCED(VTK_DEBUG_LEAKS)

SET(VTK_DEFAULT_USE_X ON)
IF (APPLE)
  OPTION (VTK_USE_COCOA 
     "Build classes using Cocoa API - Can only use one of these!" OFF)
  OPTION (VTK_USE_CARBON "Build classes using Carbon API" ON)
  IF (CMAKE_HAS_X)
    SET(VTK_DEFAULT_USE_X OFF)
  ENDIF (CMAKE_HAS_X)
ELSE(APPLE)
  IF(CYGWIN)
    SET(VTK_DEFAULT_USE_X OFF)
  ENDIF(CYGWIN)
ENDIF(APPLE)

IF(CMAKE_HAS_X)
  OPTION(VTK_USE_X "Build classes for the X11 window manager" 
         ${VTK_DEFAULT_USE_X})
ENDIF(CMAKE_HAS_X)

IF(VTK_WRAP_TCL)
  IF(VTK_USE_RENDERING)
    SET(VTK_INCLUDE_NEED_TK 1)
  ENDIF(VTK_USE_RENDERING)
ENDIF(VTK_WRAP_TCL)

IF(VTK_WRAP_PYTHON)
  IF(VTK_USE_RENDERING)
    IF(NOT VTK_WRAP_TCL)
      SET(VTK_INCLUDE_NEED_TK 1)
    ENDIF(NOT VTK_WRAP_TCL)
  ENDIF(VTK_USE_RENDERING)
ENDIF(VTK_WRAP_PYTHON)

IF(VTK_INCLUDE_NEED_TK)
  IF(WIN32)
    IF(UNIX)
      # for builds against the source tree on Cygwin
      FIND_PATH(TK_XLIB_PATH X11/Xlib.h ${TK_INCLUDE_PATH} 
                ${TK_INCLUDE_PATH}/../xlib)
      MARK_AS_ADVANCED(TK_XLIB_PATH)
    ENDIF(UNIX)
  ENDIF(WIN32)
ENDIF(VTK_INCLUDE_NEED_TK)

IF (VTK_USE_PARALLEL)
    OPTION (VTK_USE_MPI 
      "use MPI (Message Passing Interface) library for parallel support")
    MARK_AS_ADVANCED(VTK_USE_MPI)
    IF(BUILD_TESTING)
      IF (VTK_USE_MPI)
        FIND_PROGRAM(VTK_MPIRUN_EXE NAMES mpirun lamexec)
        MARK_AS_ADVANCED(VTK_MPIRUN_EXE)
        SET (VTK_MPI_PREFLAGS "-np 2" CACHE STRING " Flags used by mpi start program.")
        SET (VTK_MPI_POSTFLAGS "" CACHE STRING "Flags used by mpi start program.")
        SET (VTK_MPI_MAX_NUMPROCS "2" CACHE STRING "Maximum number of processors available to run parallel applications.")
        MARK_AS_ADVANCED(VTK_MPI_PREFLAGS VTK_MPI_POSTFLAGS VTK_MPI_MAX_NUMPROCS)
        SEPARATE_ARGUMENTS(VTK_MPI_PREFLAGS)
        SEPARATE_ARGUMENTS(VTK_MPI_POSTFLAGS)
      ENDIF (VTK_USE_MPI)
    ENDIF(BUILD_TESTING)
ENDIF (VTK_USE_PARALLEL)

OPTION (VTK_USE_MANGLED_MESA "Use mangled Mesa with OpenGL" OFF)
MARK_AS_ADVANCED(VTK_USE_MANGLED_MESA)
IF (VTK_USE_MANGLED_MESA)
  FIND_PATH(MANGLED_MESA_INCLUDE_DIR GL/gl_mangle.h )
  FIND_LIBRARY(MANGLED_MESA_LIBRARY MesaGL)
  IF(MANGLED_MESA_INCLUDE_DIR)
    USE_MANGLED_MESA(${MANGLED_MESA_INCLUDE_DIR}/GL
                     ${VTK_BINARY_DIR}/MangleMesaInclude)
  ENDIF(MANGLED_MESA_INCLUDE_DIR)
ENDIF(VTK_USE_MANGLED_MESA)

#-----------------------------------------------------------------------------
# Provide options to use system versions of third-party libraries.
INCLUDE("${VTK_SOURCE_DIR}/vtkThirdParty.cmake")
VTK_THIRD_PARTY_OPTION(ZLIB zlib)
VTK_THIRD_PARTY_OPTION(JPEG jpeg)
VTK_THIRD_PARTY_OPTION(PNG  png)
VTK_THIRD_PARTY_OPTION(TIFF tiff)
VTK_THIRD_PARTY_OPTION(EXPAT expat)
#-----------------------------------------------------------------------------

IF (VTK_WRAP_TCL)
  INCLUDE (${CMAKE_ROOT}/Modules/FindTCL.cmake)
  UTILITY_SOURCE(VTK_WRAP_TCL_EXE vtkWrapTcl Wrapping vtkWrapTcl.c)
  MARK_AS_ADVANCED(VTK_WRAP_TCL_EXE)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping )
  MARK_AS_ADVANCED(VTK_WRAP_HINTS)
  SET (VTK_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/vtk)
  IF(VTK_USE_PARALLEL)
    IF(VTK_USE_MPI)
      SET (PVTK_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/pvtk)
    ENDIF(VTK_USE_MPI)
  ENDIF(VTK_USE_PARALLEL)
ENDIF (VTK_WRAP_TCL)

SET(VTK_RENDERING_NEED_TK_INTERNAL 0)
IF(VTK_USE_RENDERING)
  IF (VTK_WRAP_TCL)
    SET(VTK_RENDERING_NEED_TK_INTERNAL 1)
  ENDIF (VTK_WRAP_TCL)
  IF (VTK_WRAP_PYTHON)
    SET(VTK_RENDERING_NEED_TK_INTERNAL 1)
  ENDIF (VTK_WRAP_PYTHON)
  IF (VTK_DISABLE_TK_INIT)
    SET(VTK_RENDERING_NEED_TK_INTERNAL 0)
  ENDIF (VTK_DISABLE_TK_INIT)

  IF(VTK_RENDERING_NEED_TK_INTERNAL)
    IF (TK_LIBRARY)
      IF (APPLE)
        FIND_PATH(TK_INTERNAL_PATH tkMacOSXInt.h
          ${VTK_SOURCE_DIR}/Rendering/tkInternals/tk84OSX)
      ELSE(APPLE)
        IF (TK_LIBRARY MATCHES tk82)
          FIND_PATH(TK_INTERNAL_PATH tkWinInt.h
            ${VTK_SOURCE_DIR}/Rendering/tkInternals/tk82)
        ENDIF (TK_LIBRARY MATCHES tk82)
        IF (TK_LIBRARY MATCHES tk83)
          FIND_PATH(TK_INTERNAL_PATH tkWinInt.h
            ${VTK_SOURCE_DIR}/Rendering/tkInternals/tk83)
        ENDIF (TK_LIBRARY MATCHES tk83)
        IF (TK_LIBRARY MATCHES tk84)
          FIND_PATH(TK_INTERNAL_PATH tkWinInt.h
            ${VTK_SOURCE_DIR}/Rendering/tkInternals/tk84)
        ENDIF (TK_LIBRARY MATCHES tk84)
      ENDIF (APPLE)
    ENDIF (TK_LIBRARY)
  ENDIF(VTK_RENDERING_NEED_TK_INTERNAL)
ENDIF(VTK_USE_RENDERING)

IF (VTK_WRAP_PYTHON)
  INCLUDE (${CMAKE_ROOT}/Modules/FindPythonLibs.cmake)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping )
  MARK_AS_ADVANCED(VTK_WRAP_HINTS)
  UTILITY_SOURCE(VTK_WRAP_PYTHON_EXE vtkWrapPython Wrapping vtkWrapPython.c)
  MARK_AS_ADVANCED(VTK_WRAP_PYTHON_EXE)
  IF (NOT VTK_BUILD_SHARED_LIBS)
    MESSAGE(SEND_ERROR "Python support requires BUILD_SHARED_LIBS to be on.")
    SET(VTK_WRAP_PYTHON 0)
  ENDIF (NOT VTK_BUILD_SHARED_LIBS)  
  IF (NOT PYTHON_DEBUG_LIBRARY)
    SET(PYTHON_DEBUG_LIBRARY ${PYTHON_LIBRARY})
    IF(WIN32)
      MESSAGE("Cannot find Python debug library. The regular Python library will be used. This means that when using debug build of VTK, python examples might not run. If you use release build or you do not care about python working, set PYTHON_DEBUG_LIBRARY to the same file as PYTHON_LIBRARY.")
    ENDIF(WIN32)
  ENDIF (NOT PYTHON_DEBUG_LIBRARY)
  SET(VTK_PYTHON_EXE ${VTK_EXECUTABLE_PATH}/vtkpython)
  SET(PVTK_PYTHON_EXE ${VTK_EXECUTABLE_PATH}/pvtkpython)
ENDIF (VTK_WRAP_PYTHON)

# python executable is used by some tests whether VTK_WRAP_PYTHON is
# on or not.  do not add a VTK_WRAP_PYTHON to this if.
SET(VTK_NEED_PYTHON_EXECUTABLE 0)
IF(BUILD_TESTING)
 IF (DART_ROOT)
   SET(VTK_NEED_PYTHON_EXECUTABLE 1)
 ENDIF (DART_ROOT)
ENDIF(BUILD_TESTING)

# If VTK_WRAP_PYTHON is on, then we need python executable to compile
# scripts.
IF(VTK_WRAP_PYTHON)
  SET(VTK_NEED_PYTHON_EXECUTABLE 1)
ENDIF(VTK_WRAP_PYTHON)

IF(VTK_NEED_PYTHON_EXECUTABLE)
  FIND_PROGRAM(PYTHON_EXECUTABLE
    NAMES python2.3 python2.2 python2.1 python2.0 python1.6 python1.5 python
    PATHS
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.3\\InstallPath]
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.2\\InstallPath]
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.1\\InstallPath]
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.0\\InstallPath]
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\1.6\\InstallPath]
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\1.5\\InstallPath]
)
  MARK_AS_ADVANCED(PYTHON_EXECUTABLE)
ENDIF(VTK_NEED_PYTHON_EXECUTABLE)

IF (VTK_WRAP_JAVA)
  INCLUDE (${CMAKE_ROOT}/Modules/FindJNI.cmake)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping )
  MARK_AS_ADVANCED(VTK_WRAP_HINTS)
  UTILITY_SOURCE(VTK_WRAP_JAVA_EXE vtkWrapJava Wrapping vtkWrapJava.c)
  UTILITY_SOURCE(VTK_PARSE_JAVA_EXE vtkParseJava Wrapping vtkParseJava.c)
  MARK_AS_ADVANCED(VTK_WRAP_JAVA_EXE VTK_PARSE_JAVA_EXE)
  # where to write the resulting .java files
  SET (VTK_JAVA_HOME ${VTK_BINARY_DIR}/java/vtk)
  MAKE_DIRECTORY(${VTK_JAVA_HOME})
  IF (WIN32)
    IF (NOT VTK_BUILD_SHARED_LIBS)
      MESSAGE("Java support requires BUILD_SHARED_LIBS to be on, please turn it on." "Error")
    ENDIF (NOT VTK_BUILD_SHARED_LIBS)  
  ENDIF (WIN32)
ENDIF (VTK_WRAP_JAVA)

IF(WIN32)
  IF(UNIX) 
    SET(VTK_DISPLAY_WIN32_WARNINGS ON)
  ELSE(UNIX)
    OPTION(VTK_DISPLAY_WIN32_WARNINGS "Show all warnings on Visual C++ with option /W4"
           OFF)
    MARK_AS_ADVANCED(VTK_DISPLAY_WIN32_WARNINGS)
  ENDIF(UNIX)   
ENDIF(WIN32)

IF(CMAKE_COMPILER_IS_GNUCXX)
  OPTION(VTK_USE_ANSI_STDLIB "Use the ANSI standard iostream library" ON)
ELSE(CMAKE_COMPILER_IS_GNUCXX)
  IF(CMAKE_BUILD_TOOL MATCHES "devenv")
    OPTION(VTK_USE_ANSI_STDLIB "Use the ANSI standard iostream library" ON)
  ELSE(CMAKE_BUILD_TOOL MATCHES "devenv")
    OPTION(VTK_USE_ANSI_STDLIB "Use the ANSI standard iostream library" OFF)
  ENDIF(CMAKE_BUILD_TOOL MATCHES "devenv")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

#
# try to find the data files for VTKData
#
FIND_PATH(VTK_DATA_ROOT VTKData.readme ${VTK_SOURCE_DIR}/../VTKData $ENV{VTK_DATA_ROOT})

# where will executable tests be written ?
SET (CXX_TEST_PATH ${EXECUTABLE_OUTPUT_PATH})

IF (VTK_USE_RENDERING)
  OPTION(VTK_USE_VOLUMEPRO "Build VTK with volume pro support" OFF)
  IF (VTK_USE_VOLUMEPRO)
    FIND_LIBRARY(VLI_LIBRARY_FOR_VG500
      NAMES vli
      PATHS  
        /usr/lib 
        /usr/local/lib
        /opt/vli/lib
        "C:/Program Files/VolumePro/lib"
      )
    FIND_LIBRARY(VLI_LIBRARY_FOR_VP1000
      NAMES vli3
      PATHS
        /usr/lib
        /usr/local/lib
        /opt/vli/lib
        "C:/Program Files/VolumePro1000/lib"
      )
    IF (VLI_LIBRARY_FOR_VG500)
      IF (WIN32)
        FIND_PATH(VLI_INCLUDE_PATH_FOR_VG500 VolumePro/inc/vli.h 
          "C:/Program Files"
          )
      ELSE (WIN32)
        FIND_PATH(VLI_INCLUDE_PATH_FOR_VG500 vli/include/vli.h
          /usr
          /usr/local
          /opt
          )
      ENDIF (WIN32)
      IF (VLI_INCLUDE_PATH_FOR_VG500)
        SET (VTK_HAVE_VG500 ON)
      ENDIF (VLI_INCLUDE_PATH_FOR_VG500)
    ENDIF (VLI_LIBRARY_FOR_VG500)
    IF (VLI_LIBRARY_FOR_VP1000)
      IF (WIN32)
        FIND_PATH(VLI_INCLUDE_PATH_FOR_VP1000 VolumePro1000/inc/vli.h 
          "C:/Program Files"
          )
      ELSE (WIN32)
        FIND_PATH(VLI_INCLUDE_PATH_FOR_VP1000 vli3/include/vli.h
          /usr
          /usr/local
          /opt/vli/include
          )
      ENDIF (WIN32)
      IF (VLI_INCLUDE_PATH_FOR_VP1000)
        SET (VTK_HAVE_VP1000 ON)
      ENDIF (VLI_INCLUDE_PATH_FOR_VP1000)
    ENDIF (VLI_LIBRARY_FOR_VP1000)
  ENDIF (VTK_USE_VOLUMEPRO)

  INCLUDE (${CMAKE_ROOT}/Modules/FindOpenGL.cmake)

  IF(UNIX)
    OPTION(VTK_OPENGL_HAS_OSMESA 
           "The opengl library being used supports off screen Mesa calls." 
           OFF)
    MARK_AS_ADVANCED(VTK_OPENGL_HAS_OSMESA)
  ENDIF(UNIX)

  IF (VTK_OPENGL_HAS_OSMESA)
    FIND_PATH(OSMESA_INCLUDE_DIR GL/osmesa.h 
      /usr/include 
      /usr/local/include 
      /usr/openwin/share/include 
      /opt/graphics/OpenGL/include 
      /usr/X11R6/include 
    )

    FIND_LIBRARY(OSMESA_LIBRARY OSMesa
      /usr/lib 
      /usr/local/lib 
      /opt/graphics/OpenGL/lib 
      /usr/openwin/lib 
      /usr/X11R6/lib
    )
  ENDIF (VTK_OPENGL_HAS_OSMESA)

  IF (VTK_USE_MANGLED_MESA)
    FIND_LIBRARY(MANGLED_OSMESA_LIBRARY OSMesa)
  ENDIF (VTK_USE_MANGLED_MESA)

  SUBDIRS(Rendering)
  INSTALL_FILES(/include/vtk .h vtkRenderingInstantiator)
ENDIF (VTK_USE_RENDERING)

IF (VTK_USE_HYBRID)
  # hybrid requires rendering
  IF (NOT VTK_USE_RENDERING)
    MESSAGE("The Hybrid option requires Rendering, but you do not have Rendering selected." "Warning")
  ENDIF (NOT VTK_USE_RENDERING)

  IF (WIN32)
    OPTION(VTK_USE_VIDEO_FOR_WINDOWS "Use Video for Windows for video input" OFF)
    OPTION(VTK_USE_MATROX_IMAGING "Use Matrox Imaging Library for video input" OFF)
    IF (VTK_USE_MATROX_IMAGING)
      FIND_LIBRARY(MIL_LIBRARY MIL
        "C:/Program Files/Matrox Imaging/mil/library/winnt/msc/dll"
        "C:/Program Files/Matrox Imaging/mil/library/windows/msc/dll"
        )
      FIND_PATH(MIL_INCLUDE_PATH mil.h
        "C:/Program Files/Matrox Imaging/mil/include"
        )
    ENDIF (VTK_USE_MATROX_IMAGING)
  ENDIF (WIN32)

  SUBDIRS(Hybrid)
  INSTALL_FILES(/include/vtk .h vtkHybridInstantiator)
ENDIF (VTK_USE_HYBRID)

#-----------------------------------------------------------------------------

IF (VTK_USE_PATENTED)
  SUBDIRS(Patented)
  INSTALL_FILES(/include/vtk .h vtkPatentedInstantiator)
ENDIF (VTK_USE_PATENTED)

IF (VTK_USE_PARALLEL)
  SUBDIRS(Parallel)
  INSTALL_FILES(/include/vtk .h vtkParallelInstantiator)
ENDIF (VTK_USE_PARALLEL)

IF (VTK_WRAP_TCL)
  SUBDIRS(Wrapping/Tcl)
ENDIF (VTK_WRAP_TCL)

IF (VTK_WRAP_PYTHON)
  SUBDIRS(Wrapping/Python)
ENDIF (VTK_WRAP_PYTHON)

IF (VTK_WRAP_JAVA)
  INCLUDE (${CMAKE_ROOT}/Modules/FindJava.cmake)
  SUBDIRS(Wrapping/Java)
ENDIF (VTK_WRAP_JAVA)

IF(CMAKE_COMPILER_IS_GNUCXX AND WIN32)
  # GCC on Cygwin cannot do explicit template instantiation correctly with
  # DLLs.
  SET(CMAKE_NO_EXPLICIT_TEMPLATE_INSTANTIATION 1)
ENDIF(CMAKE_COMPILER_IS_GNUCXX AND WIN32)

CONFIGURE_FILE(
${VTK_SOURCE_DIR}/vtkConfigure.h.in 
${VTK_BINARY_DIR}/vtkConfigure.h)

CONFIGURE_FILE(
${VTK_SOURCE_DIR}/UseVTK.cmake.in 
${VTK_BINARY_DIR}/UseVTK.cmake COPYONLY IMMEDIATE)

CONFIGURE_FILE(
${VTK_SOURCE_DIR}/vtkToolkits.h.in 
${VTK_BINARY_DIR}/vtkToolkits.h)

# Include the standard Dart testing module
INCLUDE (${CMAKE_ROOT}/Modules/Dart.cmake)

# Add the testing directories
IF(BUILD_TESTING)
    ENABLE_TESTING()
    SUBDIRS(
      Common/Testing 
      Filtering/Testing 
      Graphics/Testing 
      Imaging/Testing 
      IO/Testing
#      Examples/Build
    )

    IF (VTK_USE_RENDERING)
      SUBDIRS(Rendering/Testing)
    ENDIF(VTK_USE_RENDERING)
    IF (VTK_USE_HYBRID)
      SUBDIRS(Hybrid/Testing)
    ENDIF(VTK_USE_HYBRID)
    IF (VTK_USE_PARALLEL)
      SUBDIRS(Parallel/Testing)
    ENDIF(VTK_USE_PARALLEL)
    IF (VTK_USE_PATENTED)
      SUBDIRS(Patented/Testing)
    ENDIF(VTK_USE_PATENTED)
ENDIF(BUILD_TESTING)

# Add the examples
IF (BUILD_EXAMPLES)
  SUBDIRS(Examples)
ENDIF (BUILD_EXAMPLES)


#-----------------------------------------------------------------------------
# The entire VTK tree should use the same include path.

# Create the list of include directories needed for VTK header files.
INCLUDE (${VTK_SOURCE_DIR}/vtkIncludeDirectories.cmake)

# This should be the only INCLUDE_DIRECTORIES command in the entire
# tree, except for the CMake, Utilities, and Examples directories.  We
# need to do this in one place to make sure the order is correct.
INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIRS_BUILD_TREE}
  ${VTK_INCLUDE_DIRS_SOURCE_TREE}
  ${VTK_INCLUDE_DIRS_BUILD_TREE_CXX}
  ${VTK_INCLUDE_DIRS_SYSTEM}
)

#-----------------------------------------------------------------------------
# Put "." in the rpath of all VTK shared libraries.

LINK_DIRECTORIES(.)

#-----------------------------------------------------------------------------
# Help other projects use VTK.

# Save the compiler settings so another project can import them.
INCLUDE(${CMAKE_ROOT}/Modules/CMakeExportBuildSettings.cmake)
CMAKE_EXPORT_BUILD_SETTINGS(${VTK_BINARY_DIR}/VTKBuildSettings.cmake)
INSTALL_FILES(/lib/vtk .cmake VTKBuildSettings)

# Create the VTKConfig.cmake file containing the VTK configuration.
INCLUDE (${VTK_SOURCE_DIR}/vtkGenerateVTKConfig.cmake)

# Install some files.
INSTALL_FILES(/include/vtk .h vtkConfigure vtkToolkits)
INSTALL_FILES(/lib/vtk .cmake UseVTK)

#-----------------------------------------------------------------------------
# Add compiler flags VTK needs to work on this platform.  This must be
# done AFTER the call to CMAKE_EXPORT_BUILD_SETTINGS.

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${VTK_REQUIRED_C_FLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VTK_REQUIRED_CXX_FLAGS}")

#-----------------------------------------------------------------------------
# Allow local additions to this file without CVS conflicts.
INCLUDE (${VTK_BINARY_DIR}/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/LocalUserOptions.cmake OPTIONAL)

# If the version of CMake was too old, complain and build nothing.
# These should be the last lines in this file.
ELSE("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.5)
  MESSAGE(SEND_ERROR
          "This version of CMake is too old to build VTK.  "
          "Please upgrade to CMake 1.6.")
ENDIF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.5)
