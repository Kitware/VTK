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
  RenderTestComputeMoments.cxx
  UnitTestImageDifferenceFilter.cxx
  UnitTestImageMedianFilter.cxx
  )

set(libraries
  viskores_filter_image_processing
  viskores_source
  viskores_rendering
  viskores_rendering_testing
)

viskores_unit_tests(
  SOURCES ${unit_tests}
  LIBRARIES ${libraries}
  USE_VISKORES_JOB_POOL
)
