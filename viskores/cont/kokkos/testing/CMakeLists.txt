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

set(unit_tests
  UnitTestKokkosDeviceAdapter.cxx
  UnitTestKokkosRuntimeDeviceConfiguration.cxx
  )

viskores_unit_tests(SOURCES ${unit_tests} LABEL "KOKKOS" LIBRARIES viskores_worklet BACKEND kokkos)

if (TARGET viskores_kokkos_cuda)
  set_source_files_properties(${unit_tests} PROPERTIES LANGUAGE CUDA)
elseif(TARGET viskores_kokkos_hip)
  set_source_files_properties(${unit_tests} PROPERTIES LANGUAGE HIP)
  kokkos_compilation(SOURCE ${unit_tests})
endif()
