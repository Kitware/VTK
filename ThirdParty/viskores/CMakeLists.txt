vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo viskores_repo
    tag  viskores_tag
  VERSION_FROM_TAG
    tag  viskores_version "^v([0-9\\.]+)(-rc[1-9])?$")

vtk_module_third_party(
  INTERNAL
  INTERFACE
    LICENSE_FILES
      "vtkviskores/viskores/LICENSE.txt"
    SPDX_LICENSE_IDENTIFIER
      "BSD-3-Clause"
    SPDX_COPYRIGHT_TEXT
      "Copyright (c) 2024-2025 Kitware Inc., National Technology & Engineering Solutions of Sandia, LLC (NTESS),"
      "UT-Battelle, LLC., Los Alamos National Security, LLC."
    SPDX_DOWNLOAD_LOCATION
      "git+${viskores_repo}@${viskores_tag}"
    VERSION
      "${viskores_version}"
    SUBDIRECTORY vtkviskores
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE Viskores
    TARGETS viskores::cont viskores::cont_testing viskores::filter
    VERSION       "1.1.0"
    STANDARD_INCLUDE_DIRS)

if(VTK_MODULE_USE_EXTERNAL_vtkviskores)
  viskores_setup_job_pool()
endif()

set(viskores_specific_targets
  cont
  cont_testing
  cuda
  filter
  kokkos_cuda
  kokkos_hip
  worklet)
foreach (viskores_target IN LISTS viskores_specific_targets)
  if (VTK_MODULE_USE_EXTERNAL_vtkviskores)
    set(target "viskores::${viskores_target}")
  else ()
    set(target "viskores_${viskores_target}")
  endif ()
  if (NOT TARGET "${target}")
    continue ()
  endif()

  add_library("vtkviskores_${viskores_target}" INTERFACE)
  target_link_libraries("vtkviskores_${viskores_target}" INTERFACE "${target}")
  _vtk_module_install("vtkviskores_${viskores_target}")
  add_library("VTK::vtkviskores_${viskores_target}" ALIAS "vtkviskores_${viskores_target}")
endforeach ()

# If a VTK library contains Viskores worklets or other Viskores device code, the files must be
# declared as the proper language and the library must link to VTK::vtkviskores_worklet. This
# function sets this up. Use this function by giving it the name of a target.
function(vtk_add_viskores_device_target_information target)
  target_link_libraries(${target} PRIVATE VTK::vtkviskores_worklet)
  get_target_property(sources ${target} SOURCES)
  list(FILTER sources INCLUDE REGEX "\\.(cpp|cxx|cc|C)$")
  if(TARGET VTK::vtkviskores_cuda OR TARGET VTK::vtkviskores_kokkos_cuda)
    set_source_files_properties(${sources} PROPERTIES LANGUAGE CUDA)
  endif()
  if(TARGET VTK::vtkviskores_kokkos_hip)
    set_source_files_properties(${sources} PROPERTIES LANGUAGE HIP)
    kokkos_compilation(SOURCE ${sources})
  endif()
endfunction()
