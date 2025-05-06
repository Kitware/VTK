##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
add_library(viskores_lcl INTERFACE)

viskores_get_kit_name(kit_name kit_dir)

# lcl needs C++11
target_compile_features(viskores_lcl INTERFACE cxx_std_11)

target_include_directories(viskores_lcl INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/viskoreslcl>
  $<INSTALL_INTERFACE:${Viskores_INSTALL_INCLUDE_DIR}/viskores/thirdparty/lcl/viskoreslcl>)

viskores_install_targets(TARGETS viskores_lcl)

## Install headers
if(NOT Viskores_INSTALL_ONLY_LIBRARIES)
  install(DIRECTORY viskoreslcl
    DESTINATION ${Viskores_INSTALL_INCLUDE_DIR}/${kit_dir}/)
endif()
