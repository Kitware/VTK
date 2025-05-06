##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
viskores_get_kit_name(kit_name kit_dir)

include(CMakeDependentOption)
if (NOT DEFINED Viskores_ENABLE_DIY_NOMPI)
  cmake_dependent_option(
    Viskores_ENABLE_DIY_NOMPI "Also build DIY without mpi" OFF "Viskores_ENABLE_MPI" OFF)
endif()

if (Viskores_ENABLE_DIY_NOMPI AND
    (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC") AND
    BUILD_SHARED_LIBS)
  message(WARNING "Viskores_ENABLE_DIY_NOMPI support with MSVC shared builds is experimental and may not work.")
endif()

## Configure diy
set(build_examples OFF)
set(build_tests OFF)

set(threads OFF)
set(log OFF)
set(profile OFF)
set(caliper OFF)

set(build_diy_mpi_lib ON)

set(mpi OFF)
set(build_diy_nompi_lib OFF)
if (Viskores_ENABLE_MPI)
  set(mpi ON)
endif()
if (Viskores_ENABLE_DIY_NOMPI)
  set(build_diy_nompi_lib ON)
endif()

mark_as_advanced(FORCE caliper log profile wrapped_mpi)

set(diy_prefix "viskoresdiy")
set(diy_install_include_dir ${Viskores_INSTALL_INCLUDE_DIR}/${kit_dir}/viskoresdiy/include)
set(diy_install_bin_dir ${Viskores_INSTALL_BIN_DIR})
set(diy_install_lib_dir ${Viskores_INSTALL_LIB_DIR})
set(diy_export_name ${Viskores_EXPORT_NAME})
if (Viskores_INSTALL_ONLY_LIBRARIES)
  set(diy_install_only_libraries)
endif()
set(diy_dont_install_export)

# placeholder to support external DIY
set(VISKORES_USE_EXTERNAL_DIY OFF)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Configure.h.in
  ${Viskores_BINARY_INCLUDE_DIR}/${kit_dir}/Configure.h)

function(viskores_diy_set_target_properties target)
  if (NOT Viskores_SKIP_LIBRARY_VERSIONS)
    set_property(TARGET ${target} PROPERTY VERSION ${Viskores_VERSION}.${Viskores_VERSION_PATCH})
    set_property(TARGET ${target} PROPERTY SOVERSION ${Viskores_VERSION})
  endif()
  set_property(TARGET ${target} PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${Viskores_LIBRARY_OUTPUT_PATH})
  set_property(TARGET ${target} PROPERTY LIBRARY_OUTPUT_DIRECTORY ${Viskores_LIBRARY_OUTPUT_PATH})
  set_property(TARGET ${target} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${Viskores_EXECUTABLE_OUTPUT_PATH})
endfunction()

#-----------------------------------------------------------------------------
add_subdirectory(viskoresdiy)

# move diy libraries
if (TARGET viskoresdiympi)
  viskores_diy_set_target_properties(viskoresdiympi)
endif()
if (TARGET viskoresdiympi_nompi)
  viskores_diy_set_target_properties(viskoresdiympi_nompi)
endif()

include(ViskoresDIYUtils)

add_library(viskores_diy INTERFACE)

viskores_diy_init_target()
target_include_directories(viskores_diy INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${Viskores_INSTALL_INCLUDE_DIR}/viskores/thirdparty/diy>)
target_link_libraries(viskores_diy INTERFACE viskoresdiy)

# special logic for when both versions of the diy library are built
if (Viskores_ENABLE_DIY_NOMPI)
  # only link viskoresdiympi/viskoresdiympi_nompi when building executable
  set(is_exe "$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>")
  target_link_libraries(viskores_diy INTERFACE
    "$<LINK_ONLY:$<${is_exe}:$<TARGET_PROPERTY:viskores_diy,viskores_diy_target>>>")

  # ignore undefined symbols
  set(is_shared_lib "$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>")
  if (APPLE)
    target_link_libraries(viskores_diy INTERFACE "$<${is_shared_lib}:-undefined dynamic_lookup>")
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    target_link_libraries(viskores_diy INTERFACE
      "$<${is_shared_lib}:-INCREMENTAL:NO>"
      "$<${is_shared_lib}:-FORCE:UNRESOLVED>")
  endif()
else()
  if (TARGET viskoresdiympi)
    target_link_libraries(viskores_diy INTERFACE viskoresdiympi)
  else()
    target_link_libraries(viskores_diy INTERFACE viskoresdiympi_nompi)
  endif()
endif()

#-----------------------------------------------------------------------------
viskores_install_targets(TARGETS viskores_diy)

## Install headers
if (NOT Viskores_INSTALL_ONLY_LIBRARIES)
  install(FILES
    ${Viskores_BINARY_INCLUDE_DIR}/${kit_dir}/Configure.h
    ${CMAKE_CURRENT_SOURCE_DIR}/diy.h
    ${CMAKE_CURRENT_SOURCE_DIR}/environment.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mpi-cast.h
    ${CMAKE_CURRENT_SOURCE_DIR}/post-include.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pre-include.h
    ${CMAKE_CURRENT_SOURCE_DIR}/serialization.h
    DESTINATION ${Viskores_INSTALL_INCLUDE_DIR}/${kit_dir}/)
endif()
