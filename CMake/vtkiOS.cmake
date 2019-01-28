cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

include(ExternalProject)

# Convenience variables
set(PREFIX_DIR ${CMAKE_BINARY_DIR}/CMakeExternals/Prefix)
set(BUILD_DIR ${CMAKE_BINARY_DIR}/CMakeExternals/Build)
set(INSTALL_DIR ${CMAKE_BINARY_DIR}/CMakeExternals/Install)

# Remove previous configurations
file(REMOVE_RECURSE ${PREFIX_DIR})
file(REMOVE_RECURSE ${BUILD_DIR})
file(REMOVE_RECURSE ${INSTALL_DIR})

# Define default architectures to compile for
set(IOS_SIMULATOR_ARCHITECTURES "x86_64"
    CACHE STRING "iOS Simulator Architectures")
set(IOS_DEVICE_ARCHITECTURES "arm64"
    CACHE STRING "iOS Device Architectures")
list(REMOVE_DUPLICATES IOS_SIMULATOR_ARCHITECTURES)
list(REMOVE_DUPLICATES IOS_DEVICE_ARCHITECTURES)

# Check that at least one architure is defined
list(LENGTH IOS_SIMULATOR_ARCHITECTURES SIMULATOR_ARCHS_NBR)
list(LENGTH IOS_DEVICE_ARCHITECTURES DEVICE_ARCHS_NBR)
math(EXPR IOS_ARCHS_NBR ${DEVICE_ARCHS_NBR}+${SIMULATOR_ARCHS_NBR})
if(NOT ${IOS_ARCHS_NBR})
  message(FATAL_ERROR "No IOS simulator or device architecture to compile for. Populate IOS_DEVICE_ARCHITECTURES and/or IOS_SIMULATOR_ARCHITECTURES.")
endif()

# iOS Deployment Target
execute_process(COMMAND /usr/bin/xcrun -sdk iphoneos --show-sdk-version
                OUTPUT_VARIABLE IOS_DEPLOYMENT_TARGET_TMP
                OUTPUT_STRIP_TRAILING_WHITESPACE)
set(IOS_DEPLOYMENT_TARGET ${IOS_DEPLOYMENT_TARGET_TMP} CACHE STRING "iOS Deployment Target")

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
  )
endmacro()
compile_vtk_tools()


# Hide some CMake configs from the user
mark_as_advanced(FORCE
  BUILD_SHARED_LIBS
  BUILD_TESTING
  CMAKE_INSTALL_PREFIX
  CMAKE_OSX_ARCHITECTURES
  CMAKE_OSX_DEPLOYMENT_TARGET
  CMAKE_OSX_ROOT
)
if(BUILD_SHARED_LIBS)
  message(WARNING "Can not build shared libraries for iOS framework. BUILD_SHARED_LIBS will be ignored.")
endif()
if(BUILD_TESTING)
  message(WARNING "Tests not supported for the iOS framework. BUILD_TESTING will be ignored.")
endif()

# expose some module options
set(module_options
  #DICOM
  FiltersModeling
  FiltersSources
  IOGeometry
  IOImage
  IOInfovis
  IOLegacy
  IOPLY
  IOXML
  InteractionStyle
  InteractionWidgets
  RenderingFreeType
  RenderingImage
  RenderingLOD
  RenderingOpenGL2
  RenderingVolumeOpenGL2)

set(IOXML_desc "Include IO/XML Module")
set(InteractionStyle_desc "Include InteractionStyle module")
set(InteractionWidgets_desc "Include InteractionWidgets module")
set(RenderingLOD_desc "Include LOD Rendering Support")
set(RenderingOpenGL2_desc "Include Polygonal Rendering Support")
set(RenderingVolumeOpenGL2_desc "Include Volume Rendering Support")

set(InteractionStyle_default ON)
set(RenderingOpenGL2_default ON)
set(RenderingVolumeOpenGL2_default ON)

foreach (module IN LISTS module_options)
  set(desc "Turn on or off this module")
  if (DEFINED "${module}_desc")
    set(desc "${${module}_desc}")
  endif ()
  set(default OFF)
  if (DEFINED "${module}_default")
    set(default "${${module}_default}")
  endif ()
  option(VTK_MODULE_ENABLE_VTK_${module} "${desc}" ${default})
  set("enable_option_${module}" "DONT_WANT")
  if (VTK_MODULE_ENABLE_VTK_${module})
    set("enable_option_${module}" "YES")
  endif ()
endforeach ()

# XXX(FIXME): DICOM has not been added back into the main build yet.
if (FALSE AND Module_vtkDICOM)
  set(DICOM_OPTION -DModule_vtkDICOM:BOOL=ON)
endif()

