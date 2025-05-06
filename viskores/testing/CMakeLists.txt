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

if(Viskores_ENABLE_TESTING)
  set(unit_tests
    UnitTestArrayPortalBasic.cxx
    UnitTestArrayPortalValueReference.cxx
    UnitTestBinaryPredicates.cxx
    UnitTestBinaryOperators.cxx
    UnitTestBounds.cxx
    UnitTestCellShape.cxx
    UnitTestConfigureFor32.cxx
    UnitTestConfigureFor64.cxx
    UnitTestDeprecated.cxx
    UnitTestExceptions.cxx
    #UnitTestFunctionInterface.cxx #FIXME
    UnitTestHash.cxx
    UnitTestList.cxx
    UnitTestMatrix.cxx
    UnitTestNewtonsMethod.cxx
    UnitTestNoAssert.cxx
    UnitTestPair.cxx
    UnitTestRange.cxx
    UnitTestTesting.cxx
    UnitTestTransform3D.cxx
    UnitTestTuple.cxx
    UnitTestTypeList.cxx
    UnitTestTypes.cxx
    UnitTestTypeTraits.cxx
    UnitTestUnaryPredicates.cxx
    UnitTestVecAxisAlignedPointCoordinates.cxx
    UnitTestVecFromPortal.cxx
    UnitTestVecFromPortalPermute.cxx
    UnitTestVectorAnalysis.cxx
    UnitTestVecFlat.cxx
    UnitTestVecTraits.cxx
    UnitTestVecVariable.cxx
    )

  # Unit tests that have device-specific code to be tested
  set(unit_tests_device
    UnitTestAtomic.cxx
    UnitTestGeometry.cxx
    UnitTestLowerBound.cxx
    UnitTestMath.cxx
    UnitTestUpperBound.cxx
    )


  #variable tracking size limit exceeded with -fvar-tracking-assignments, retrying without
  set_source_files_properties(UnitTestBounds.cxx PROPERTIES
    COMPILE_OPTIONS "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU>>:-fno-var-tracking-assignments>")

  viskores_unit_tests(SOURCES ${unit_tests})

  viskores_unit_tests(
    NAME UnitTests_viskores_testing_device
    DEVICE_SOURCES ${unit_tests_device}
    ALL_BACKENDS
    )
endif()
