CMAKE_MINIMUM_REQUIRED(VERSION 2.0)
PROJECT(VTK)

SET(VTK_CMAKE_DIR "${VTK_SOURCE_DIR}/CMake")
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${VTK_CMAKE_DIR}")

# Don't build anything unless the version of CMake is high enough.
# The matching ELSE/ENDIF should be the last lines in the file.
IF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.9)

# Tell the CMake makefile generator to not have rules depend on
# themselves.  This causes extra rebuilds when the include path
# changes from turning a kit on or off.
SET(CMAKE_SKIP_RULE_DEPENDENCY 1)

#-----------------------------------------------------------------------------
# VTK version number.  An even minor number corresponds to releases.
SET(VTK_MAJOR_VERSION 5)
SET(VTK_MINOR_VERSION 1)
SET(VTK_BUILD_VERSION 0)
SET(VTK_VERSION
    "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.${VTK_BUILD_VERSION}")

# Append the library version information to the library target
# properties.  A parent project may set its own properties and/or may
# block this.
IF(NOT VTK_NO_LIBRARY_VERSION)
  SET(VTK_LIBRARY_PROPERTIES ${VTK_LIBRARY_PROPERTIES}
    VERSION "${VTK_VERSION}"
    SOVERSION "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}"
    )
ENDIF(NOT VTK_NO_LIBRARY_VERSION)

#-----------------------------------------------------------------------------
# Determine whether we are being built by CTest and if so the version.
# For CTest 2.2 and higher this will contain the version number.  For
# earlier CTest versions this will contain just "1".  If not
# configuring from CTest this should be empty, so set the result to a
# literal "0" (helps with numerical comparisons).
SET(VTK_TEST_FROM_CTEST "$ENV{DASHBOARD_TEST_FROM_CTEST}")
IF(NOT VTK_TEST_FROM_CTEST)
  SET(VTK_TEST_FROM_CTEST 0)
ENDIF(NOT VTK_TEST_FROM_CTEST)

# Determine whether tests requiring the configuration type to be known
# can be run.  Start by assuming the configuration type is known.
# This is the case when using a generator supporting only one
# configuration type.
SET(VTK_TEST_CONFIG_TYPE_KNOWN 1)
IF(CMAKE_CONFIGURATION_TYPES)
  # When there are multiple configuration types we must be running
  # tests with a CTest that knows how to give the configuration type
  # to the test when it runs.  This requires CTest 2.2 or higher.
  IF(VTK_TEST_FROM_CTEST LESS "2.2")
    # This is either a CTest older than 2.2 or not a CTest.
    # The configuration type will not be known when running the tests.
    SET(VTK_TEST_CONFIG_TYPE_KNOWN 0)
  ENDIF(VTK_TEST_FROM_CTEST LESS "2.2")
ENDIF(CMAKE_CONFIGURATION_TYPES)

#-----------------------------------------------------------------------------
# Load some macros.
INCLUDE(${VTK_CMAKE_DIR}/vtkDependentOption.cmake)
INCLUDE(${VTK_CMAKE_DIR}/vtkThirdParty.cmake)
INCLUDE(${VTK_CMAKE_DIR}/vtkExportKit.cmake)
INCLUDE(${VTK_CMAKE_DIR}/vtkMakeInstantiator.cmake)
INCLUDE(${CMAKE_ROOT}/Modules/CMakeExportBuildSettings.cmake)

#-----------------------------------------------------------------------------
# Choose static or shared libraries.
INCLUDE(${VTK_CMAKE_DIR}/vtkSelectSharedLibraries.cmake)

# Disable the install targets if using the rpath feature.
IF(NOT WIN32)
  IF(VTK_USE_RPATH)
    SET(VTK_INSTALL_NO_RUNTIME 1)
    SET(VTK_INSTALL_NO_DEVELOPMENT 1)
    SET(VTK_INSTALL_NO_DOCUMENTATION 1)
  ENDIF(VTK_USE_RPATH)
ENDIF(NOT WIN32)

#-----------------------------------------------------------------------------
# Output directories.
IF(NOT LIBRARY_OUTPUT_PATH)
  SET(LIBRARY_OUTPUT_PATH ${VTK_BINARY_DIR}/bin CACHE INTERNAL "Single output directory for building all libraries.")
ENDIF(NOT LIBRARY_OUTPUT_PATH)
IF(NOT EXECUTABLE_OUTPUT_PATH)
  SET(EXECUTABLE_OUTPUT_PATH ${VTK_BINARY_DIR}/bin CACHE INTERNAL "Single output directory for building all executables.")
ENDIF(NOT EXECUTABLE_OUTPUT_PATH)
SET(VTK_LIBRARY_DIR ${LIBRARY_OUTPUT_PATH}/${CMAKE_CFG_INTDIR})
SET(VTK_EXECUTABLE_DIR ${EXECUTABLE_OUTPUT_PATH}/${CMAKE_CFG_INTDIR})
SET(CXX_TEST_PATH ${EXECUTABLE_OUTPUT_PATH})

#-----------------------------------------------------------------------------
# Configure install locations.  This allows parent projects to modify
# the install location.  Optionally allow the project to specify a
# single VTK_INSTALL_ROOT which basically adds to its install prefix
# for VTK only.

# The location in which to install VTK executables.
IF(NOT VTK_INSTALL_BIN_DIR)
  SET(VTK_INSTALL_BIN_DIR ${VTK_INSTALL_ROOT}/bin)
ENDIF(NOT VTK_INSTALL_BIN_DIR)

# The location in which to install VTK header files.
IF(NOT VTK_INSTALL_INCLUDE_DIR)
  SET(VTK_INSTALL_INCLUDE_DIR
    ${VTK_INSTALL_ROOT}/include/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}
    )
ENDIF(NOT VTK_INSTALL_INCLUDE_DIR)

# The location in which to install VTK libraries.
IF(NOT VTK_INSTALL_LIB_DIR)
  SET(VTK_INSTALL_LIB_DIR ${VTK_INSTALL_ROOT}/lib)
ENDIF(NOT VTK_INSTALL_LIB_DIR)

# The location in which to install CMake scripts for packaging VTK.
IF(NOT VTK_INSTALL_PACKAGE_DIR)
  SET(VTK_INSTALL_PACKAGE_DIR
    ${VTK_INSTALL_LIB_DIR}/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}
    )
ENDIF(NOT VTK_INSTALL_PACKAGE_DIR)

# The location in which to install VTK doxygen documentation helper
# files.
IF(NOT VTK_INSTALL_DOXYGEN_DIR)
  SET(VTK_INSTALL_DOXYGEN_DIR ${VTK_INSTALL_PACKAGE_DIR}/doxygen)
ENDIF(NOT VTK_INSTALL_DOXYGEN_DIR)

# Compute the proper location for installing the Tcl package.  This
# must be a fixed relative path below the library install location and
# is therefore not settable by parent projects.
SET(VTK_INSTALL_TCL_DIR
  ${VTK_INSTALL_LIB_DIR}/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}
  )

IF(NOT VTK_INSTALL_JAVA_DIR)
  SET(VTK_INSTALL_JAVA_DIR ${VTK_INSTALL_PACKAGE_DIR}/java)
ENDIF(NOT VTK_INSTALL_JAVA_DIR)

# Choose a default CMAKE_INSTALL_PREFIX on Windows.  CMake 2.2 and
# above already choose a good default, so force a change only if the
# current value looks like the CMake 2.0 default.
IF(NOT UNIX)
  IF(CMAKE_INSTALL_PREFIX MATCHES "/usr/local")
    IF("$ENV{ProgramFiles}" MATCHES "^$")
      IF("$ENV{SystemDrive}" MATCHES "^$")
        SET(VTK_GENERIC_PROGRAM_FILES "C:/Program Files")
      ELSE("$ENV{SystemDrive}" MATCHES "^$")
        SET(VTK_GENERIC_PROGRAM_FILES "$ENV{SystemDrive}/Program Files")
      ENDIF("$ENV{SystemDrive}" MATCHES "^$")
    ELSE("$ENV{ProgramFiles}" MATCHES "^$")
      SET(VTK_GENERIC_PROGRAM_FILES "$ENV{ProgramFiles}")
    ENDIF("$ENV{ProgramFiles}" MATCHES "^$")
    SET(CMAKE_INSTALL_PREFIX
      "${VTK_GENERIC_PROGRAM_FILES}/${PROJECT_NAME}"
      CACHE PATH "Install path prefix, prepended onto install directories."
      FORCE)
    SET(VTK_GENERIC_SYSTEM_DRIVE)
  ENDIF(CMAKE_INSTALL_PREFIX MATCHES "/usr/local")
  MARK_AS_ADVANCED(CLEAR CMAKE_INSTALL_PREFIX)
ENDIF(NOT UNIX)

# There are three basic components to the VTK installation: runtime,
# development, and documentation.  Install rules for each component
# are surrounded by blockers.  Parent projects or users can specify
# VTK_INSTALL_NO_RUNTIME, VTK_INSTALL_NO_DEVELOPMENT, or
# VTK_INSTALL_NO_DOCUMENTATION to avoid installation of the
# corresponding component.

# Shared libraries are considered both runtime and development and
# static libraries are considered development only.  In order to
# switch library installation on and off correctly we make the
# decision here.
SET(VTK_INSTALL_NO_LIBRARIES)
IF(BUILD_SHARED_LIBS)
  IF(VTK_INSTALL_NO_RUNTIME AND VTK_INSTALL_NO_DEVELOPMENT)
    SET(VTK_INSTALL_NO_LIBRARIES 1)
  ENDIF(VTK_INSTALL_NO_RUNTIME AND VTK_INSTALL_NO_DEVELOPMENT)
ELSE(BUILD_SHARED_LIBS)
  IF(VTK_INSTALL_NO_DEVELOPMENT)
    SET(VTK_INSTALL_NO_LIBRARIES 1)
  ENDIF(VTK_INSTALL_NO_DEVELOPMENT)
ENDIF(BUILD_SHARED_LIBS)

#-----------------------------------------------------------------------------
# Save the compiler settings so another project can import them.
CMAKE_EXPORT_BUILD_SETTINGS(${VTK_BINARY_DIR}/VTKBuildSettings.cmake)
IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES(${VTK_INSTALL_PACKAGE_DIR} .cmake VTKBuildSettings)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

#-----------------------------------------------------------------------------
# Provide compatibility options.
OPTION(VTK_LEGACY_REMOVE "Remove all legacy code completely." OFF)
OPTION(VTK_LEGACY_SILENT "Silence all legacy code messages." OFF)
MARK_AS_ADVANCED(VTK_LEGACY_REMOVE VTK_LEGACY_SILENT)

