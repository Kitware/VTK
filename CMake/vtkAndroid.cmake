cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

#
# Instructions:
# 1. Download and install the Android NDK.
# 2. Run ccmake -DVTK_ANDROID_BUILD=ON /path/to/vtk/source
# 3. Set ANDROID_NDK to be the path to the NDK. (/opt/android-ndk by default)
# 4. Set API level and architecture.
# 5. Generate and make
#
include(ExternalProject)

# Convenience variables
set(PREFIX_DIR ${CMAKE_BINARY_DIR}/CMakeExternals/Prefix)
set(BUILD_DIR ${CMAKE_BINARY_DIR}/CMakeExternals/Build)
set(INSTALL_DIR ${CMAKE_BINARY_DIR}/CMakeExternals/Install)

# Android options
set (_ANDROID_NDK_DEFAULT "/opt/android-ndk")
if (DEFINED ENV{ANDROID_NDK})
  set (_ANDROID_NDK_DEFAULT "$ENV{ANDROID_NDK}")
endif()
set(ANDROID_NDK ${_ANDROID_NDK_DEFAULT} CACHE PATH
  "Set to the absolute path of the Android NDK root directory.\
 A \$\{ANDROID_NDK\}/platforms directory must exist."
  )
if (NOT EXISTS "${ANDROID_NDK}/platforms")
  message(FATAL_ERROR "Please set a valid ANDROID_NDK path")
endif()
set(ANDROID_NATIVE_API_LEVEL "21" CACHE STRING "Android Native API Level")
set(ANDROID_ARCH_ABI "armeabi" CACHE STRING "Target Android architecture/abi")

# find android
set(example_flags)
if (VTK_BUILD_EXAMPLES)
  find_program(ANDROID_EXECUTABLE
    NAMES android
    DOC   "The android command-line tool")
  if(NOT ANDROID_EXECUTABLE)
    message(FATAL_ERROR "Can not find android command line tool: android")
  endif()

  #find ant
  find_program(ANT_EXECUTABLE
    NAMES ant
    DOC   "The ant build tool")
  if(NOT ANT_EXECUTABLE)
    message(FATAL_ERROR "Can not find ant build tool: ant")
  endif()

  list(APPEND example_flags
    -DANDROID_EXECUTABLE:FILE=${ANDROID_EXECUTABLE}
    -DANT_EXECUTABLE:FILE=${ANT_EXECUTABLE}
  )
endif()

# Fail if the install path is invalid
if (NOT EXISTS ${CMAKE_INSTALL_PREFIX})
  message(FATAL_ERROR
    "Install path ${CMAKE_INSTALL_PREFIX} does not exist.")
endif()

# make sure we have a CTestCustom.cmake file
configure_file("${vtk_cmake_dir}/CTestCustom.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake" @ONLY)

# Compile a minimal VTK for its compile tools
macro(compile_vtk_tools)
  ExternalProject_Add(
    vtk-compile-tools
    SOURCE_DIR ${CMAKE_SOURCE_DIR}
    PREFIX ${CMAKE_BINARY_DIR}/CompileTools
    BINARY_DIR ${CMAKE_BINARY_DIR}/CompileTools
    INSTALL_COMMAND ""
    BUILD_ALWAYS 1
    CMAKE_CACHE_ARGS
      -DVTK_BUILD_COMPILE_TOOLS_ONLY:BOOL=ON
      -DCMAKE_BUILD_TYPE:STRING=Release
      -DVTK_BUILD_ALL_MODULES:BOOL=OFF
      -DBUILD_SHARED_LIBS:BOOL=ON
      -DVTK_BUILD_EXAMPLES:BOOL=OFF
      -DVTK_BUILD_TESTING:BOOL=OFF
      -DCMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM}
      -DVTK_ENABLE_LOGGING:BOOL=OFF
  )
endmacro()
compile_vtk_tools()

# Hide some CMake configs from the user
mark_as_advanced(
  BUILD_SHARED_LIBS
  CMAKE_INSTALL_PREFIX
  CMAKE_OSX_ARCHITECTURES
  CMAKE_OSX_DEPLOYMENT_TARGET
)

