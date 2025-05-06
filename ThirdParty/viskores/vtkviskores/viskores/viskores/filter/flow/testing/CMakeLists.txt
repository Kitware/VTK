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

set(filter_unit_tests
  UnitTestLagrangianFilter.cxx
  UnitTestLagrangianStructuresFilter.cxx
  UnitTestStreamlineFilter.cxx
  UnitTestStreamlineFilterWarpX.cxx
  UnitTestStreamSurfaceFilter.cxx
  )
set(worklet_unit_tests
  UnitTestWorkletParticleAdvection.cxx
  UnitTestWorkletTemporalAdvection.cxx
  UnitTestStreamSurfaceWorklet.cxx
  )

if (TARGET viskores_rendering_testing)
  list(APPEND filter_unit_tests
    RenderTestStreamline.cxx
    )
endif()

viskores_unit_tests(
  SOURCES ${filter_unit_tests}
  DEVICE_SOURCES ${worklet_unit_tests}
  USE_VISKORES_JOB_POOL
  )

#add distributed tests i.e.test to run with MPI
#if MPI is enabled.
if (Viskores_ENABLE_MPI)
  set(mpi_unit_tests
    UnitTestAdvectionMPI.cxx
    UnitTestPathlineMPI.cxx
    UnitTestStreamlineAMRMPI.cxx
    UnitTestStreamlineMPI.cxx
    )
  set(mpi_helpers
    TestingFlow.cxx
    TestingFlow.h
    )
  set_source_files_properties(${mpi_helpers} PROPERTIES NOT_A_TEST TRUE)
  viskores_unit_tests(
    MPI
    SOURCES ${mpi_unit_tests} ${mpi_helpers}
    USE_VISKORES_JOB_POOL
  )
  set_tests_properties(UnitTestAdvectionMPI_mpi PROPERTIES TIMEOUT 500)
endif()
