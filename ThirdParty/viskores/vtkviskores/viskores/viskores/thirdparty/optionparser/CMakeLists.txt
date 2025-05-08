##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
add_library(viskores_optionparser INTERFACE)

viskores_get_kit_name(kit_name kit_dir)

target_include_directories(viskores_optionparser INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${Viskores_INSTALL_INCLUDE_DIR}/viskores/thirdparty/optionparser>)

viskores_install_targets(TARGETS viskores_optionparser)

## Install headers
if(NOT Viskores_INSTALL_ONLY_LIBRARIES)
  install(DIRECTORY viskoresoptionparser
    DESTINATION ${Viskores_INSTALL_INCLUDE_DIR}/${kit_dir}/)
endif()
