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

set(OPENGL_ES_VERSION "2.0" CACHE STRING "OpenGL ES version (2.0 or 3.0)")
set_property(CACHE OPENGL_ES_VERSION PROPERTY STRINGS 2.0 3.0)

# Android options
set(ANDROID_NDK "/opt/android-ndk" CACHE PATH "Path to the Android NDK")
set(ANDROID_NATIVE_API_LEVEL "21" CACHE STRING "Android Native API Level")
set(ANDROID_ARCH_NAME "arm" CACHE STRING "Target Android architecture")

# Fail if the install path is invalid
if (NOT EXISTS ${CMAKE_INSTALL_PREFIX})
  message(FATAL_ERROR
    "Install path ${CMAKE_INSTALL_PREFIX} does not exist.")
endif()

# First, determine how to build
if (CMAKE_GENERATOR MATCHES "NMake Makefiles")
  set(VTK_BUILD_COMMAND BUILD_COMMAND nmake)
elseif (CMAKE_GENERATOR MATCHES "Ninja")
  set(VTK_BUILD_COMMAND BUILD_COMMAND ninja)
else()
  set(VTK_BUILD_COMMAND BUILD_COMMAND make)
endif()

# Compile a minimal VTK for its compile tools
macro(compile_vtk_tools)
  ExternalProject_Add(
    vtk-compile-tools
    SOURCE_DIR ${CMAKE_SOURCE_DIR}
    PREFIX ${PREFIX_DIR}/vtk-compile-tools
    BINARY_DIR ${BUILD_DIR}/vtk-compile-tools
    ${VTK_BUILD_COMMAND} vtkCompileTools
    CMAKE_ARGS
      -DCMAKE_BUILD_TYPE:STRING=Release
      -DVTK_BUILD_ALL_MODULES:BOOL=OFF
      -DVTK_Group_Rendering:BOOL=OFF
      -DVTK_Group_StandAlone:BOOL=ON
      -DBUILD_SHARED_LIBS:BOOL=ON
      -DBUILD_EXAMPLES:BOOL=OFF
      -DBUILD_TESTING:BOOL=OFF
  )
endmacro()
compile_vtk_tools()

# Hide some CMake configs from the user
mark_as_advanced(
  VTK_IOS_BUILD
  BUILD_SHARED_LIBS
  CMAKE_INSTALL_PREFIX
  CMAKE_OSX_ARCHITECTURES
  CMAKE_OSX_DEPLOYMENT_TARGET
  CMAKE_OSX_ROOT
  VTK_RENDERING_BACKEND
)

# Now cross-compile VTK with the android toolchain
set(android_cmake_flags
  -DANDROID_NDK:PATH=${ANDROID_NDK}
  -DANDROID_NATIVE_API_LEVEL:STRING=${ANDROID_NATIVE_API_LEVEL}
  -DANDROID_ARCH_NAME:STRING=${ANDROID_ARCH_NAME}
  -DBUILD_SHARED_LIBS:BOOL=OFF
  -DBUILD_TESTING:BOOL=OFF
  -DBUILD_EXAMPLES:BOOL=OFF
  -DVTK_RENDERING_BACKEND:STRING=OpenGL2
  -DOPENGL_ES_VERSION:STRING=${OPENGL_ES_VERSION}
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
  -DModule_vtkRenderingFreeType:BOOL=OFF
)

macro(crosscompile target toolchain_file)
  ExternalProject_Add(
    ${target}
    SOURCE_DIR ${CMAKE_SOURCE_DIR}
    PREFIX ${PREFIX_DIR}/${target}
    BINARY_DIR ${BUILD_DIR}/${target}
    INSTALL_DIR ${INSTALL_DIR}/${target}
    DEPENDS vtk-compile-tools
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX:PATH=${INSTALL_DIR}/${target}
      -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
      -DCMAKE_TOOLCHAIN_FILE:PATH=CMake/${toolchain_file}
      -DANDROID_NDK:PATH=${ANDROID_NDK}
      -DVTKCompileTools_DIR:PATH=${BUILD_DIR}/vtk-compile-tools
      ${android_cmake_flags}
  )
endmacro()
crosscompile(vtk-android android.toolchain.cmake)