#-----------------------------------------------------------------------------
# Determine the set of VTK kits that should be built.
OPTION(VTK_USE_RENDERING "Build the vtkRendering kit.  Needed for displaying data or using widgets." ON)
VTK_DEPENDENT_OPTION(VTK_USE_PARALLEL "Build the vtkParallel kit." OFF
                     "VTK_USE_RENDERING" OFF)

# Determine Shading Support
VTK_DEPENDENT_OPTION(VTK_USE_CG_SHADERS "Build pixel and vertex shader support for Cg." OFF
                     "VTK_USE_RENDERING" OFF)
VTK_DEPENDENT_OPTION(VTK_USE_GLSL_SHADERS "Build pixel and vertex shader support for GLSL." OFF
                     "VTK_USE_RENDERING" OFF)

SET(VTK_DEFAULT_SHADERS_DIR
   "${VTK_BINARY_DIR}/Utilities/MaterialLibrary/Repository"
   CACHE INTERNAL 
   "The directory in which code for Shaders is provided.")

SET(VTK_MATERIALS_DIRS 
  ${VTK_DEFAULT_SHADERS_DIR}
  CACHE STRING
  "; separated directories to search for materials/shaders")

MARK_AS_ADVANCED(VTK_MATERIALS_DIRS)

  
# Determine GUI Support.
VTK_DEPENDENT_OPTION(VTK_USE_GUISUPPORT "Build VTK with GUI Support" OFF
                     "VTK_USE_RENDERING" OFF)
MARK_AS_ADVANCED(VTK_USE_GUISUPPORT)

# Remove old options from an existing cache.
IF("VTK_USE_HYBRID" MATCHES "^VTK_USE_HYBRID$")
ELSE("VTK_USE_HYBRID" MATCHES "^VTK_USE_HYBRID$")
  SET(VTK_USE_HYBRID "" CACHE INTERNAL "Hiding old option")
ENDIF("VTK_USE_HYBRID" MATCHES "^VTK_USE_HYBRID$")
IF("VTK_USE_PATENTED" MATCHES "^VTK_USE_PATENTED$")
ELSE("VTK_USE_PATENTED" MATCHES "^VTK_USE_PATENTED$")
  SET(VTK_USE_PATENTED "" CACHE INTERNAL "Hiding old option")
ENDIF("VTK_USE_PATENTED" MATCHES "^VTK_USE_PATENTED$")
IF("VTK_USE_VOLUMERENDERING" MATCHES "^VTK_USE_VOLUMERENDERING$")
ELSE("VTK_USE_VOLUMERENDERING" MATCHES "^VTK_USE_VOLUMERENDERING$")
  SET(VTK_USE_VOLUMERENDERING "" CACHE INTERNAL "Hiding old option")
ENDIF("VTK_USE_VOLUMERENDERING" MATCHES "^VTK_USE_VOLUMERENDERING$")

SET(VTK_KITS COMMON FILTERING IO GRAPHICS GENERIC_FILTERING IMAGING)
IF(VTK_USE_RENDERING)
  SET(VTK_KITS ${VTK_KITS} RENDERING)
  SET(VTK_KITS ${VTK_KITS} VOLUMERENDERING)
  SET(VTK_KITS ${VTK_KITS} HYBRID)
  SET(VTK_KITS ${VTK_KITS} WIDGETS)
ENDIF(VTK_USE_RENDERING)
IF(VTK_USE_PARALLEL)
  SET(VTK_KITS ${VTK_KITS} PARALLEL)
ENDIF(VTK_USE_PARALLEL)

# from GUISupport
IF(VTK_USE_QVTK)
  SET(VTK_KITS ${VTK_KITS} QVTK)
ENDIF(VTK_USE_QVTK)
IF(VTK_USE_MFC)
  SET(VTK_KITS ${VTK_KITS} MFC)
ENDIF(VTK_USE_MFC)


#-----------------------------------------------------------------------------
# Determine GUI.
FIND_PACKAGE(X11)
SET(VTK_USE_X_OPTIONAL 0)
SET(VTK_USE_X_FORCE 0)
IF(X11_FOUND)
  IF(CYGWIN)
    SET(VTK_USE_X_OPTIONAL 1)
  ENDIF(CYGWIN)
  IF(APPLE)
    SET(VTK_USE_X_OPTIONAL 1)
  ELSE(APPLE)
    SET(VTK_USE_X_FORCE ${VTK_USE_RENDERING})
  ENDIF(APPLE)
ENDIF(X11_FOUND)

VTK_DEPENDENT_OPTION(VTK_USE_CARBON "Build classes using Carbon API." ON
                     "APPLE;VTK_USE_RENDERING" OFF)
VTK_DEPENDENT_OPTION(VTK_USE_COCOA "Build classes using Cocoa API." OFF
                     "APPLE;VTK_USE_RENDERING" OFF)
VTK_DEPENDENT_OPTION(VTK_USE_X
                     "Build classes for the X11 window system." OFF
                     "X11_FOUND;VTK_USE_RENDERING;VTK_USE_X_OPTIONAL"
                     "${VTK_USE_X_FORCE}")

IF(VTK_USE_CARBON AND VTK_USE_COCOA)
  MESSAGE(SEND_ERROR "Only one of VTK_USE_CARBON and VTK_USE_COCOA may be ON.")
  SET(VTK_USE_CARBON 0)
  SET(VTK_USE_COCOA 0)
ENDIF(VTK_USE_CARBON AND VTK_USE_COCOA)

#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
# VTK requires special compiler flags on some platforms.
SET(VTK_REQUIRED_C_FLAGS)
SET(VTK_REQUIRED_CXX_FLAGS)

# make sure Crun is linked in with the native compiler, it is
# not used by default for shared libraries and is required for
# things like java to work.
IF(CMAKE_SYSTEM MATCHES "SunOS.*")
  IF(NOT CMAKE_COMPILER_IS_GNUCXX)
     LINK_LIBRARIES(Crun)
  ENDIF(NOT CMAKE_COMPILER_IS_GNUCXX)
ENDIF(CMAKE_SYSTEM MATCHES "SunOS.*")

IF(CMAKE_COMPILER_IS_GNUCXX)
  # A GCC compiler.  Quiet warning about strstream deprecation.
  SET(VTK_REQUIRED_CXX_FLAGS "${VTK_REQUIRED_CXX_FLAGS} -Wno-deprecated")
  IF(WIN32)
    # The platform is gcc on cygwin.
    SET(VTK_REQUIRED_CXX_FLAGS "${VTK_REQUIRED_CXX_FLAGS} -mwin32")
    SET(VTK_REQUIRED_C_FLAGS "${VTK_REQUIRED_C_FLAGS} -mwin32")
  ENDIF(WIN32)
  IF(MINGW)
    SET(VTK_REQUIRED_CXX_FLAGS "${VTK_REQUIRED_CXX_FLAGS} -mthreads")
    SET(VTK_REQUIRED_C_FLAGS "${VTK_REQUIRED_C_FLAGS} -mthreads")
    SET(VTK_REQUIRED_EXE_LINKER_FLAGS "${VTK_REQUIRED_EXE_LINKER_FLAGS} -mthreads")
    SET(VTK_REQUIRED_SHARED_LINKER_FLAGS "${VTK_REQUIRED_SHARED_LINKER_FLAGS} -mthreads")
    SET(VTK_REQUIRED_MODULE_LINKER_FLAGS "${VTK_REQUIRED_MODULE_LINKER_FLAGS} -mthreads")
    LINK_LIBRARIES(-lgdi32)
  ENDIF(MINGW)
  IF(CMAKE_SYSTEM MATCHES "SunOS.*")
    # Disable warnings that occur in X11 headers.
    IF(DART_ROOT AND BUILD_TESTING)
      SET(VTK_REQUIRED_CXX_FLAGS "${VTK_REQUIRED_CXX_FLAGS} -Wno-unknown-pragmas")
      SET(VTK_REQUIRED_C_FLAGS "${VTK_REQUIRED_C_FLAGS} -Wno-unknown-pragmas")
    ENDIF(DART_ROOT AND BUILD_TESTING)
  ENDIF(CMAKE_SYSTEM MATCHES "SunOS.*")
ELSE(CMAKE_COMPILER_IS_GNUCXX)
  IF(CMAKE_ANSI_CFLAGS)
    SET(VTK_REQUIRED_C_FLAGS "${VTK_REQUIRED_C_FLAGS} ${CMAKE_ANSI_CFLAGS}")
  ENDIF(CMAKE_ANSI_CFLAGS)
  IF(CMAKE_SYSTEM MATCHES "OSF1-V.*")
     SET(VTK_REQUIRED_CXX_FLAGS
         "${VTK_REQUIRED_CXX_FLAGS} -timplicit_local -no_implicit_include")
  ENDIF(CMAKE_SYSTEM MATCHES "OSF1-V.*")
  IF(CMAKE_SYSTEM MATCHES "IRIX.*")
    SET(VTK_REQUIRED_CXX_FLAGS
      "${VTK_REQUIRED_CXX_FLAGS} -Wl,-woff84 -woff 15 -woff 84 -woff 3439 -woff 1424 -woff 3201")
    SET(VTK_REQUIRED_C_FLAGS "${VTK_REQUIRED_C_FLAGS} -woff 3439")
  ENDIF(CMAKE_SYSTEM MATCHES "IRIX.*")
  #silence duplicate symbol warnings on AIX
  IF(CMAKE_SYSTEM MATCHES "AIX.*")
    SET(VTK_REQUIRED_EXE_LINKER_FLAGS "${VTK_REQUIRED_EXE_LINKER_FLAGS} -bhalt:5")
    SET(VTK_REQUIRED_SHARED_LINKER_FLAGS "${VTK_REQUIRED_SHARED_LINKER_FLAGS} -bhalt:5")
    SET(VTK_REQUIRED_MODULE_LINKER_FLAGS "${VTK_REQUIRED_MODULE_LINKER_FLAGS} -bhalt:5")
  ENDIF(CMAKE_SYSTEM MATCHES "AIX.*")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

IF(APPLE)
  SET(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "${CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS} -Wl,-flat_namespace,-U,_environ")
  SET(CMAKE_SHARED_MODULE_CREATE_C_FLAGS "${CMAKE_SHARED_MODULE_CREATE_C_FLAGS} -Wl,-flat_namespace,-U,_environ")
  IF(CMAKE_COMPILER_IS_GNUCXX)
    SET(VTK_REQUIRED_C_FLAGS "${VTK_REQUIRED_C_FLAGS} -no-cpp-precomp")
    SET(VTK_REQUIRED_CXX_FLAGS "${VTK_REQUIRED_CXX_FLAGS} -no-cpp-precomp")
  ENDIF(CMAKE_COMPILER_IS_GNUCXX)
ENDIF(APPLE)

