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
  UnitTestFieldSelection.cxx
  UnitTestMapFieldMergeAverage.cxx
  UnitTestMapFieldPermutation.cxx
  UnitTestMultiBlockFilter.cxx
  UnitTestPartitionedDataSetFilters.cxx
)

viskores_unit_tests(
  # For historical reasons, these tests are part of the viskores_filter_core module even though
  # they are in viskores/filter. Thus, we need to rename the test for the filter to recognize it.
  NAME UnitTests_viskores_filter_core_testing
  DEVICE_SOURCES ${unit_tests}
  LIBRARIES ${libraries}
  USE_VISKORES_JOB_POOL
  )
