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
  UnitTestBOVDataSetReader.cxx
  UnitTestFileUtils.cxx
  UnitTestPixelTypes.cxx
  UnitTestVisItFileDataSetReader.cxx
  UnitTestVTKDataSetReader.cxx
  UnitTestVTKDataSetWriter.cxx
)

set(unit_test_libraries viskores_io)

if(Viskores_ENABLE_RENDERING)
  list(APPEND unit_tests
    UnitTestImageWriter.cxx
  )

  if (Viskores_ENABLE_HDF5_IO)
    list(APPEND unit_tests
      UnitTestHDF5Image.cxx)
  endif()

  list(APPEND unit_test_libraries viskores_rendering)
endif()

viskores_unit_tests(SOURCES ${unit_tests} LIBRARIES ${unit_test_libraries})
