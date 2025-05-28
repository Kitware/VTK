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
      "git+git@github.com:Viskores/viskores.git@v1.0.0"
    VERSION
      "1.0.0"
    SUBDIRECTORY vtkviskores
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE Viskores
    TARGETS viskores::cont viskores::cont_testing viskores::filter viskores::worklet
    VERSION       "1.0.0"
    STANDARD_INCLUDE_DIRS)

if(VTK_MODULE_USE_EXTERNAL_vtkviskores)
  viskores_setup_job_pool()
endif()
