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
  UnitTestOscillatorSource.cxx
  UnitTestTangleSource.cxx
  UnitTestWaveletSource.cxx
 )

set(libraries
  viskores_source
  )

if (Viskores_ENABLE_RENDERING)
  list(APPEND libraries viskores_rendering_testing viskores_filter_contour)

  list(APPEND unit_tests
    RenderTestPerlinNoise.cxx
    )
endif()

viskores_unit_tests(
  SOURCES ${unit_tests}
  LIBRARIES ${libraries}
)