IF(UNIX)
  IF(NOT CMAKE_COMPILER_IS_GNUCXX)
    INCLUDE(${VTK_CMAKE_DIR}/TestNO_ICC_IDYNAMIC_NEEDED.cmake)
    TESTNO_ICC_IDYNAMIC_NEEDED(NO_ICC_IDYNAMIC_NEEDED ${VTK_CMAKE_DIR})
    IF(NO_ICC_IDYNAMIC_NEEDED)
      SET(VTK_REQUIRED_CXX_FLAGS "${VTK_REQUIRED_CXX_FLAGS}")
    ELSE(NO_ICC_IDYNAMIC_NEEDED)
      SET(VTK_REQUIRED_CXX_FLAGS "${VTK_REQUIRED_CXX_FLAGS} -i_dynamic")
    ENDIF(NO_ICC_IDYNAMIC_NEEDED)
  ENDIF(NOT CMAKE_COMPILER_IS_GNUCXX)
ENDIF(UNIX)

IF(CMAKE_BUILD_TOOL MATCHES "(msdev|devenv|nmake)")
  # Use the highest warning level for visual studio.
  SET(CMAKE_CXX_WARNING_LEVEL 4)
  IF(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    STRING(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  ELSE(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  ENDIF(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
ENDIF(CMAKE_BUILD_TOOL MATCHES "(msdev|devenv|nmake)")

# Tell VTK source files they are being built inside VTK.
ADD_DEFINITIONS(-DVTK_IN_VTK)

#-----------------------------------------------------------------------------
# Add compiler flags VTK needs to work on this platform.  This must be
# done after the call to CMAKE_EXPORT_BUILD_SETTINGS, but before any
# try-compiles are done.
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${VTK_REQUIRED_C_FLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VTK_REQUIRED_CXX_FLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${VTK_REQUIRED_EXE_LINKER_FLAGS}")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${VTK_REQUIRED_SHARED_LINKER_FLAGS}")
SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${VTK_REQUIRED_MODULE_LINKER_FLAGS}")

# The Borland implib tool is used by CMake to generate the .lib export
# file from a .dll.  CMake 2.0 and earlier did not add the "-c" option
# to specify that the exported symbols should be case sensitive.  Add
# the flag now if it is not there.
IF(BORLAND)
  STRING(REGEX REPLACE "implib -w" "implib -c -w"
    CMAKE_CXX_CREATE_SHARED_LIBRARY "${CMAKE_CXX_CREATE_SHARED_LIBRARY}")
ENDIF(BORLAND)

#-----------------------------------------------------------------------------
# Platform configuration tests.
INCLUDE(${CMAKE_ROOT}/Modules/CMakeBackwardCompatibilityC.cmake)
INCLUDE(${CMAKE_ROOT}/Modules/CMakeBackwardCompatibilityCXX.cmake)
INCLUDE(${CMAKE_ROOT}/Modules/CheckTypeSize.cmake)

SET(VTK_SIZEOF_CHAR   ${CMAKE_SIZEOF_CHAR})
SET(VTK_SIZEOF_DOUBLE ${CMAKE_SIZEOF_DOUBLE})
SET(VTK_SIZEOF_FLOAT  ${CMAKE_SIZEOF_FLOAT})
SET(VTK_SIZEOF_INT    ${CMAKE_SIZEOF_INT})
SET(VTK_SIZEOF_LONG   ${CMAKE_SIZEOF_LONG})
SET(VTK_SIZEOF_SHORT  ${CMAKE_SIZEOF_SHORT})
CHECK_TYPE_SIZE("long long" VTK_SIZEOF_LONG_LONG)
CHECK_TYPE_SIZE("__int64"   VTK_SIZEOF___INT64)

IF(VTK_SIZEOF___INT64)
  IF("VTK_TYPE_SAME_LONG_AND___INT64" MATCHES "^VTK_TYPE_SAME_LONG_AND___INT64$")
    MESSAGE(STATUS "Checking whether long and __int64 are the same type")
    TRY_COMPILE(VTK_TYPE_SAME_LONG_AND___INT64
      ${VTK_BINARY_DIR}/CMakeTmp
      ${VTK_CMAKE_DIR}/vtkTestCompareTypes.cxx
      COMPILE_DEFINITIONS
      -DVTK_TEST_COMPARE_TYPE_1=long
      -DVTK_TEST_COMPARE_TYPE_2=__int64
      OUTPUT_VARIABLE OUTPUT)
    IF(VTK_TYPE_SAME_LONG_AND___INT64)
      MESSAGE(STATUS "Checking whether long and __int64 are the same type -- yes")
      SET(VTK_TYPE_SAME_LONG_AND___INT64 1 CACHE INTERNAL "Whether long and __int64 are the same type")
      WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
        "Determining whether long and __int64 are the same type "
        "passed with the following output:\n"
        "${OUTPUT}\n" APPEND)
    ELSE(VTK_TYPE_SAME_LONG_AND___INT64)
      MESSAGE(STATUS "Checking whether long and __int64 are the same type -- no")
      SET(VTK_TYPE_SAME_LONG_AND___INT64 0 CACHE INTERNAL "Whether long and __int64 are the same type")
      WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
        "Determining whether long and __int64 are the same type "
        "failed with the following output:\n"
        "${OUTPUT}\n" APPEND)
    ENDIF(VTK_TYPE_SAME_LONG_AND___INT64)
  ENDIF("VTK_TYPE_SAME_LONG_AND___INT64" MATCHES "^VTK_TYPE_SAME_LONG_AND___INT64$")
  IF(VTK_SIZEOF_LONG_LONG)
    IF("VTK_TYPE_SAME_LONG_LONG_AND___INT64" MATCHES "^VTK_TYPE_SAME_LONG_LONG_AND___INT64$")
      MESSAGE(STATUS "Checking whether long long and __int64 are the same type")
      TRY_COMPILE(VTK_TYPE_SAME_LONG_LONG_AND___INT64
        ${VTK_BINARY_DIR}/CMakeTmp
        ${VTK_CMAKE_DIR}/vtkTestCompareTypes.cxx
        COMPILE_DEFINITIONS
        -DVTK_TEST_COMPARE_TYPE_1=TYPE_LONG_LONG
        -DVTK_TEST_COMPARE_TYPE_2=__int64
        OUTPUT_VARIABLE OUTPUT)
      IF(VTK_TYPE_SAME_LONG_LONG_AND___INT64)
        MESSAGE(STATUS "Checking whether long long and __int64 are the same type -- yes")
        SET(VTK_TYPE_SAME_LONG_LONG_AND___INT64 1 CACHE INTERNAL "Whether long long and __int64 are the same type")
        WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
          "Determining whether long long and __int64 are the same type "
          "passed with the following output:\n"
          "${OUTPUT}\n" APPEND)
      ELSE(VTK_TYPE_SAME_LONG_LONG_AND___INT64)
        MESSAGE(STATUS "Checking whether long long and __int64 are the same type -- no")
        SET(VTK_TYPE_SAME_LONG_LONG_AND___INT64 0 CACHE INTERNAL "Whether long long and __int64 are the same type")
        WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
          "Determining whether long long and __int64 are the same type "
          "failed with the following output:\n"
          "${OUTPUT}\n" APPEND)
      ENDIF(VTK_TYPE_SAME_LONG_LONG_AND___INT64)
    ENDIF("VTK_TYPE_SAME_LONG_LONG_AND___INT64" MATCHES "^VTK_TYPE_SAME_LONG_LONG_AND___INT64$")
  ENDIF(VTK_SIZEOF_LONG_LONG)
  IF(NOT VTK_TYPE_SAME_LONG_AND___INT64)
    IF(NOT VTK_TYPE_SAME_LONG_LONG_AND___INT64)
      #  VS 6 cannot convert unsigned __int64 to double unless the
      # "Visual C++ Processor Pack" is installed.
      IF("VTK_TYPE_CONVERT_UI64_TO_DOUBLE" MATCHES "^VTK_TYPE_CONVERT_UI64_TO_DOUBLE$")
        MESSAGE(STATUS "Checking whether unsigned __int64 can convert to double")
        TRY_COMPILE(VTK_TYPE_CONVERT_UI64_TO_DOUBLE
          ${VTK_BINARY_DIR}/CMakeTmp
          ${VTK_CMAKE_DIR}/vtkTestConvertTypes.cxx
          COMPILE_DEFINITIONS
          -DVTK_TEST_CONVERT_TYPE_FROM=TYPE_UNSIGNED___INT64
          -DVTK_TEST_CONVERT_TYPE_TO=double
          OUTPUT_VARIABLE OUTPUT)
        IF(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
          MESSAGE(STATUS "Checking whether unsigned __int64 can convert to double -- yes")
          SET(VTK_TYPE_CONVERT_UI64_TO_DOUBLE 1 CACHE INTERNAL "Whether unsigned __int64 can convert to double")
          WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
            "Determining whether unsigned __int64 can convert to double "
            "passed with the following output:\n"
            "${OUTPUT}\n" APPEND)
        ELSE(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
          MESSAGE(STATUS "Checking whether unsigned __int64 can convert to double -- no")
          SET(VTK_TYPE_CONVERT_UI64_TO_DOUBLE 0 CACHE INTERNAL "Whether unsigned __int64 can convert to double")
          WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
            "Determining whether unsigned __int64 can convert to double "
            "failed with the following output:\n"
            "${OUTPUT}\n" APPEND)
        ENDIF(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
      ENDIF("VTK_TYPE_CONVERT_UI64_TO_DOUBLE" MATCHES "^VTK_TYPE_CONVERT_UI64_TO_DOUBLE$")
    ENDIF(NOT VTK_TYPE_SAME_LONG_LONG_AND___INT64)
  ENDIF(NOT VTK_TYPE_SAME_LONG_AND___INT64)
ENDIF(VTK_SIZEOF___INT64)

# Enable the "long long" type if it is available.  It is standard in
# C99 and C++03 but not in earlier standards.
SET(VTK_TYPE_USE_LONG_LONG)
IF(VTK_SIZEOF_LONG_LONG)
  SET(VTK_TYPE_USE_LONG_LONG 1)
ENDIF(VTK_SIZEOF_LONG_LONG)

# Enable the "__int64" type if it is available and unique.  It is not
# standard.
SET(VTK_TYPE_USE___INT64)
IF(VTK_SIZEOF___INT64)
  IF(NOT VTK_TYPE_SAME_LONG_AND___INT64)
    IF(NOT VTK_TYPE_SAME_LONG_LONG_AND___INT64)
      SET(VTK_TYPE_USE___INT64 1)
    ENDIF(NOT VTK_TYPE_SAME_LONG_LONG_AND___INT64)
  ENDIF(NOT VTK_TYPE_SAME_LONG_AND___INT64)
ENDIF(VTK_SIZEOF___INT64)

IF("VTK_COMPILER_HAS_BOOL" MATCHES "^VTK_COMPILER_HAS_BOOL$")
  MESSAGE(STATUS "Checking support for C++ type bool")
  TRY_COMPILE(VTK_COMPILER_HAS_BOOL
              ${VTK_BINARY_DIR}/CMakeTmp/Bool
              ${VTK_CMAKE_DIR}/vtkTestBoolType.cxx
              OUTPUT_VARIABLE OUTPUT)
  IF(VTK_COMPILER_HAS_BOOL)
    MESSAGE(STATUS "Checking support for C++ type bool -- yes")
    SET(VTK_COMPILER_HAS_BOOL 1 CACHE INTERNAL "Support for C++ type bool")
    WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
      "Determining if the C++ compiler supports type bool "
      "passed with the following output:\n"
      "${OUTPUT}\n" APPEND)
  ELSE(VTK_COMPILER_HAS_BOOL)
    MESSAGE(STATUS "Checking support for C++ type bool -- no")
    SET(VTK_COMPILER_HAS_BOOL 0 CACHE INTERNAL "Support for C++ type bool")
    WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
      "Determining if the C++ compiler supports type bool "
      "failed with the following output:\n"
      "${OUTPUT}\n" APPEND)
  ENDIF(VTK_COMPILER_HAS_BOOL)