# Now cross-compile VTK with custom toolchains
set(ios_cmake_flags
  -DBUILD_SHARED_LIBS:BOOL=OFF
  -DVTK_BUILD_TESTING:BOOL=OFF
  -DVTK_BUILD_EXAMPLES:BOOL=${VTK_BUILD_EXAMPLES}
  -DVTK_USE_64BIT_IDS:BOOL=OFF
  -DVTK_GROUP_ENABLE_Rendering:STRING=DONT_WANT
  -DVTK_GROUP_ENABLE_StandAlone:STRING=DONT_WANT
  -DVTK_GROUP_ENABLE_Imaging:STRING=DONT_WANT
  -DVTK_GROUP_ENABLE_MPI:STRING=DONT_WANT
  -DVTK_GROUP_ENABLE_Views:STRING=DONT_WANT
  -DVTK_GROUP_ENABLE_Qt:STRING=DONT_WANT
  -DVTK_GROUP_ENABLE_Web:STRING=DONT_WANT
  -DVTK_MODULE_ENABLE_VTK_RenderingOpenGL2:STRING=${enable_option_RenderingOpenGL2}
  -DVTK_MODULE_ENABLE_VTK_InteractionStyle:STRING=${enable_option_InteractionStyle}
  -DVTK_MODULE_ENABLE_VTK_InteractionWidgets:STRING=${enable_option_InteractionWidgets}
  -DVTK_MODULE_ENABLE_VTK_IOXML:STRING=${enable_option_IOXML}
  -DVTK_MODULE_ENABLE_VTK_FiltersModeling:STRING=${enable_option_FiltersModeling}
  -DVTK_MODULE_ENABLE_VTK_FiltersSources:STRING=${enable_option_FiltersSources}
  -DVTK_MODULE_ENABLE_VTK_IOGeometry:STRING=${enable_option_IOGeometry}
  -DVTK_MODULE_ENABLE_VTK_IOLegacy:STRING=${enable_option_IOLegacy}
  -DVTK_MODULE_ENABLE_VTK_IOImage:STRING=${enable_option_IOImage}
  -DVTK_MODULE_ENABLE_VTK_IOPLY:STRING=${enable_option_IOPLY}
  -DVTK_MODULE_ENABLE_VTK_IOInfovis:STRING=${enable_option_IOInfovis}
  -DVTK_MODULE_ENABLE_VTK_RenderingFreeType:STRING=${enable_option_RenderingFreeType}
  -DVTK_MODULE_ENABLE_VTK_RenderingImage:STRING=${enable_option_RenderingImage}
  -DVTK_MODULE_ENABLE_VTK_RenderingVolumeOpenGL2:STRING=${enable_option_RenderingVolumeOpenGL2}
  -DVTK_MODULE_ENABLE_VTK_RenderingLOD:STRING=${enable_option_RenderingLOD}
  ${DICOM_OPTION}
)

if (Module_vtkDICOM AND IOS_EMBED_BITCODE)
  # libvtkzlib does not contain bitcode
  list (APPEND ios_cmake_flags
    -DBUILD_DICOM_PROGRAMS:BOOL=OFF
    )
endif()

macro(crosscompile target toolchain_file)
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
      -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
      -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${toolchain_file}
      -DVTKCompileTools_DIR:PATH=${CMAKE_BINARY_DIR}/CompileTools
      -DCMAKE_INSTALL_PREFIX:PATH=${INSTALL_DIR}/${target}
      -DCMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM}
      ${ios_cmake_flags}
  )
  #
  # add an INSTALL_ALWAYS since we want it and cmake lacks it
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

# for simulator architectures
if (${SIMULATOR_ARCHS_NBR})
  configure_file(CMake/ios.simulator.toolchain.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/CMake/ios.simulator.toolchain.cmake
    @ONLY
  )
  crosscompile(vtk-ios-simulator
    ${CMAKE_CURRENT_BINARY_DIR}/CMake/ios.simulator.toolchain.cmake
  )
  set(VTK_GLOB_LIBS "${VTK_GLOB_LIBS} \"${INSTALL_DIR}/vtk-ios-simulator/lib/libvtk*.a\"" )
  list(APPEND IOS_ARCHITECTURES vtk-ios-simulator )
endif()

# for each device architecture
foreach (arch ${IOS_DEVICE_ARCHITECTURES})
  set(CMAKE_CC_ARCH ${arch})
  configure_file(CMake/ios.device.toolchain.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/CMake/ios.device.toolchain.${arch}.cmake
    @ONLY
  )
  crosscompile(vtk-ios-device-${arch}
    ${CMAKE_CURRENT_BINARY_DIR}/CMake/ios.device.toolchain.${arch}.cmake
  )
  set(VTK_GLOB_LIBS "${VTK_GLOB_LIBS} \"${INSTALL_DIR}/vtk-ios-device-${arch}/lib/libvtk*.a\"" )
  list(APPEND IOS_ARCHITECTURES vtk-ios-device-${arch} )
endforeach()

# Pile it all into a framework
list(GET IOS_ARCHITECTURES 0 IOS_ARCH_FIRST)
set(VTK_INSTALLED_HEADERS
    "${INSTALL_DIR}/${IOS_ARCH_FIRST}/include/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}")
configure_file(CMake/MakeFramework.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/CMake/MakeFramework.cmake
               @ONLY)
add_custom_target(vtk-framework ALL
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/CMake/MakeFramework.cmake
  DEPENDS ${IOS_ARCHITECTURES})
