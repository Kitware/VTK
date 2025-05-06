//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/internal/ConfigureFor32.h>

#include <viskores/Types.h>

#include <viskores/testing/Testing.h>

// Size of 32 bits.
#define EXPECTED_SIZE 4

#if defined(VISKORES_USE_64BIT_IDS)
#error viskores::Id an unexpected size.
#endif

#if defined(VISKORES_USE_DOUBLE_PRECISION)
#error viskores::FloatDefault an unexpected size.
#endif

namespace
{

void TestTypeSizes()
{
  VISKORES_TEST_ASSERT(sizeof(viskores::Id) == EXPECTED_SIZE, "viskores::Id an unexpected size.");
  VISKORES_TEST_ASSERT(sizeof(viskores::FloatDefault) == EXPECTED_SIZE,
                       "viskores::FloatDefault an unexpected size.");
}
}

int UnitTestConfigureFor32(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestTypeSizes, argc, argv);
}
