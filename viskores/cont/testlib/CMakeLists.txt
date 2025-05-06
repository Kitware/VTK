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

# Headers, which are used by tests all over Viskores, are expected to be in the
# testing directory where tests are located. We could move things around so
# that this module is built in the testing directory and the tests are created
# somewhere else, which would also be confusing to developers trying to add
# tests. This seems like the least confusing option.
set(testing_library_headers
  ../testing/ExplicitTestData.h
  ../testing/MakeTestDataSet.h
  ../testing/Testing.h
  )

set(testing_library_sources
  MakeTestDataSet.cxx
  TestEqualArrayHandles.cxx
  Testing.cxx
  )

viskores_library(
  NAME viskores_cont_testing
  SOURCES ${testing_library_sources}
  HEADERS ${testing_library_headers}
  )
