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

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCartesianProduct.h>

#include <viskores/cont/testing/Testing.h>

#include <vector>

namespace ArrayHandleCartesianProductNamespace
{

template <typename T>
void ArrayHandleCPBasic(viskores::cont::ArrayHandle<T> x,
                        viskores::cont::ArrayHandle<T> y,
                        viskores::cont::ArrayHandle<T> z)

{
  viskores::cont::ArrayHandleCartesianProduct<viskores::cont::ArrayHandle<T>,
                                              viskores::cont::ArrayHandle<T>,
                                              viskores::cont::ArrayHandle<T>>
    cpArray;

  viskores::Id nx = x.GetNumberOfValues();
  viskores::Id ny = y.GetNumberOfValues();
  viskores::Id nz = z.GetNumberOfValues();
  viskores::Id n = nx * ny * nz;

  cpArray = viskores::cont::make_ArrayHandleCartesianProduct(x, y, z);

  //Make sure we have the right number of values.
  VISKORES_TEST_ASSERT(cpArray.GetNumberOfValues() == (nx * ny * nz),
                       "Cartesian array constructor has wrong number of values");
  VISKORES_TEST_ASSERT(cpArray.GetNumberOfComponentsFlat() == 3);

  //Make sure the values are correct.
  viskores::Vec<T, 3> val;
  auto xPortal = x.ReadPortal();
  auto yPortal = y.ReadPortal();
  auto zPortal = z.ReadPortal();
  auto cpPortal = cpArray.ReadPortal();
  for (viskores::Id i = 0; i < n; i++)
  {
    viskores::Id idx0 = (i % (nx * ny)) % nx;
    viskores::Id idx1 = (i % (nx * ny)) / nx;
    viskores::Id idx2 = i / (nx * ny);

    val = viskores::Vec<T, 3>(xPortal.Get(idx0), yPortal.Get(idx1), zPortal.Get(idx2));
    VISKORES_TEST_ASSERT(test_equal(cpPortal.Get(i), val), "Wrong value in array");
  }
}

template <typename T>
void createArr(std::vector<T>& arr, std::size_t n)
{
  arr.resize(n);
  for (std::size_t i = 0; i < n; i++)
    arr[i] = static_cast<T>(i);
}

template <typename T>
void RunTest()
{
  std::size_t nX = 11, nY = 13, nZ = 11;

  for (std::size_t i = 1; i < nX; i += 2)
  {
    for (std::size_t j = 1; j < nY; j += 4)
    {
      for (std::size_t k = 1; k < nZ; k += 5)
      {
        std::vector<T> X, Y, Z;
        createArr(X, nX);
        createArr(Y, nY);
        createArr(Z, nZ);

        ArrayHandleCPBasic(viskores::cont::make_ArrayHandle(X, viskores::CopyFlag::Off),
                           viskores::cont::make_ArrayHandle(Y, viskores::CopyFlag::Off),
                           viskores::cont::make_ArrayHandle(Z, viskores::CopyFlag::Off));
      }
    }
  }
}

void TestArrayHandleCartesianProduct()
{
  RunTest<viskores::Float32>();
  RunTest<viskores::Float64>();
  RunTest<viskores::Id>();
}

} // namespace ArrayHandleCartesianProductNamespace

int UnitTestArrayHandleCartesianProduct(int argc, char* argv[])
{
  using namespace ArrayHandleCartesianProductNamespace;
  return viskores::cont::testing::Testing::Run(TestArrayHandleCartesianProduct, argc, argv);
}
