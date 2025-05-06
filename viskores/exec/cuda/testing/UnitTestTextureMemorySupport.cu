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

#include <viskores/cont/testing/Testing.h>
#include <viskores/exec/cuda/internal/ArrayPortalFromThrust.h>

namespace
{

struct customType
{
};

void TestScalarTextureLoad()
{
  using namespace viskores::exec::cuda::internal;
  using f = load_through_texture<viskores::Float32>;
  using i = load_through_texture<viskores::Int32>;
  using ui = load_through_texture<viskores::UInt8>;

  using ct = load_through_texture<customType>;

  VISKORES_TEST_ASSERT(f::WillUseTexture == 1, "Float32 can be loaded through texture memory");
  VISKORES_TEST_ASSERT(i::WillUseTexture == 1, "Int32 can be loaded through texture memory");
  VISKORES_TEST_ASSERT(ui::WillUseTexture == 1,
                       "Unsigned Int8 can be loaded through texture memory");
  VISKORES_TEST_ASSERT(ct::WillUseTexture == 0,
                       "Custom Types can't be loaded through texture memory");
}

void TestVecTextureLoad()
{
  using namespace viskores::exec::cuda::internal;
  using ui32_3 = load_through_texture<viskores::Vec<viskores::UInt32, 3>>;
  using f32_3 = load_through_texture<viskores::Vec<viskores::Float32, 3>>;
  using ui8_3 = load_through_texture<viskores::Vec<viskores::UInt8, 3>>;
  using f64_3 = load_through_texture<viskores::Vec<viskores::Float64, 3>>;

  using ui32_4 = load_through_texture<viskores::Vec<viskores::UInt32, 4>>;
  using f32_4 = load_through_texture<viskores::Vec<viskores::Float32, 4>>;
  using ui8_4 = load_through_texture<viskores::Vec<viskores::UInt8, 4>>;
  using f64_4 = load_through_texture<viskores::Vec<viskores::Float64, 4>>;

  using ct_3 = load_through_texture<viskores::Vec<customType, 3>>;
  using ct_4 = load_through_texture<viskores::Vec<customType, 4>>;

  VISKORES_TEST_ASSERT(ui32_3::WillUseTexture == 1, "Can be loaded through texture loads");
  VISKORES_TEST_ASSERT(f32_3::WillUseTexture == 1, "Can be loaded through texture loads");
  VISKORES_TEST_ASSERT(ui8_3::WillUseTexture == 1, "Can be loaded through texture loads");
  VISKORES_TEST_ASSERT(f64_3::WillUseTexture == 1, "Can be loaded through texture loads");

  VISKORES_TEST_ASSERT(ui32_4::WillUseTexture == 1, "Can be loaded through texture loads");
  VISKORES_TEST_ASSERT(f32_4::WillUseTexture == 1, "Can be loaded through texture loads");
  VISKORES_TEST_ASSERT(ui8_4::WillUseTexture == 1, "Can be loaded through texture loads");
  VISKORES_TEST_ASSERT(f64_4::WillUseTexture == 1, "Can be loaded through texture loads");

  VISKORES_TEST_ASSERT(ct_4::WillUseTexture == 0, "Can't be loaded through texture loads");
  VISKORES_TEST_ASSERT(ct_4::WillUseTexture == 0, "Can't be loaded through texture loads");
}

} // namespace

void TestTextureMemorySupport()
{
  TestScalarTextureLoad();
  TestVecTextureLoad();
}

int UnitTestTextureMemorySupport(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestTextureMemorySupport, argc, argv);
}
