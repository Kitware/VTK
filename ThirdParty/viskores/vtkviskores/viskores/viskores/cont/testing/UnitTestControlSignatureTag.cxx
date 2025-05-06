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

// This test does not really need a device compiler
#define VISKORES_NO_ERROR_ON_MIXED_CUDA_CXX_TAG

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

void TestControlSignatures()
{
  VISKORES_IS_CONTROL_SIGNATURE_TAG(viskores::worklet::WorkletMapField::FieldIn);

  VISKORES_TEST_ASSERT(viskores::cont::arg::internal::ControlSignatureTagCheck<
                         viskores::worklet::WorkletMapField::FieldIn>::Valid,
                       "Bad check for FieldIn");

  VISKORES_TEST_ASSERT(viskores::cont::arg::internal::ControlSignatureTagCheck<
                         viskores::worklet::WorkletMapField::FieldOut>::Valid,
                       "Bad check for FieldOut");

  VISKORES_TEST_ASSERT(
    !viskores::cont::arg::internal::ControlSignatureTagCheck<viskores::exec::arg::WorkIndex>::Valid,
    "Bad check for WorkIndex");

  VISKORES_TEST_ASSERT(
    !viskores::cont::arg::internal::ControlSignatureTagCheck<viskores::Id>::Valid,
    "Bad check for viskores::Id");
}

} // anonymous namespace

int UnitTestControlSignatureTag(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestControlSignatures, argc, argv);
}