ENDIF("VTK_COMPILER_HAS_BOOL" MATCHES "^VTK_COMPILER_HAS_BOOL$")

IF("VTK_COMPILER_HAS_FULL_SPECIALIZATION" MATCHES "^VTK_COMPILER_HAS_FULL_SPECIALIZATION$")
  MESSAGE(STATUS "Checking support for full template specialization syntax")
  TRY_COMPILE(VTK_COMPILER_HAS_FULL_SPECIALIZATION
              ${VTK_BINARY_DIR}/CMakeTmp
              ${VTK_CMAKE_DIR}/vtkTestFullSpecialization.cxx
              OUTPUT_VARIABLE OUTPUT)
  IF(VTK_COMPILER_HAS_FULL_SPECIALIZATION)
    MESSAGE(STATUS "Checking support for full template specialization syntax -- yes")
    SET(VTK_COMPILER_HAS_FULL_SPECIALIZATION 1 CACHE INTERNAL "Support for full template specialization syntax")
    WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
      "Determining if the C++ compiler supports full template specialization syntax "
      "passed with the following output:\n"
      "${OUTPUT}\n" APPEND)
  ELSE(VTK_COMPILER_HAS_FULL_SPECIALIZATION)
    MESSAGE(STATUS "Checking support for full template specialization syntax -- no")
    SET(VTK_COMPILER_HAS_FULL_SPECIALIZATION 0 CACHE INTERNAL "Support for full template specialization syntax")
    WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
      "Determining if the C++ compiler supports full template specialization syntax "
      "failed with the following output:\n"
      "${OUTPUT}\n" APPEND)
  ENDIF(VTK_COMPILER_HAS_FULL_SPECIALIZATION)
ENDIF("VTK_COMPILER_HAS_FULL_SPECIALIZATION" MATCHES "^VTK_COMPILER_HAS_FULL_SPECIALIZATION$")

IF("VTK_TYPE_CHAR_IS_SIGNED" MATCHES "^VTK_TYPE_CHAR_IS_SIGNED$")
  MESSAGE(STATUS "Checking signedness of char")
  TRY_RUN(VTK_TYPE_CHAR_IS_SIGNED VTK_TYPE_CHAR_IS_SIGNED_COMPILED
          ${VTK_BINARY_DIR}/CMakeTmp/Char
          ${VTK_CMAKE_DIR}/vtkTestCharSignedness.cxx)
  IF(VTK_TYPE_CHAR_IS_SIGNED_COMPILED)
    IF(VTK_TYPE_CHAR_IS_SIGNED)
      MESSAGE(STATUS "Checking signedness of char -- signed")
      SET(VTK_TYPE_CHAR_IS_SIGNED 1 CACHE INTERNAL "Whether char is signed.")
    ELSE(VTK_TYPE_CHAR_IS_SIGNED)
      MESSAGE(STATUS "Checking signedness of char -- unsigned")
      SET(VTK_TYPE_CHAR_IS_SIGNED 0 CACHE INTERNAL "Whether char is signed.")
    ENDIF(VTK_TYPE_CHAR_IS_SIGNED)
  ELSE(VTK_TYPE_CHAR_IS_SIGNED_COMPILED)
    MESSAGE(STATUS "Checking signedness of char -- failed")
  ENDIF(VTK_TYPE_CHAR_IS_SIGNED_COMPILED)
ENDIF("VTK_TYPE_CHAR_IS_SIGNED" MATCHES "^VTK_TYPE_CHAR_IS_SIGNED$")

INCLUDE(Parallel/VTKParallelCMakeTests.cmake)

# Check for explicit template instantiation support by compiler.
INCLUDE(${VTK_CMAKE_DIR}/vtkTestExplicitInstantiation.cmake)

# Setup clean configuration of vtkConfigure.h and vtkToolkits.h.
MACRO(VTK_PREPARE_CMAKEDEFINE not invar outvar)
  IF(${not} ${invar})
    SET(${outvar} 1)
  ELSE(${not} ${invar})
    SET(${outvar})
  ENDIF(${not} ${invar})
ENDMACRO(VTK_PREPARE_CMAKEDEFINE)
VTK_PREPARE_CMAKEDEFINE("" CMAKE_WORDS_BIGENDIAN VTK_WORDS_BIGENDIAN)
VTK_PREPARE_CMAKEDEFINE("" CMAKE_USE_PTHREADS VTK_USE_PTHREADS)
VTK_PREPARE_CMAKEDEFINE("" CMAKE_USE_SPROC VTK_USE_SPROC)
VTK_PREPARE_CMAKEDEFINE("" CMAKE_HP_PTHREADS VTK_HP_PTHREADS)
VTK_PREPARE_CMAKEDEFINE("" CMAKE_USE_WIN32_THREADS VTK_USE_WIN32_THREADS)
VTK_PREPARE_CMAKEDEFINE("" CMAKE_NO_ANSI_STRING_STREAM VTK_NO_ANSI_STRING_STREAM)
VTK_PREPARE_CMAKEDEFINE("" CMAKE_NO_STD_NAMESPACE VTK_NO_STD_NAMESPACE)
VTK_PREPARE_CMAKEDEFINE(NOT CMAKE_ANSI_FOR_SCOPE VTK_NO_FOR_SCOPE)
VTK_PREPARE_CMAKEDEFINE(NOT VTK_EXPLICIT_TEMPLATES
                  VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION)
VTK_PREPARE_CMAKEDEFINE(NOT VTK_COMPILER_HAS_FULL_SPECIALIZATION
                        VTK_NO_FULL_TEMPLATE_SPECIALIZATION)

#-----------------------------------------------------------------------------
# Include file dependency tracking regular expression.
SET(VTK_REGEX "vtk[^.]*\\.([^t]|t[^x]|tx[^x]|cxx|hxx)")
IF(VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION)
  # Track all .txx file dependencies.
  SET(VTK_REGEX_TXX "vtk[^.]*\\.txx")
ELSE(VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION)
  # Track all .txx file dependencies except *Implicit.txx files.
  SET(VTK_REGEX_TXX "vtk[^.]*([^t]|[^i]t|[^c]it|[^i]cit|[^l]icit|[^p]licit|[^m]plicit|[^I]mplicit)\\.txx")
ENDIF(VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION)
INCLUDE_REGULAR_EXPRESSION("^((lex|png|j|z|t|D).*|${VTK_REGEX}|${VTK_REGEX_TXX})$")

#-----------------------------------------------------------------------------
# Determine the set of language wrappers that should be built.
OPTION(VTK_WRAP_TCL "Wrap VTK classes into the TCL language." OFF)
OPTION(VTK_WRAP_PYTHON "Wrap VTK classes into the Python language." OFF)
OPTION(VTK_WRAP_JAVA "Wrap VTK classes into the Java language." OFF)

# Python requires shared libraries.
IF(VTK_WRAP_PYTHON)
  IF(NOT BUILD_SHARED_LIBS)
    MESSAGE(SEND_ERROR "VTK_WRAP_PYTHON requires BUILD_SHARED_LIBS to be ON.")
    SET(VTK_WRAP_PYTHON 0)
  ENDIF(NOT BUILD_SHARED_LIBS)
ENDIF(VTK_WRAP_PYTHON)

# Java requires shared libraries on Windows.
IF(VTK_WRAP_JAVA)
  IF(WIN32)
    IF(NOT BUILD_SHARED_LIBS)
      MESSAGE(SEND_ERROR "VTK_WRAP_JAVA requires BUILD_SHARED_LIBS to be ON.")
      SET(VTK_WRAP_JAVA 0)
    ENDIF(NOT BUILD_SHARED_LIBS)
  ENDIF(WIN32)
ENDIF(VTK_WRAP_JAVA)

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
# Load the VTK CMake extension modules.  This must be done after the
# wrapping language selection.

# Import the VTK_COMPILE_CMAKE_EXTENSIONS and VTK_LOAD_CMAKE_EXTENSIONS macros
INCLUDE(${VTK_CMAKE_DIR}/vtkCompileCMakeExtensions.cmake)
INCLUDE(${VTK_CMAKE_DIR}/vtkLoadCMakeExtensions.cmake)

# Compile the extensions into the build tree and load them.
VTK_COMPILE_CMAKE_EXTENSIONS(${VTK_CMAKE_DIR} ${VTK_BINARY_DIR}/CMake
  VTK_CMAKE_EXTENSIONS_COMPILED)
IF(VTK_CMAKE_EXTENSIONS_COMPILED)
  VTK_LOAD_CMAKE_EXTENSIONS(${VTK_BINARY_DIR}/CMake)
ENDIF(VTK_CMAKE_EXTENSIONS_COMPILED)

#-----------------------------------------------------------------------------
# Configure Dart testing support.
INCLUDE(${CMAKE_ROOT}/Modules/Dart.cmake)
MARK_AS_ADVANCED(BUILD_TESTING DART_ROOT TCL_TCLSH)
IF(BUILD_TESTING)
  ENABLE_TESTING()
  CONFIGURE_FILE(${VTK_CMAKE_DIR}/CTestCustom.ctest.in
    ${VTK_BINARY_DIR}/CMake/CTestCustom.ctest @ONLY)
  OPTION(VTK_USE_DISPLAY "Turn this option off and tests will not popup windows" ON)
  MARK_AS_ADVANCED(VTK_USE_DISPLAY)
ENDIF(BUILD_TESTING)

