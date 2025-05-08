##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

if (NOT (DEFINED Viskores_BUILD_CMAKE_BASE_DIR AND
         DEFINED Viskores_INSTALL_CONFIG_DIR AND
         DEFINED Viskores_CMAKE_MODULE_PATH))
  message(FATAL_ERROR
    "ViskoresInstallCMakePackage is missing input variables")
endif()

set(viskores_cmake_module_files)

if(Viskores_ENABLE_TBB)
# XXX(kitware): removed by VTK:   list(APPEND viskores_cmake_module_files FindTBB.cmake)
endif()

set(viskores_cmake_build_dir ${Viskores_BUILD_CMAKE_BASE_DIR}/${Viskores_INSTALL_CONFIG_DIR})
foreach (viskores_cmake_module_file IN LISTS viskores_cmake_module_files)
  configure_file(
    "${Viskores_CMAKE_MODULE_PATH}/${viskores_cmake_module_file}"
    "${viskores_cmake_build_dir}/${viskores_cmake_module_file}"
    COPYONLY)
  list(APPEND viskores_cmake_files_to_install
    "${viskores_cmake_module_file}")
endforeach()

foreach (viskores_cmake_file IN LISTS viskores_cmake_files_to_install)
  if (IS_ABSOLUTE "${viskores_cmake_file}")
    file(RELATIVE_PATH viskores_cmake_subdir_root "${viskores_cmake_build_dir}" "${viskores_cmake_file}")
    get_filename_component(viskores_cmake_subdir "${viskores_cmake_subdir_root}" DIRECTORY)
    set(viskores_cmake_original_file "${viskores_cmake_file}")
  else ()
    get_filename_component(viskores_cmake_subdir "${viskores_cmake_file}" DIRECTORY)
    set(viskores_cmake_original_file "${Viskores_CMAKE_MODULE_PATH}/${viskores_cmake_file}")
  endif ()
  install(
    FILES       "${viskores_cmake_original_file}"
    DESTINATION "${Viskores_INSTALL_CONFIG_DIR}/${viskores_cmake_subdir}"
    COMPONENT   "development")
endforeach ()
