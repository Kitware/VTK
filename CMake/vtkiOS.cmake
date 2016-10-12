include(ExternalProject)

# Convenience variables
set(PREFIX_DIR ${CMAKE_BINARY_DIR}/CMakeExternals/Prefix)
set(BUILD_DIR ${CMAKE_BINARY_DIR}/CMakeExternals/Build)
set(INSTALL_DIR ${CMAKE_BINARY_DIR}/CMakeExternals/Install)

# Remove previous configurations
file(REMOVE_RECURSE ${PREFIX_DIR})
file(REMOVE_RECURSE ${BUILD_DIR})
file(REMOVE_RECURSE ${INSTALL_DIR})

# Options for iOS
set(OPENGL_ES_VERSION "2.0" CACHE STRING "OpenGL ES version (2.0 or 3.0)")
set_property(CACHE OPENGL_ES_VERSION PROPERTY STRINGS 2.0 3.0)

set(IOS_SIMULATOR_ARCHITECTURES "i386;x86_64"
    CACHE STRING "iOS Simulator Architectures")
set(IOS_DEVICE_ARCHITECTURES "arm64;armv7;armv7s"
    CACHE STRING "iOS Device Architectures")

set(IOS_EMBED_BITCODE ON CACHE BOOL "Embed LLVM bitcode")

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
    PREFIX ${CMAKE_BINARY_DIR}/CompileTools
    BINARY_DIR ${CMAKE_BINARY_DIR}/CompileTools
    INSTALL_COMMAND ""
    ${VTK_BUILD_COMMAND} vtkCompileTools
    ${BUILD_ALWAYS_STRING}
    CMAKE_CACHE_ARGS
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
  BUILD_SHARED_LIBS
  CMAKE_INSTALL_PREFIX
  CMAKE_OSX_ARCHITECTURES
  CMAKE_OSX_DEPLOYMENT_TARGET
  CMAKE_OSX_ROOT
  VTK_RENDERING_BACKEND
)


# expose some module options
option(Module_vtkRenderingOpenGL2 "Include Polygonal Rendering Support" ON)
option(Module_vtkInteractionWidgets "Include InteractionWidgets module" OFF)
option(Module_vtkIOXML "Include IO/XML Module" OFF)
option(Module_vtkFiltersModeling "Turn on or off this module" OFF)
option(Module_vtkFiltersSources "Turn on or off this module" OFF)
option(Module_vtkIOGeometry "Turn on or off this module" OFF)
option(Module_vtkIOLegacy "Turn on or off this module" OFF)
option(Module_vtkIOImage "Turn on or off this module" OFF)
option(Module_vtkIOPLY "Turn on or off this module" OFF)
option(Module_vtkIOInfovis "Turn on or off this module" OFF)
option(Module_vtkRenderingFreeType "Turn on or off this module" OFF)


# add volume rendering option for ES 3.0
if (OPENGL_ES_VERSION STREQUAL "3.0" AND Module_vtkRenderingOpenGL2)
  option(Module_vtkRenderingVolumeOpenGL2 "Include Volume Rendering Support" ON)
endif()

mark_as_advanced(Module_${vtk-module})

# Now cross-compile VTK with custom toolchains
set(ios_cmake_flags
  -DBUILD_SHARED_LIBS:BOOL=OFF
  -DBUILD_TESTING:BOOL=OFF
  -DBUILD_EXAMPLES:BOOL=${BUILD_EXAMPLES}
  -DOPENGL_ES_VERSION:STRING=${OPENGL_ES_VERSION}
  -DVTK_Group_Rendering:BOOL=OFF
  -DVTK_Group_StandAlone:BOOL=OFF
  -DVTK_Group_Imaging:BOOL=OFF
  -DVTK_Group_MPI:BOOL=OFF
  -DVTK_Group_Views:BOOL=OFF
  -DVTK_Group_Qt:BOOL=OFF
  -DVTK_Group_Tk:BOOL=OFF
  -DVTK_Group_Web:BOOL=OFF
  -DModule_vtkRenderingOpenGL2:BOOL=${Module_vtkRenderingOpenGL2}
  -DModule_vtkInteractionWidgets:BOOL=${Module_vtkInteractionWidgets}
  -DModule_vtkFiltersModeling:BOOL=${Module_vtkFiltersModeling}
  -DModule_vtkFiltersSources:BOOL=${Module_vtkFiltersSources}
  -DModule_vtkIOGeometry:BOOL=${Module_vtkIOGeometry}
  -DModule_vtkIOLegacy:BOOL=${Module_vtkIOLegacy}
  -DModule_vtkIOImage:BOOL=${Module_vtkIOImage}
  -DModule_vtkIOPLY:BOOL=${Module_vtkIOPLY}
  -DModule_vtkIOInfovis:BOOL=${Module_vtkIOInfovis}
  -DModule_vtkRenderingFreeType:BOOL=${Module_vtkRenderingFreeType}
)

if (Module_vtkRenderingOpenGL2)
  set (ios_cmake_flags ${ios_cmake_flags}
    -DVTK_RENDERING_BACKEND:STRING=OpenGL2
    )
else()
  set (ios_cmake_flags ${ios_cmake_flags}
    -DVTK_RENDERING_BACKEND:STRING=None
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
      -DVTKCompileTools_DIR:PATH=${CMAKE_BINARY_DIR}/CompileTools
      -DCMAKE_INSTALL_PREFIX:PATH=${INSTALL_DIR}/${target}
      ${ios_cmake_flags}
  )
  #
  # add an INSTALL_ALLWAYS since we want it and cmake lacks it
  #
  ExternalProject_Get_Property(${target} binary_dir)
  _ep_get_build_command(${target} INSTALL cmd)
  ExternalProject_Add_Step(${target} always-install
    COMMAND ${cmd}
    WORKING_DIRECTORY ${binary_dir}
    DEPENDEES build install
    ALWAYS 1
    )
endmacro()

crosscompile(vtk-ios-simulator
  CMake/ios.simulator.toolchain.cmake
  "${IOS_SIMULATOR_ARCHITECTURES}"
 )

# for each architecture
foreach (arch ${IOS_DEVICE_ARCHITECTURES})
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