SET(CMAKE_TESTDRIVER_BEFORE_TESTMAIN "    try {")
SET(CMAKE_TESTDRIVER_AFTER_TESTMAIN "    }
    catch(vtkstd::exception& e)
      {
      fprintf(stderr, \"Test driver caught exception: [%s]\\n\", e.what());
      result = -1;
      }")

#-----------------------------------------------------------------------------
# Select a streams library.

INCLUDE(${VTK_CMAKE_DIR}/vtkSelectStreamsLibrary.cmake)
VTK_SELECT_STREAMS_LIBRARY(VTK_USE_ANSI_STDLIB ${VTK_SOURCE_DIR})

# Check the severity of EOF bugs in the streams library.
SET(VTK_TEST_STREAM_EOF_CXX ${VTK_CMAKE_DIR}/vtkTestStreamEOF.cxx.in)
CONFIGURE_FILE(${VTK_CMAKE_DIR}/vtkTestStreamEOF.cxx.in
  ${VTK_BINARY_DIR}/CMake/vtkTestStreamEOF.cxx @ONLY IMMEDIATE)
IF(VTK_USE_ANSI_STDLIB)
  IF("VTK_ANSI_STREAM_EOF_RESULT" MATCHES "^VTK_ANSI_STREAM_EOF_RESULT$")
    MESSAGE(STATUS "Checking ANSI streams end-of-file bug level")
    TRY_RUN(VTK_ANSI_STREAM_EOF_RESULT VTK_ANSI_STREAM_EOF_COMPILED
      ${VTK_BINARY_DIR}/CMakeTmp
      ${VTK_BINARY_DIR}/CMake/vtkTestStreamEOF.cxx)
    IF(VTK_ANSI_STREAM_EOF_COMPILED)
      MESSAGE(STATUS "Checking ANSI streams end-of-file bug level - ${VTK_ANSI_STREAM_EOF_RESULT}")
    ELSE(VTK_ANSI_STREAM_EOF_COMPILED)
      SET(VTK_ANSI_STREAM_EOF_RESULT 0)
      MESSAGE(STATUS "Checking ANSI streams end-of-file bug level - failed to compile test")
    ENDIF(VTK_ANSI_STREAM_EOF_COMPILED)
  ENDIF("VTK_ANSI_STREAM_EOF_RESULT" MATCHES "^VTK_ANSI_STREAM_EOF_RESULT$")
  SET(VTK_STREAM_EOF_SEVERITY ${VTK_ANSI_STREAM_EOF_RESULT})
ELSE(VTK_USE_ANSI_STDLIB)
  IF("VTK_OLD_STREAM_EOF_RESULT" MATCHES "^VTK_OLD_STREAM_EOF_RESULT$")
    MESSAGE(STATUS "Checking old streams end-of-file bug level")
    TRY_RUN(VTK_OLD_STREAM_EOF_RESULT VTK_OLD_STREAM_EOF_COMPILED
      ${VTK_BINARY_DIR}/CMakeTmp
      ${VTK_BINARY_DIR}/CMake/vtkTestStreamEOF.cxx)
    IF(VTK_OLD_STREAM_EOF_COMPILED)
      MESSAGE(STATUS "Checking old streams end-of-file bug level - ${VTK_OLD_STREAM_EOF_RESULT}")
    ELSE(VTK_OLD_STREAM_EOF_COMPILED)
      SET(VTK_OLD_STREAM_EOF_RESULT 0)
      MESSAGE(STATUS "Checking old streams end-of-file bug level - failed to compile test")
    ENDIF(VTK_OLD_STREAM_EOF_COMPILED)
  ENDIF("VTK_OLD_STREAM_EOF_RESULT" MATCHES "^VTK_OLD_STREAM_EOF_RESULT$")
  SET(VTK_STREAM_EOF_SEVERITY ${VTK_OLD_STREAM_EOF_RESULT})
ENDIF(VTK_USE_ANSI_STDLIB)

IF(VTK_SIZEOF_LONG_LONG)
  CONFIGURE_FILE(${VTK_CMAKE_DIR}/vtkTestStreamLongLong.cxx.in
    ${VTK_BINARY_DIR}/CMake/vtkTestStreamLongLong.cxx @ONLY IMMEDIATE)
  IF("VTK_OSTREAM_SUPPORTS_LONG_LONG" MATCHES "^VTK_OSTREAM_SUPPORTS_LONG_LONG$")
    MESSAGE(STATUS "Checking if ostream supports long long")
    TRY_COMPILE(VTK_OSTREAM_SUPPORTS_LONG_LONG
      ${VTK_BINARY_DIR}
      ${VTK_BINARY_DIR}/CMake/vtkTestStreamLongLong.cxx
      COMPILE_DEFINITIONS -DVTK_TEST_OSTREAM_LONG_LONG
      OUTPUT_VARIABLE OUTPUT)
    IF(VTK_OSTREAM_SUPPORTS_LONG_LONG)
      MESSAGE(STATUS "Checking if ostream supports long long -- yes")
      SET(VTK_OSTREAM_SUPPORTS_LONG_LONG 1 CACHE INTERNAL "Whether ostream supports long long")
      WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
        "Determining if ostream supports long long "
        "passed with the following output:\n"
        "${OUTPUT}\n" APPEND)
    ELSE(VTK_OSTREAM_SUPPORTS_LONG_LONG)
      MESSAGE(STATUS "Checking if ostream supports long long -- no")
      SET(VTK_OSTREAM_SUPPORTS_LONG_LONG 0 CACHE INTERNAL "Whether ostream supports long long")
      WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
        "Determining if ostream supports long long "
        "failed with the following output:\n"
        "${OUTPUT}\n" APPEND)
    ENDIF(VTK_OSTREAM_SUPPORTS_LONG_LONG)
  ENDIF("VTK_OSTREAM_SUPPORTS_LONG_LONG" MATCHES "^VTK_OSTREAM_SUPPORTS_LONG_LONG$")
  IF("VTK_ISTREAM_SUPPORTS_LONG_LONG" MATCHES "^VTK_ISTREAM_SUPPORTS_LONG_LONG$")
    MESSAGE(STATUS "Checking if istream supports long long")
    TRY_COMPILE(VTK_ISTREAM_SUPPORTS_LONG_LONG
      ${VTK_BINARY_DIR}
      ${VTK_BINARY_DIR}/CMake/vtkTestStreamLongLong.cxx
      COMPILE_DEFINITIONS -DVTK_TEST_ISTREAM_LONG_LONG
      OUTPUT_VARIABLE OUTPUT)
    IF(VTK_ISTREAM_SUPPORTS_LONG_LONG)
      MESSAGE(STATUS "Checking if istream supports long long -- yes")
      SET(VTK_ISTREAM_SUPPORTS_LONG_LONG 1 CACHE INTERNAL "Whether istream supports long long")
      WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
        "Determining if istream supports long long "
        "passed with the following output:\n"
        "${OUTPUT}\n" APPEND)
    ELSE(VTK_ISTREAM_SUPPORTS_LONG_LONG)
      MESSAGE(STATUS "Checking if istream supports long long -- no")
      SET(VTK_ISTREAM_SUPPORTS_LONG_LONG 0 CACHE INTERNAL "Whether istream supports long long")
      WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
        "Determining if istream supports long long "
        "failed with the following output:\n"
        "${OUTPUT}\n" APPEND)
    ENDIF(VTK_ISTREAM_SUPPORTS_LONG_LONG)
  ENDIF("VTK_ISTREAM_SUPPORTS_LONG_LONG" MATCHES "^VTK_ISTREAM_SUPPORTS_LONG_LONG$")
ENDIF(VTK_SIZEOF_LONG_LONG)

IF(VTK_USE_RENDERING AND WIN32)
  # Check for vfw32 support
  IF("VTK_USE_VIDEO_FOR_WINDOWS" MATCHES "^VTK_USE_VIDEO_FOR_WINDOWS$")
    MESSAGE(STATUS "Checking if vfw32 is available")
    TRY_COMPILE(VTK_USE_VIDEO_FOR_WINDOWS_DEFAULT
      ${VTK_BINARY_DIR}/CMakeTmp
      ${VTK_CMAKE_DIR}/vtkTestvfw32.cxx
      CMAKE_FLAGS "-DLINK_LIBRARIES:STRING=vfw32"
      OUTPUT_VARIABLE OUTPUT)
    IF(VTK_USE_VIDEO_FOR_WINDOWS_DEFAULT)
      MESSAGE(STATUS "Checking if vfw32 is available -- yes")
      OPTION(VTK_USE_VIDEO_FOR_WINDOWS "Enable using Video for Windows (vfw32) for video input and output." ON)
      WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
        "Checking if vfw32 is available "
        "passed with the following output:\n"
        "${OUTPUT}\n" APPEND)
    ELSE(VTK_USE_VIDEO_FOR_WINDOWS_DEFAULT)
      MESSAGE(STATUS "Checking if vfw32 is available -- no")
      OPTION(VTK_USE_VIDEO_FOR_WINDOWS "Enable using Video for Windows (vfw32) for video input and output." OFF)
      WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
        "Checking if vfw32 is available "
        "failed with the following output:\n"
        "${OUTPUT}\n" APPEND)
    ENDIF(VTK_USE_VIDEO_FOR_WINDOWS_DEFAULT)
    MARK_AS_ADVANCED(VTK_USE_VIDEO_FOR_WINDOWS)
  ENDIF("VTK_USE_VIDEO_FOR_WINDOWS" MATCHES "^VTK_USE_VIDEO_FOR_WINDOWS$")

  # Check if vfw32 supports the video capture functions
  IF(VTK_USE_VIDEO_FOR_WINDOWS)
    IF("VTK_VFW_SUPPORTS_CAPTURE" MATCHES "^VTK_VFW_SUPPORTS_CAPTURE$")
      MESSAGE(STATUS "Checking if vfw32 supports video capture")
      TRY_COMPILE(VTK_VFW_SUPPORTS_CAPTURE
        ${VTK_BINARY_DIR}/CMakeTmp
        ${VTK_CMAKE_DIR}/vtkTestvfw32Capture.cxx
        CMAKE_FLAGS "-DLINK_LIBRARIES:STRING=vfw32"
        OUTPUT_VARIABLE OUTPUT)
      IF(VTK_VFW_SUPPORTS_CAPTURE)
        MESSAGE(STATUS "Checking if vfw32 supports video capture -- yes")
        SET(VTK_VFW_SUPPORTS_CAPTURE 1 CACHE INTERNAL "Enable using Video for Windows (vfw32) for video capture.")
        WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
          "Checking if vfw32 supports video capture "
          "passed with the following output:\n"
          "${OUTPUT}\n" APPEND)
      ELSE(VTK_VFW_SUPPORTS_CAPTURE)
        MESSAGE(STATUS "Checking if vfw32 supports video capture -- no")
        SET(VTK_VFW_SUPPORTS_CAPTURE 0 CACHE INTERNAL "Enable using Video for Windows (vfw32) for video capture.")
        WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
          "Checking if vfw32 supports video capture "
          "failed with the following output:\n"
          "${OUTPUT}\n" APPEND)
      ENDIF(VTK_VFW_SUPPORTS_CAPTURE)
    ENDIF("VTK_VFW_SUPPORTS_CAPTURE" MATCHES "^VTK_VFW_SUPPORTS_CAPTURE$")
  ELSE(VTK_USE_VIDEO_FOR_WINDOWS)
    SET(VTK_VFW_SUPPORTS_CAPTURE 0)
  ENDIF(VTK_USE_VIDEO_FOR_WINDOWS)
