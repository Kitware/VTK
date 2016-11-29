cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

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
# Android options
if (DEFINED ENV{ANDROID_NDK})
  set(ANDROID_NDK "$ENV{ANDROID_NDK}" CACHE PATH "Path to the Android NDK")
else()
  set(ANDROID_NDK "/opt/android-ndk" CACHE PATH "Path to the Android NDK")
endif()
set(ANDROID_NATIVE_API_LEVEL "21" CACHE STRING "Android Native API Level")
set(ANDROID_ARCH_ABI "armeabi" CACHE STRING "Target Android architecture/abi")

# find android
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


# Fail if the install path is invalid
if (NOT EXISTS ${CMAKE_INSTALL_PREFIX})
  message(FATAL_ERROR
    "Install path ${CMAKE_INSTALL_PREFIX} does not exist.")
endif()

# Compile a minimal VTK for its compile tools
macro(compile_vtk_tools)
  ExternalProject_Add(
    vtk-compile-tools
    SOURCE_DIR ${CMAKE_SOURCE_DIR}
    PREFIX ${CMAKE_BINARY_DIR}/CompileTools
    BINARY_DIR ${CMAKE_BINARY_DIR}/CompileTools
    INSTALL_COMMAND ""
    BUILD_COMMAND ${CMAKE_COMMAND} --build . --config $<CONFIGURATION> --target vtkCompileTools
    BUILD_ALWAYS 1
    CMAKE_CACHE_ARGS
      -DCMAKE_BUILD_TYPE:STRING=Release
      -DVTK_BUILD_ALL_MODULES:BOOL=OFF
      -DVTK_Group_Rendering:BOOL=OFF
      -DVTK_Group_StandAlone:BOOL=ON
      -DBUILD_SHARED_LIBS:BOOL=ON
      -DBUILD_EXAMPLES:BOOL=OFF
      -DBUILD_TESTING:BOOL=OFF
      -DCMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM}
  )
endmacro()
compile_vtk_tools()

# Hide some CMake configs from the user
mark_as_advanced(
  BUILD_SHARED_LIBS
  CMAKE_INSTALL_PREFIX
  CMAKE_OSX_ARCHITECTURES
  CMAKE_OSX_DEPLOYMENT_TARGET
  VTK_RENDERING_BACKEND
)

# Now cross-compile VTK with the android toolchain
set(android_cmake_flags
  -DANDROID_EXECUTABLE:FILE=${ANDROID_EXECUTABLE}
  -DANT_EXECUTABLE:FILE=${ANT_EXECUTABLE}
  -DBUILD_SHARED_LIBS:BOOL=OFF
  -DBUILD_TESTING:BOOL=OFF
  -DBUILD_EXAMPLES:BOOL=${BUILD_EXAMPLES}
  -DVTK_RENDERING_BACKEND:STRING=OpenGL2
  -DVTK_Group_Rendering:BOOL=OFF
  -DVTK_Group_StandAlone:BOOL=OFF
  -DVTK_Group_Imaging:BOOL=OFF
  -DVTK_Group_MPI:BOOL=OFF
  -DVTK_Group_Views:BOOL=OFF
  -DVTK_Group_Qt:BOOL=OFF
  -DVTK_Group_Tk:BOOL=OFF
  -DVTK_Group_Web:BOOL=OFF
  -DModule_vtkFiltersCore:BOOL=ON
  -DModule_vtkFiltersModeling:BOOL=ON
  -DModule_vtkFiltersSources:BOOL=ON
  -DModule_vtkFiltersGeometry:BOOL=ON
  -DModule_vtkIOGeometry:BOOL=ON
  -DModule_vtkIOLegacy:BOOL=ON
  -DModule_vtkIOImage:BOOL=OFF
  -DModule_vtkIOPLY:BOOL=ON
  -DModule_vtkIOInfovis:BOOL=ON
  -DModule_vtkImagingCore:BOOL=ON
  -DModule_vtkInteractionStyle:BOOL=ON
  -DModule_vtkParallelCore:BOOL=ON
  -DModule_vtkRenderingCore:BOOL=ON
  -DModule_vtkRenderingFreeType:BOOL=ON
  -DModule_vtkTestingCore:BOOL=ON
  -DModule_vtkTestingRendering:BOOL=ON
  -DModule_vtkRenderingVolumeOpenGL2:BOOL=ON
)

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
      -DVTKCompileTools_DIR:PATH=${CMAKE_BINARY_DIR}/CompileTools
      -DCMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM}
      ${android_cmake_flags}
  )
  set(${out_build_dir} "${BUILD_DIR}/${_ANDROID_DIR}")
endmacro()
crosscompile(vtk-android "${ANDROID_NATIVE_API_LEVEL}" "${ANDROID_ARCH_ABI}" vtk_android_build_dir)

add_test(NAME AndroidNative
    WORKING_DIRECTORY ${vtk_android_build_dir}/Examples/Android/NativeVTK/bin
    COMMAND ${CMAKE_COMMAND}
    -DWORKINGDIR=${vtk_android_build_dir}/Examples/Android/NativeVTK/bin
    -P ${CMAKE_CURRENT_SOURCE_DIR}/Examples/Android/NativeVTK/runtest.cmake
  )
