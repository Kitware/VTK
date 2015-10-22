include(ExternalProject)

# Convenience variables
set(PREFIX_DIR ${CMAKE_BINARY_DIR}/CMakeExternals/Prefix)
set(BUILD_DIR ${CMAKE_BINARY_DIR}/CMakeExternals/Build)
set(INSTALL_DIR ${CMAKE_BINARY_DIR}/CMakeExternals/Install)

# Options for iOS
set(OPENGL_ES_VERSION "2.0" CACHE STRING "OpenGL ES version (2.0 or 3.0)")
set_property(CACHE OPENGL_ES_VERSION PROPERTY STRINGS 2.0 3.0)

set(IOS_SIMULATOR_ARCHITECTURES "i386;x86_64"
    CACHE STRING "iOS Simulator Architectures")
set(IOS_DEVICE_ARCHITECTURES "arm64;armv7;armv7s"
    CACHE STRING "iOS Device Architectures")

set(CMAKE_FRAMEWORK_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/frameworks"
    CACHE PATH "Framework install path")

# Fail if the install path is invalid
if (NOT EXISTS ${CMAKE_INSTALL_PREFIX})
  message(FATAL_ERROR
    "Install path ${CMAKE_INSTALL_PREFIX} does not exist.")
endif()

# Try to make the framework install directory if it doesn't exist
if (NOT EXISTS ${CMAKE_FRAMEWORK_INSTALL_PREFIX})
  file(MAKE_DIRECTORY ${CMAKE_FRAMEWORK_INSTALL_PREFIX})
  if (NOT EXISTS ${CMAKE_FRAMEWORK_INSTALL_PREFIX})
    message(FATAL_ERROR
      "Framework install path ${CMAKE_FRAMEWORK_INSTALL_PREFIX} does not exist.")
  endif()
endif()

# First, determine how to build
if (CMAKE_GENERATOR MATCHES "NMake Makefiles")
  set(VTK_BUILD_COMMAND BUILD_COMMAND nmake)
elseif (CMAKE_GENERATOR MATCHES "Ninja")
  set(VTK_BUILD_COMMAND BUILD_COMMAND ninja)
else()
  set(VTK_BUILD_COMMAND BUILD_COMMAND make)
endif()

set(BUILD_ALWAYS_STRING)
if(${CMAKE_VERSION} GREATER 3.0)
  set(BUILD_ALWAYS_STRING BUILD_ALWAYS 1)
endif()

# Compile a minimal VTK for its compile tools
macro(compile_vtk_tools)
  ExternalProject_Add(
    vtk-compile-tools
    SOURCE_DIR ${CMAKE_SOURCE_DIR}
    PREFIX ${PREFIX_DIR}/vtk-compile-tools
    BINARY_DIR ${BUILD_DIR}/vtk-compile-tools
    ${VTK_BUILD_COMMAND} vtkCompileTools
    ${BUILD_ALWAYS_STRING}
    INSTALL_DIR ${INSTALL_DIR}/vtk-compile-tools
    CMAKE_CACHE_ARGS
      -DCMAKE_BUILD_TYPE:STRING=Release
      -DVTK_BUILD_ALL_MODULES:BOOL=OFF
      -DVTK_Group_Rendering:BOOL=OFF
      -DVTK_Group_StandAlone:BOOL=ON
      -DBUILD_SHARED_LIBS:BOOL=ON
      -DBUILD_EXAMPLES:BOOL=OFF
      -DBUILD_TESTING:BOOL=OFF
      -DCMAKE_INSTALL_PREFIX:PATH=${INSTALL_DIR}/vtk-compile-tools
  )
endmacro()
compile_vtk_tools()


# Hide some CMake configs from the user
mark_as_advanced(
  BUILD_SHARED_LIBS
  CMAKE_INSTALL_PREFIX
  CMAKE_OSX_ARCHITECTURES
  CMAKE_OSX_DEPLOYMENT_TARGET
  CMAKE_OSX_ROOT
  VTK_RENDERING_BACKEND
)