ENDIF(VTK_USE_RENDERING AND WIN32)

#-----------------------------------------------------------------------------
# Configure KWSys to be named "vtksys".
SET(KWSYS_NAMESPACE vtksys)
SET(KWSYS_USE_Process 1)
SET(KWSYS_USE_SystemTools 1)
SET(KWSYS_USE_RegularExpression 1)
SET(KWSYS_USE_CommandLineArguments 1)
SET(KWSYS_USE_Base64 1)
SET(KWSYS_HEADER_ROOT ${VTK_BINARY_DIR}/Utilities)
SET(KWSYS_PROPERTIES_CXX ${VTK_LIBRARY_PROPERTIES})
IF(NOT VTK_USE_ANSI_STDLIB)
  SET(KWSYS_IOS_FORCE_OLD 1)
ENDIF(NOT VTK_USE_ANSI_STDLIB)
IF(NOT VTK_INSTALL_NO_LIBRARIES)
  SET(KWSYS_LIBRARY_INSTALL_DIR ${VTK_INSTALL_LIB_DIR})
ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  SET(KWSYS_HEADER_INSTALL_DIR ${VTK_INSTALL_INCLUDE_DIR})
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

#-----------------------------------------------------------------------------
# Dispatch the build into the proper subdirectories.


SET(VTK_HAS_EXODUS)
IF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.8)
  SET(VTK_HAS_EXODUS 1)
ELSE("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.8)
  MESSAGE(STATUS "CMake older than 2.0 detected. NetCDF will be disabled")
ENDIF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.8)

# Utility libraries and executables.
SUBDIRS(Wrapping Utilities)

# Build the subdirector for each kit.

SUBDIRS(
  Common
  Filtering
  Imaging
  Graphics
  GenericFiltering
  IO)

IF(VTK_USE_RENDERING)
  SUBDIRS(Rendering)
  SUBDIRS(VolumeRendering)
  SUBDIRS(Hybrid)
  SUBDIRS(Widgets)
ENDIF(VTK_USE_RENDERING)
IF(VTK_USE_PARALLEL)
  SUBDIRS(Parallel)
ENDIF(VTK_USE_PARALLEL)
IF(VTK_USE_GUISUPPORT)
  SUBDIRS(GUISupport)
ENDIF(VTK_USE_GUISUPPORT)

# Wrapping.
IF(VTK_WRAP_TCL)
  SUBDIRS(Wrapping/Tcl)
ENDIF(VTK_WRAP_TCL)
IF(VTK_WRAP_PYTHON)
  SUBDIRS(Wrapping/Python)
ENDIF(VTK_WRAP_PYTHON)
IF(VTK_WRAP_JAVA)
  SUBDIRS(Wrapping/Java)
ENDIF(VTK_WRAP_JAVA)

# Testing.
IF(BUILD_TESTING)
  MAKE_DIRECTORY(${VTK_BINARY_DIR}/Testing/Temporary)
  # Build scripts to convert tcl tests to python
  SUBDIRS(Utilities/vtkTclTest2Py)
  SUBDIRS(Common/Testing Filtering/Testing Graphics/Testing GenericFiltering/Testing Imaging/Testing
          IO/Testing)
  IF(VTK_USE_RENDERING)
    SUBDIRS(Rendering/Testing)
    SUBDIRS(VolumeRendering/Testing)
    SUBDIRS(Hybrid/Testing)
    SUBDIRS(Widgets/Testing)
  ENDIF(VTK_USE_RENDERING)
  IF(VTK_USE_PARALLEL)
    SUBDIRS(Parallel/Testing)
  ENDIF(VTK_USE_PARALLEL)
ENDIF(BUILD_TESTING)


# Include the examples if they are enabled.  Note that the in-tree
# build adds tests and a custom target to build the examples project
# in a separate build tree.  The examples are not directly included in
# the VTK build.  Doing so will not work because they are designed to
# be built out-of-tree.
IF(BUILD_EXAMPLES)
  SET(VTK_CMAKE_VERSION_FOR_EXAMPLES)
  # The examples tree now requires CMake 2.0.4 or higher.
  IF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.9)
    IF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" MATCHES "^2.0$")
      # Have version 2.0.  Check for the Modules/CMakeTestForFreeVC.cxx
      # file which was added in 2.0.4 (this is a nasty hack).
      IF(EXISTS ${CMAKE_ROOT}/Modules/CMakeTestForFreeVC.cxx)
        SET(VTK_CMAKE_VERSION_FOR_EXAMPLES 1)
      ENDIF(EXISTS ${CMAKE_ROOT}/Modules/CMakeTestForFreeVC.cxx)
    ELSE("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" MATCHES "^2.0$")
      # Have version 2.1 or higher.
      SET(VTK_CMAKE_VERSION_FOR_EXAMPLES 1)
    ENDIF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" MATCHES "^2.0$")
  ENDIF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.9)
  IF(VTK_CMAKE_VERSION_FOR_EXAMPLES)
    SUBDIRS(Examples)
  ELSE(VTK_CMAKE_VERSION_FOR_EXAMPLES)
    MESSAGE(
      "Warning: The BUILD_EXAMPLES option now requires CMake 2.0.4 or higher "
      "and is being ignored.")
  ENDIF(VTK_CMAKE_VERSION_FOR_EXAMPLES)
ENDIF(BUILD_EXAMPLES)

#-----------------------------------------------------------------------------
# Provide a few configuration options.
OPTION(BUILD_EXAMPLES "Build VTK examples." OFF)
OPTION(VTK_USE_64BIT_IDS "Build VTK with 64 bit ids" OFF)
OPTION(VTK_DEBUG_LEAKS "Build leak checking support into VTK." OFF)
MARK_AS_ADVANCED(VTK_DEBUG_LEAKS VTK_USE_64BIT_IDS)

VTK_DEPENDENT_OPTION(VTK_USE_MANGLED_MESA "Use mangled Mesa with OpenGL." OFF
                     "VTK_USE_RENDERING" OFF)
VTK_DEPENDENT_OPTION(VTK_OPENGL_HAS_OSMESA
                     "The opengl library being used supports off screen Mesa calls." OFF
                     "VTK_USE_RENDERING;UNIX" OFF)
SET(VTK_CAN_DO_OFF_SCREEN)
IF(VTK_USE_MANGLED_MESA OR VTK_OPENGL_HAS_OSMESA)
  SET(VTK_CAN_DO_OFF_SCREEN 1)
ENDIF(VTK_USE_MANGLED_MESA OR VTK_OPENGL_HAS_OSMESA)
VTK_DEPENDENT_OPTION(VTK_USE_OFFSCREEN
                     "The opengl library being used supports off screen Mesa calls." OFF
                     "VTK_CAN_DO_OFF_SCREEN" OFF)
VTK_DEPENDENT_OPTION(VTK_USE_MPI
                     "Use Message Passing Interface (MPI) library for parallel support." OFF
                     "VTK_USE_PARALLEL" OFF)
VTK_DEPENDENT_OPTION(VTK_USE_MATROX_IMAGING
                     "Use Matrox Imaging Library for video input." OFF
                     "VTK_USE_RENDERING;WIN32" OFF)
VTK_DEPENDENT_OPTION(VTK_USE_GL2PS "Build VTK with gl2ps support." OFF
                     "VTK_USE_RENDERING" ON)

SET(VTK_CAN_USE_TK)
IF(VTK_WRAP_PYTHON OR VTK_WRAP_TCL)
  IF(NOT VTK_USE_COCOA)
    IF(NOT VTK_DISABLE_TK_INIT)
      SET(VTK_CAN_USE_TK 1)
    ENDIF(NOT VTK_DISABLE_TK_INIT)
  ENDIF(NOT VTK_USE_COCOA)
ENDIF(VTK_WRAP_PYTHON OR VTK_WRAP_TCL)
VTK_DEPENDENT_OPTION(VTK_USE_TK "Build VTK with Tk support" ON
                     "VTK_CAN_USE_TK" OFF)

MARK_AS_ADVANCED(VTK_OPENGL_HAS_OSMESA
                 VTK_USE_OFFSCREEN
                 VTK_USE_TK
                 VTK_USE_GL2PS
                 VTK_USE_MANGLED_MESA
                 VTK_USE_MATROX_IMAGING
                 VTK_USE_MPI)

#-----------------------------------------------------------------------------
# Provide options to use system versions of third-party libraries.
VTK_THIRD_PARTY_OPTION(ZLIB zlib)
VTK_THIRD_PARTY_OPTION(JPEG jpeg)
VTK_THIRD_PARTY_OPTION(PNG  png)
VTK_THIRD_PARTY_OPTION(TIFF tiff)
VTK_THIRD_PARTY_OPTION(EXPAT expat)
VTK_THIRD_PARTY_OPTION(FREETYPE freetype)

#-----------------------------------------------------------------------------
# Configure OpenGL support.
IF(VTK_USE_RENDERING)
  INCLUDE(${CMAKE_ROOT}/Modules/FindOpenGL.cmake)
ENDIF(VTK_USE_RENDERING)

VTK_PREPARE_CMAKEDEFINE("" OPENGL_LIBRARY VTK_USE_OPENGL_LIBRARY)

#-----------------------------------------------------------------------------
# Configure Mangled MESA support.
IF(VTK_USE_MANGLED_MESA)
  INCLUDE(${VTK_CMAKE_DIR}/FindMangledMesa.cmake)

  MARK_AS_ADVANCED(MANGLED_MESA_INCLUDE_DIR MANGLED_MESA_LIBRARY
                   MANGLED_OSMESA_INCLUDE_DIR MANGLED_OSMESA_LIBRARY)

  IF(MANGLED_MESA_INCLUDE_DIR)
    USE_MANGLED_MESA(${MANGLED_MESA_INCLUDE_DIR}/GL
                     ${VTK_BINARY_DIR}/MangleMesaInclude)
  ENDIF(MANGLED_MESA_INCLUDE_DIR)
ENDIF(VTK_USE_MANGLED_MESA)

