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
  UnitTestOpenMPDeviceAdapter.cxx
  UnitTestOpenMPRuntimeDeviceConfiguration.cxx
  )

viskores_unit_tests(SOURCES ${unit_tests}
                LABEL "OPENMP"
                DEFINES VISKORES_NO_ERROR_ON_MIXED_CUDA_CXX_TAG
                LIBRARIES viskores_worklet
                BACKEND openmp
                )

#We need to have all OpenMP tests run serially as they
#will uses all the system cores, and we will cause a N*N thread
#explosion which causes the tests to run slower than when run
#serially
foreach (test ${unit_tests})
  if (NOT TEST "${test}")
    continue ()
  endif ()
  get_filename_component(tname ${test} NAME_WE)
  set_tests_properties(${tname} PROPERTIES
      RUN_SERIAL True
  )
endforeach()