# Now cross-compile VTK with custom toolchains
set(ios_cmake_flags
  -DBUILD_SHARED_LIBS:BOOL=OFF
  -DBUILD_TESTING:BOOL=OFF
  -DBUILD_EXAMPLES:BOOL=${BUILD_EXAMPLES}
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
  -DModule_vtkIOImage:BOOL=ON
  -DModule_vtkIOPLY:BOOL=ON
  -DModule_vtkIOInfovis:BOOL=ON
  -DModule_vtkImagingCore:BOOL=ON
  -DModule_vtkInteractionStyle:BOOL=ON
  -DModule_vtkParallelCore:BOOL=ON
  -DModule_vtkRenderingCore:BOOL=ON
  -DModule_vtkRenderingFreeType:BOOL=OFF
)

# add volume rendering for ES 3.0
if (OPENGL_ES_VERSION STREQUAL "3.0")
  set(ios_cmake_flags ${ios_cmake_flags}
    -DModule_vtkRenderingVolumeOpenGL2:BOOL=ON
    )
endif()

macro(crosscompile target toolchain_file archs)
  ExternalProject_Add(
    ${target}
    SOURCE_DIR ${CMAKE_SOURCE_DIR}
    PREFIX ${PREFIX_DIR}/${target}
    BINARY_DIR ${BUILD_DIR}/${target}
    INSTALL_DIR ${INSTALL_DIR}/${target}
    DEPENDS vtk-compile-tools
    ${BUILD_ALWAYS_STRING}
    CMAKE_ARGS
      -DCMAKE_CROSSCOMPILING:BOOL=ON
      -DCMAKE_OSX_ARCHITECTURES:STRING=${archs}
      -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
      -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${toolchain_file}
      -DVTKCompileTools_DIR:PATH=${BUILD_DIR}/vtk-compile-tools
      -DCMAKE_INSTALL_PREFIX:PATH=${INSTALL_DIR}/${target}
      ${ios_cmake_flags}
  )
endmacro()

crosscompile(vtk-ios-simulator
  CMake/ios.simulator.toolchain.cmake
  "${IOS_SIMULATOR_ARCHITECTURES}"
 )

# for each architecture
set(IOS_ARCHITECTURES arm64 armv7 armv7s)
foreach (arch ${IOS_ARCHITECTURES})
  set(CMAKE_CC_ARCH ${arch})
  configure_file(CMake/ios.device.toolchain.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/CMake/ios.device.toolchain.${arch}.cmake
               @ONLY)
  crosscompile(vtk-ios-device-${arch}
    ${CMAKE_CURRENT_BINARY_DIR}/CMake/ios.device.toolchain.${arch}.cmake
    ${arch}
  )
  set(VTK_DEVICE_LIBS "${VTK_DEVICE_LIBS}
    \"${INSTALL_DIR}/vtk-ios-device-${arch}/lib/libvtk*.a\"" )
  set(VTK_DEVICE_DEPENDS ${VTK_DEVICE_DEPENDS}
    vtk-ios-device-${arch} )
endforeach()

# Pile it all into a framework
set(VTK_SIMULATOR_LIBS
    "${INSTALL_DIR}/vtk-ios-simulator/lib/libvtk*.a" )
set(VTK_INSTALLED_HEADERS
    "${INSTALL_DIR}/vtk-ios-device-arm64/include/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}")
set(VTK_GLOB_LIBS "${VTK_DEVICE_LIBS} \"${VTK_SIMULATOR_LIBS}\"")
configure_file(CMake/MakeFramework.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/CMake/MakeFramework.cmake
               @ONLY)
add_custom_target(vtk-framework ALL
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/CMake/MakeFramework.cmake
  DEPENDS ${VTK_DEVICE_DEPENDS} vtk-ios-simulator)