#-----------------------------------------------------------------------------
# Configure Off-Screen MESA support.
IF(VTK_OPENGL_HAS_OSMESA)
  INCLUDE(${VTK_CMAKE_DIR}/FindOSMesa.cmake)
  MARK_AS_ADVANCED(OSMESA_INCLUDE_DIR OSMESA_LIBRARY)
ENDIF(VTK_OPENGL_HAS_OSMESA)

# Off-Screen MESA cannot be used with Mangled MESA.
IF(VTK_OPENGL_HAS_OSMESA AND VTK_USE_MANGLED_MESA)
  MESSAGE(SEND_ERROR
    "Off-Screen MESA cannot be used with Mangled MESA.  Turn off either "
    "VTK_OPENGL_HAS_OSMESA or VTK_USE_MANGLED_MESA.")
ENDIF(VTK_OPENGL_HAS_OSMESA AND VTK_USE_MANGLED_MESA)

#-----------------------------------------------------------------------------
# Configure CG-Shading support.
IF(VTK_USE_CG_SHADERS)
  INCLUDE(${VTK_CMAKE_DIR}/FindCg.cmake)
  MARK_AS_ADVANCED(CG_COMPILER CG_INCLUDE_PATH CG_LIBRARY CG_GL_LIBRARY)
ENDIF(VTK_USE_CG_SHADERS)

#-----------------------------------------------------------------------------
# Configure GLSL-Shading support.
# TODO: Should verify that GL2.0 is supported.

#-----------------------------------------------------------------------------
# Configure Matrox Imaging support.
IF(VTK_USE_MATROX_IMAGING)
  FIND_LIBRARY(MIL_LIBRARY MIL
    "C:/Program Files/Matrox Imaging/mil/library/winnt/msc/dll"
    "C:/Program Files/Matrox Imaging/mil/library/windows/msc/dll"
    )
  FIND_PATH(MIL_INCLUDE_PATH mil.h
    "C:/Program Files/Matrox Imaging/mil/include"
    )
ENDIF(VTK_USE_MATROX_IMAGING)

#-----------------------------------------------------------------------------
# Configure MPI testing support.
# FLAGS used and set for MPI testing
# VTK_MPIRUN_EXE - full path to mpirun command
# VTK_MPI_NUMPROC_FLAG - flag that is used to tell this mpirun how many procs to start
# VTK_MPI_PREFLAGS - flags used directly before process to be run by mpirun
# VTK_MPI_POSTFLAGS - flags used after all other flags by mpirun
# So, tests will be run something like this:
# ${VTK_MPIRUN_EXE} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS} executable ${VTK_MPI_POSTFLAGS}
#
IF(VTK_USE_MPI AND BUILD_TESTING)
  FIND_PROGRAM(VTK_MPIRUN_EXE NAMES mpirun lamexec PATHS "C:/Program Files/MPICH/mpd/bin")
  SET(VTK_MPI_NUMPROC_FLAG "-np" CACHE STRING "Flag used by mpi to specify the number of processes, the next option will be the number of processes. (see ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt for more info.)")
  SET(VTK_MPI_PREFLAGS "" CACHE STRING "These flags will be directly before the executable that is being run by VTK_MPIRUN_EXE. (see ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt for more info.)")
  SET(VTK_MPI_POSTFLAGS "" CACHE STRING "These flags will come after all flags given to MPIRun.(see ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt for more info.)")
  SET(VTK_MPI_MAX_NUMPROCS "2" CACHE STRING "Maximum number of processors available to run parallel applications. (see ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt for more info.)")
  MARK_AS_ADVANCED(
    VTK_MPI_NUMPROC_FLAG VTK_MPIRUN_EXE VTK_MPI_PREFLAGS VTK_MPI_POSTFLAGS VTK_MPI_MAX_NUMPROCS)
  SEPARATE_ARGUMENTS(VTK_MPI_PREFLAGS)
  SEPARATE_ARGUMENTS(VTK_MPI_POSTFLAGS)
ENDIF(VTK_USE_MPI AND BUILD_TESTING)

#-----------------------------------------------------------------------------
# Create STL header wrappers to block warnings in the STL headers.
FOREACH(header algorithm deque iterator list map numeric queue set stack string
               utility vector exception stdexcept)
  SET(VTK_STL_HEADER "${header}")
  CONFIGURE_FILE(${VTK_SOURCE_DIR}/Utilities/vtkstd.h.in
                 ${VTK_BINARY_DIR}/vtkstd/${header} @ONLY IMMEDIATE)
  IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
    INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR}/vtkstd
                  FILES ${VTK_BINARY_DIR}/vtkstd/${header})
  ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
ENDFOREACH(header)

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
# Configure Tcl wrapping support.

MACRO (VTK_INCLUDE_TCL_TK_MODULES)
  INCLUDE(${CMAKE_ROOT}/Modules/FindTCL.cmake)
  SET(VTK_TCL_LIBRARIES ${TCL_LIBRARY})
  IF(UNIX)
    # The tcl library needs the math library on unix.
    SET(VTK_TCL_LIBRARIES ${VTK_TCL_LIBRARIES} m)
  ENDIF(UNIX)
  IF(VTK_USE_TK)
    SET(VTK_TK_LIBRARIES ${TK_LIBRARY} ${VTK_TCL_LIBRARIES})
  ENDIF(VTK_USE_TK)
  INCLUDE(${VTK_CMAKE_DIR}/vtkTclTkMacros.cmake)
  # Hide useless settings provided by FindTCL.
  FOREACH(entry 
          TCL_LIBRARY_DEBUG
          TK_LIBRARY_DEBUG
                  TCL_STUB_LIBRARY
                  TCL_STUB_LIBRARY_DEBUG
                  TK_STUB_LIBRARY
                  TK_STUB_LIBRARY_DEBUG
          TK_WISH)
    SET(${entry} "${${entry}}" CACHE INTERNAL "This value is not used by VTK.")
  ENDFOREACH(entry)
ENDMACRO (VTK_INCLUDE_TCL_TK_MODULES)

IF(VTK_WRAP_TCL)
  INCLUDE("${VTK_CMAKE_DIR}/vtkWrapTcl.cmake")

  VTK_INCLUDE_TCL_TK_MODULES()

  # Wrapping executables.
  UTILITY_SOURCE(VTK_WRAP_TCL_EXE vtkWrapTcl Wrapping vtkWrapTcl.c)
  UTILITY_SOURCE(VTK_WRAP_TCL_INIT_EXE vtkWrapTclInit 
                 Wrapping vtkWrapTclInit.c)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping)
  MARK_AS_ADVANCED(VTK_WRAP_TCL_EXE VTK_WRAP_TCL_INIT_EXE VTK_WRAP_HINTS)

  # VTK tcl executables.
  SET(VTK_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/vtk)
  IF(VTK_USE_PARALLEL AND VTK_USE_MPI)
    SET(PVTK_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/pvtk)
  ENDIF(VTK_USE_PARALLEL AND VTK_USE_MPI)

  # Tcl package location.
  SET(VTK_TCL_HOME ${VTK_SOURCE_DIR}/Wrapping/Tcl)

  OPTION(VTK_TCL_TK_STATIC "Build with static Tcl/Tk support. TCL_LIBRARY and TK_LIBRARY must point to the corresponding Tcl/Tk static libraries (example, tcl84sx.lib, tk84sx.lib)." OFF)
  MARK_AS_ADVANCED(VTK_TCL_TK_STATIC)

ENDIF(VTK_WRAP_TCL)

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES(${VTK_INSTALL_PACKAGE_DIR}/CMake FILES
    ${VTK_CMAKE_DIR}/vtkTclTkMacros.cmake
    ${VTK_CMAKE_DIR}/vtkWrapTcl.cmake
    ${VTK_CMAKE_DIR}/vtkWrapJava.cmake
    ${VTK_CMAKE_DIR}/vtkWrapPython.cmake
    ${VTK_CMAKE_DIR}/vtkWrapperInit.data.in
    )
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

#-----------------------------------------------------------------------------
# Configure Python wrapping support.
IF(VTK_WRAP_PYTHON)
  # Tell vtkWrapPython.cmake to set VTK_PYTHON_LIBRARIES for us.
  SET(VTK_WRAP_PYTHON_FIND_LIBS 1)
  INCLUDE("${VTK_CMAKE_DIR}/vtkWrapPython.cmake")

  # Wrapping executables.
  UTILITY_SOURCE(VTK_WRAP_PYTHON_EXE vtkWrapPython Wrapping vtkWrapPython.c)
  UTILITY_SOURCE(VTK_WRAP_PYTHON_INIT_EXE vtkWrapPythonInit 
                   Wrapping vtkWrapPythonInit.c)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping)
  MARK_AS_ADVANCED(VTK_WRAP_PYTHON_EXE VTK_WRAP_PYTHON_INIT_EXE VTK_WRAP_HINTS)

  # VTK tcl executables.
  SET(VTK_PYTHON_EXE ${EXECUTABLE_OUTPUT_PATH}/vtkpython)
  IF(VTK_USE_PARALLEL AND VTK_USE_MPI)
    SET(PVTK_PYTHON_EXE ${EXECUTABLE_OUTPUT_PATH}/pvtkpython)
  ENDIF(VTK_USE_PARALLEL AND VTK_USE_MPI)

ENDIF(VTK_WRAP_PYTHON)

#-----------------------------------------------------------------------------
# Configure Java wrapping support.
IF(VTK_WRAP_JAVA)
  SET(VTK_WRAP_JAVA3_INIT_DIR "${VTK_SOURCE_DIR}/Wrapping")
  INCLUDE("${VTK_CMAKE_DIR}/vtkWrapJava.cmake")
  INCLUDE(${CMAKE_ROOT}/Modules/FindJava.cmake)
  INCLUDE(${CMAKE_ROOT}/Modules/FindJNI.cmake)

  # Wrapping executables.
  UTILITY_SOURCE(VTK_WRAP_JAVA_EXE vtkWrapJava Wrapping vtkWrapJava.c)
  UTILITY_SOURCE(VTK_PARSE_JAVA_EXE vtkParseJava Wrapping vtkParseJava.c)
  FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping)
  MARK_AS_ADVANCED(VTK_WRAP_JAVA_EXE VTK_PARSE_JAVA_EXE VTK_WRAP_HINTS)

  # Java package location.
  SET(VTK_JAVA_JAR ${LIBRARY_OUTPUT_PATH}/vtk.jar)
  SET(VTK_JAVA_HOME ${VTK_BINARY_DIR}/java/vtk)
  MAKE_DIRECTORY(${VTK_JAVA_HOME})
ENDIF(VTK_WRAP_JAVA)

VTK_PREPARE_CMAKEDEFINE("" JAVA_AWT_INCLUDE_PATH VTK_USE_JAWT)

