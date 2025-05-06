##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##=============================================================================

set(headers
  SuperArcHelper.h
  TestingContourTreeUniformDistributedFilter.h
  VolumeHelper.h
  )

#viskores_declare_headers(${headers})

set(unit_tests
  UnitTestContourTreeUniformFilter.cxx
  )

set(unit_tests_device
  UnitTestContourTreeUniformAugmentedFilter.cxx
  UnitTestContourTreeUniformAugmentedWorklet.cxx
  UnitTestContourTreeUniformDistributedFilter.cxx
  UnitTestDistributedBranchDecompositionFilter.cxx
  )

viskores_unit_tests(
  SOURCES ${unit_tests}
  DEVICE_SOURCES ${unit_tests_device}
  USE_VISKORES_JOB_POOL
)

if (Viskores_ENABLE_MPI)
  set(mpi_unit_tests
    )
  set(mpi_unit_tests_device
    UnitTestContourTreeUniformDistributedFilterMPI.cxx
    )
  viskores_unit_tests(
    MPI
    SOURCES ${mpi_unit_tests}
    DEVICE_SOURCES ${mpi_unit_tests_device}
    LIBRARIES viskores_filter viskores_source viskores_io
    USE_VISKORES_JOB_POOL
  )
endif()
