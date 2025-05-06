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

macro(viskores_diy_get_general_target target)
  if(PROJECT_NAME STREQUAL "Viskores" OR CMAKE_PROJECT_NAME STREQUAL "Viskores")
    set(${target} "viskores_diy")
  else()
    set(${target} "viskores::diy")
  endif()
endmacro()

macro(_viskores_diy_target flag target)
  set(${target} "viskoresdiympi")
  if (NOT ${flag})
    set(${target} "viskoresdiympi_nompi")
  endif()
endmacro()

function(viskores_diy_init_target)
  set(viskores_diy_default_flag "${Viskores_ENABLE_MPI}")
  _viskores_diy_target(viskores_diy_default_flag viskores_diy_default_target)

  viskores_diy_get_general_target(diy_target)
  set_target_properties(${diy_target} PROPERTIES
    viskores_diy_use_mpi_stack ${viskores_diy_default_flag}
    viskores_diy_target ${viskores_diy_default_target})
endfunction()

#-----------------------------------------------------------------------------
function(viskores_diy_use_mpi_push)
  set(topval ${Viskores_ENABLE_MPI})
  if (NOT ARGC EQUAL 0)
    set(topval ${ARGV0})
  endif()
  viskores_diy_get_general_target(diy_target)
  get_target_property(stack ${diy_target} viskores_diy_use_mpi_stack)
  list (APPEND stack ${topval})
  _viskores_diy_target(topval target)
  set_target_properties(${diy_target} PROPERTIES
    viskores_diy_use_mpi_stack "${stack}"
    viskores_diy_target "${target}")
endfunction()

function(viskores_diy_use_mpi value)
  viskores_diy_get_general_target(diy_target)
  get_target_property(stack ${diy_target} viskores_diy_use_mpi_stack)
  list (REMOVE_AT stack -1)
  list (APPEND stack ${value})
  _viskores_diy_target(value target)
  set_target_properties(${diy_target} PROPERTIES
    viskores_diy_use_mpi_stack "${stack}"
    viskores_diy_target "${target}")
endfunction()

function(viskores_diy_use_mpi_pop)
  viskores_diy_get_general_target(diy_target)
  get_target_property(stack ${diy_target} viskores_diy_use_mpi_stack)
  list (GET stack -1 value)
  list (REMOVE_AT stack -1)
  _viskores_diy_target(value target)
  set_target_properties(${diy_target} PROPERTIES
    viskores_diy_use_mpi_stack "${stack}"
    viskores_diy_target "${target}")
endfunction()