#-----------------------------------------------------------------------------
# Configure the Tk library for vtkRendering.
IF(VTK_WRAP_TCL OR VTK_WRAP_PYTHON)
  IF(VTK_USE_RENDERING OR VTK_WRAP_TCL)
    SET(VTK_INCLUDE_NEED_TCL 1)
  ENDIF(VTK_USE_RENDERING OR VTK_WRAP_TCL)
  IF(VTK_USE_RENDERING)
    IF(VTK_USE_TK)
      SET(VTK_INCLUDE_NEED_TK 1)
    ENDIF(VTK_USE_TK)
  ENDIF(VTK_USE_RENDERING)
ENDIF(VTK_WRAP_TCL OR VTK_WRAP_PYTHON)

IF(VTK_USE_TK)
  INCLUDE(${VTK_SOURCE_DIR}/Wrapping/Tcl/vtkDetermineTkResources.cmake)
ENDIF(VTK_USE_TK)

IF(VTK_INCLUDE_NEED_TK)
  # Need Tk headers and libraries for python TK widgets
  IF(NOT VTK_WRAP_TCL)
    VTK_INCLUDE_TCL_TK_MODULES()
  ENDIF(NOT VTK_WRAP_TCL)

  # Need Tk sources on windows
  IF(WIN32)
    FIND_PATH(TK_XLIB_PATH
              X11/Xlib.h ${TK_INCLUDE_PATH}
              ${TK_INCLUDE_PATH}/../xlib)
    MARK_AS_ADVANCED(TK_XLIB_PATH)
  ENDIF(WIN32)
ENDIF(VTK_INCLUDE_NEED_TK)

# Need Tk internal headers for Tk initialization.

SET(VTK_RENDERING_NEED_TK_INTERNAL ${VTK_INCLUDE_NEED_TK})

IF(VTK_INCLUDE_NEED_TK)
  # Need Tk Internal headers to include tk.h on Cocoa
  IF(VTK_USE_RENDERING AND VTK_USE_COCOA)
     SET(VTK_RENDERING_NEED_TK_INTERNAL 1)
  ENDIF(VTK_USE_RENDERING AND VTK_USE_COCOA)
  # Do not need Tk Internal on Unix
  IF(UNIX)
    IF (NOT CYGWIN)
      IF (NOT APPLE)
        SET(VTK_RENDERING_NEED_TK_INTERNAL 0)
      ENDIF (NOT APPLE)
    ENDIF (NOT CYGWIN)
  ENDIF(UNIX)
ENDIF(VTK_INCLUDE_NEED_TK)

IF (VTK_RENDERING_NEED_TK_INTERNAL AND TK_LIBRARY)
  SET (try_file "")
  IF (CYGWIN OR WIN32)
    SET (try_file "tkWinPort.h")
  ENDIF (CYGWIN OR WIN32)
  IF (APPLE)
    SET (try_file "tkMacOSXPort.h")
  ENDIF (APPLE)
  IF (try_file)
    VTK_GET_TCL_TK_VERSION ("TCL_TK_MAJOR_VERSION" "TCL_TK_MINOR_VERSION")
    SET (TCL_TK_VERSIOND "${TCL_TK_MAJOR_VERSION}.${TCL_TK_MINOR_VERSION}")
    FIND_PATH(
       TK_INTERNAL_PATH 
       ${try_file} 
       "${VTK_SOURCE_DIR}/Utilities/TclTk/internals/tk${TCL_TK_VERSIOND}"
       DOC "The path to the Tk internal headers (${try_file}).")
    MARK_AS_ADVANCED(TK_INTERNAL_PATH)
  ENDIF (try_file)
ENDIF(VTK_RENDERING_NEED_TK_INTERNAL AND TK_LIBRARY)

#-----------------------------------------------------------------------------
# Configure the python executable for use by testing.

# Python executable is used by some tests whether VTK_WRAP_PYTHON is
# on or not.  do not add a VTK_WRAP_PYTHON to this if.
SET(VTK_NEED_PYTHON_EXECUTABLE 0)
IF(BUILD_TESTING)
  SET(VTK_NEED_PYTHON_EXECUTABLE 1)
ENDIF(BUILD_TESTING)

# If VTK_WRAP_PYTHON is on, then we need python executable to compile
# scripts.
IF(VTK_WRAP_PYTHON)
  SET(VTK_NEED_PYTHON_EXECUTABLE 1)
ENDIF(VTK_WRAP_PYTHON)

IF(VTK_NEED_PYTHON_EXECUTABLE)
  FIND_PROGRAM(PYTHON_EXECUTABLE
    NAMES python2.4 python2.3 python2.2 python2.1 python2.0 python1.6 python1.5 python
    PATHS
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.4\\InstallPath]
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.3\\InstallPath]
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.2\\InstallPath]
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.1\\InstallPath]
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.0\\InstallPath]
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\1.6\\InstallPath]
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\1.5\\InstallPath]
)
  MARK_AS_ADVANCED(PYTHON_EXECUTABLE)
ENDIF(VTK_NEED_PYTHON_EXECUTABLE)

#-----------------------------------------------------------------------------
# Configure the default VTK_DATA_ROOT for the location of VTKData.
FIND_PATH(VTK_DATA_ROOT VTKData.readme 
  ${VTK_SOURCE_DIR}/../VTKData 
  ${VTK_SOURCE_DIR}/../../VTKData 
  $ENV{VTK_DATA_ROOT})

#-----------------------------------------------------------------------------
# Configure files with settings for use by the build.
CONFIGURE_FILE(${VTK_SOURCE_DIR}/vtkConfigure.h.in
               ${VTK_BINARY_DIR}/vtkConfigure.h @ONLY IMMEDIATE)

CONFIGURE_FILE(${VTK_SOURCE_DIR}/UseVTK.cmake.in
               ${VTK_BINARY_DIR}/UseVTK.cmake COPYONLY IMMEDIATE)

CONFIGURE_FILE(${VTK_SOURCE_DIR}/vtkToolkits.h.in
               ${VTK_BINARY_DIR}/vtkToolkits.h @ONLY)

#-----------------------------------------------------------------------------
# The entire VTK tree should use the same include path.

# Create the list of include directories needed for VTK header files.
INCLUDE(${VTK_SOURCE_DIR}/vtkIncludeDirectories.cmake)

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
# Help other projects use VTK.

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  # Install the instantiator headers.
  INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} .h vtkCommonInstantiator
                vtkFilteringInstantiator vtkIOInstantiator
                vtkImagingInstantiator vtkGraphicsInstantiator
                vtkGenericFilteringInstantiator)
  IF(VTK_USE_RENDERING)
    INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} .h vtkRenderingInstantiator)
    INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} .h vtkVolumeRenderingInstantiator)
    INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} .h vtkHybridInstantiator)
    INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} .h vtkWidgetsInstantiator)
  ENDIF(VTK_USE_RENDERING)
  IF(VTK_USE_PARALLEL)
    INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} .h vtkParallelInstantiator)
  ENDIF(VTK_USE_PARALLEL)

  # Install cmake extensions so user projects can load them.
  INSTALL_FILES(${VTK_INSTALL_PACKAGE_DIR}/CMake FILES
    ${VTK_CMAKE_DIR}/vtkLoadCMakeExtensions.cmake 
    ${VTK_CMAKE_DIR}/vtkMakeInstantiator.cmake
    ${VTK_CMAKE_DIR}/vtkMakeInstantiator.h.in
    ${VTK_CMAKE_DIR}/vtkMakeInstantiator.cxx.in)
  IF (VTK_NEED_LOADED_COMMANDS)
    FOREACH(cmd VTK_WRAP_TCL2 VTK_WRAP_PYTHON2 VTK_WRAP_JAVA2
        VTK_GENERATE_JAVA_DEPENDENCIES)
      IF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 2.0)
        # CMake 2.2 and above will set CMAKE_LOADED_COMMAND_<command-name>
        # to the full path to the actual module that was loaded.  Use
        # this variable to find the module to install.
        INSTALL_FILES(${VTK_INSTALL_PACKAGE_DIR}/CMake FILES ${CMAKE_LOADED_COMMAND_${cmd}})
      ELSE("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 2.0)
        # CMake 2.0 will not tell us what module was loaded.  Construct the
        # name of the module for this platform.  If using a generator with
        # multiple configurations the loaded commands are always built Debug.
        IF(CMAKE_CONFIGURATION_TYPES)
          INSTALL_FILES(${VTK_INSTALL_PACKAGE_DIR}/CMake FILES
            "${VTK_BINARY_DIR}/CMake/Debug/${CMAKE_SHARED_MODULE_PREFIX}cm${cmd}${CMAKE_SHARED_MODULE_SUFFIX}")
        ELSE(CMAKE_CONFIGURATION_TYPES)
          INSTALL_FILES(${VTK_INSTALL_PACKAGE_DIR}/CMake FILES
            "${VTK_BINARY_DIR}/CMake/${CMAKE_SHARED_MODULE_PREFIX}cm${cmd}${CMAKE_SHARED_MODULE_SUFFIX}")
        ENDIF(CMAKE_CONFIGURATION_TYPES)
      ENDIF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 2.0)
    ENDFOREACH(cmd)
  ENDIF (VTK_NEED_LOADED_COMMANDS)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

# Save library dependencies.
EXPORT_LIBRARY_DEPENDENCIES(${VTK_BINARY_DIR}/VTKLibraryDepends.cmake)

# Install some files.
IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR} .h vtkConfigure vtkToolkits)
  INSTALL_FILES(${VTK_INSTALL_PACKAGE_DIR} .cmake UseVTK VTKLibraryDepends)
  IF(VTK_WRAP_HINTS)
    INSTALL_FILES(${VTK_INSTALL_PACKAGE_DIR} FILES ${VTK_WRAP_HINTS})
  ENDIF(VTK_WRAP_HINTS)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

#-----------------------------------------------------------------------------
# Allow local additions to this file without CVS conflicts.
INCLUDE(${VTK_BINARY_DIR}/LocalUserOptions.cmake OPTIONAL)
INCLUDE(${VTK_SOURCE_DIR}/LocalUserOptions.cmake OPTIONAL)

#-----------------------------------------------------------------------------
# The commands in this directory are intended to be executed as
# the end of the whole configuration process, as a "last step".
# This directory is typically the last SUBDIRS in the main CMakeLists.txt.
SUBDIRS(Utilities/LastConfigureStep)

# If the version of CMake was too old, complain and build nothing.
# These should be the last lines in this file.
ELSE("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.9)
  MESSAGE(SEND_ERROR
          "This version of CMake is too old to build VTK.  "
          "Please upgrade to CMake 2.0.")
ENDIF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 1.9)
