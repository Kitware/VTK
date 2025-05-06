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
  UnitTestContinuousScatterPlot.cxx
  UnitTestEntropyFilter.cxx
  UnitTestHistogramFilter.cxx
  UnitTestNDEntropyFilter.cxx
  UnitTestNDHistogramFilter.cxx
  UnitTestPartitionedDataSetHistogramFilter.cxx
  UnitTestStatisticsFilter.cxx
  UnitTestStatisticsFilterMPI.cxx
  )

set(unit_tests_device
  UnitTestParticleDensity.cxx # uses DescriptiveStatistcs worklet
  )

set(libraries
  viskores_filter_density_estimate
  viskores_source)

viskores_unit_tests(
  SOURCES ${unit_tests}
  DEVICE_SOURCES ${unit_tests_device}
  LIBRARIES ${libraries}
  USE_VISKORES_JOB_POOL
)

if (Viskores_ENABLE_MPI)
  set(mpi_unit_tests
    UnitTestStatisticsFilterMPI.cxx
  )
  viskores_unit_tests(
    MPI
    DEVICE_SOURCES ${mpi_unit_tests}
    USE_VISKORES_JOB_POOL
  )
endif()