# Now cross-compile VTK with the android toolchain
set(android_cmake_flags
  ${example_flags}
  -DBUILD_SHARED_LIBS:BOOL=OFF
  -DVTK_BUILD_TESTING:STRING=OFF
  -DVTK_BUILD_EXAMPLES:BOOL=${VTK_BUILD_EXAMPLES}
  -DVTK_ENABLE_LOGGING:BOOL=OFF
  -DVTK_GROUP_ENABLE_Rendering:STRING=DONT_WANT
  -DVTK_GROUP_ENABLE_StandAlone:STRING=DONT_WANT
  -DVTK_MODULE_ENABLE_VTK_FiltersCore:STRING=YES
  -DVTK_MODULE_ENABLE_VTK_FiltersModeling:STRING=YES
  -DVTK_MODULE_ENABLE_VTK_FiltersSources:STRING=YES
  -DVTK_MODULE_ENABLE_VTK_FiltersGeometry:STRING=YES
  -DVTK_MODULE_ENABLE_VTK_IOGeometry:STRING=YES
  -DVTK_MODULE_ENABLE_VTK_IOLegacy:STRING=YES
  -DVTK_MODULE_ENABLE_VTK_IOImage:STRING=YES
  -DVTK_MODULE_ENABLE_VTK_IOPLY:STRING=YES
  -DVTK_MODULE_ENABLE_VTK_IOInfovis:STRING=YES
  -DVTK_MODULE_ENABLE_VTK_ImagingCore:STRING=YES
  -DVTK_MODULE_ENABLE_VTK_InteractionStyle:STRING=YES
  -DVTK_MODULE_ENABLE_VTK_ParallelCore:STRING=YES
  -DVTK_MODULE_ENABLE_VTK_RenderingCore:STRING=YES
  -DVTK_MODULE_ENABLE_VTK_RenderingFreeType:STRING=YES
  -DVTK_MODULE_ENABLE_VTK_TestingCore:STRING=YES
  -DVTK_MODULE_ENABLE_VTK_TestingRendering:STRING=YES
  -DVTK_MODULE_ENABLE_VTK_RenderingVolumeOpenGL2:STRING=YES
)

if (VTK_LEGACY_REMOVE)
  list(APPEND android_cmake_flags -DVTK_LEGACY_REMOVE:BOOL=ON)
endif()

macro(crosscompile target api abi out_build_dir)
  set(_ANDROID_API "${api}")
  set(_ANDROID_ABI "${abi}")
  set(_ANDROID_DIR "${target}-${api}-${abi}")
  set(_ANDROID_TOOLCHAIN ${BUILD_DIR}/${_ANDROID_DIR}-toolchain.cmake)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CMake/vtkAndroid-toolchain.cmake.in
    ${_ANDROID_TOOLCHAIN} @ONLY)
  ExternalProject_Add(
    ${target}
    SOURCE_DIR ${CMAKE_SOURCE_DIR}
    PREFIX ${PREFIX_DIR}/${_ANDROID_DIR}
    BINARY_DIR ${BUILD_DIR}/${_ANDROID_DIR}
    INSTALL_DIR ${INSTALL_DIR}/${_ANDROID_DIR}
    DEPENDS vtk-compile-tools
    BUILD_ALWAYS 1
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX:PATH=${INSTALL_DIR}/${target}
      -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
      -DCMAKE_TOOLCHAIN_FILE:PATH=${_ANDROID_TOOLCHAIN}
      -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
      -DVTKCompileTools_DIR:PATH=${CMAKE_BINARY_DIR}/CompileTools
      -DCMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM}
      ${android_cmake_flags}
  )
  set(${out_build_dir} "${BUILD_DIR}/${_ANDROID_DIR}")
endmacro()
crosscompile(vtk-android "${ANDROID_NATIVE_API_LEVEL}" "${ANDROID_ARCH_ABI}" vtk_android_build_dir)

# Having issues getting the test to run after some
# changes on the device we use for testing
#
# add_test(NAME AndroidNative
#     WORKING_DIRECTORY ${vtk_android_build_dir}/Examples/Android/NativeVTK/bin
#     COMMAND ${CMAKE_COMMAND}
#     -DWORKINGDIR=${vtk_android_build_dir}/Examples/Android/NativeVTK/bin
#     -P ${CMAKE_CURRENT_SOURCE_DIR}/Examples/Android/NativeVTK/runtest.cmake
#   )
