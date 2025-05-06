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



#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/ArrayHandleXGCCoordinates.h>
#include <viskores/cont/ArrayRangeCompute.h>
#include <viskores/cont/testing/Testing.h>

#include <algorithm>

namespace
{
//std::vector<viskores::Vec<viskores::Float32,3>> points_rz = { viskores::Vec<viskores::Float32,3>(1.72485139f, 0.020562f,   1.73493571f),
//                                viskores::Vec<viskores::Float32,3>(0.02052826f, 1.73478011f, 0.02299051f )}; //really a vec<float,2>
std::vector<float> points_rz = { 1.72485139f, 0.020562f,   1.73493571f,
                                 0.02052826f, 1.73478011f, 0.02299051f }; //really a vec<float,2>
std::vector<float> correct_x_coords = {
  1.72485139f,      1.73493571f,      1.73478011f,      1.21965411f,  1.22678481f,  1.22667478f,
  1.05616686e-16f,  1.06234173e-16f,  1.06224646e-16f,  -1.21965411f, -1.22678481f, -1.22667478f,
  -1.72485139f,     -1.73493571f,     -1.73478011f,     -1.21965411f, -1.22678481f, -1.22667478f,
  -3.16850059e-16f, -3.18702520e-16f, -3.18673937e-16f, 1.21965411f,  1.22678481f,  1.22667478f
};
std::vector<float> correct_y_coords = { 0.0f,
                                        0.0f,
                                        0.0f,
                                        1.21965411f,
                                        1.22678481f,
                                        1.22667478f,
                                        1.72485139f,
                                        1.73493571f,
                                        1.73478011f,
                                        1.21965411f,
                                        1.22678481f,
                                        1.22667478f,
                                        2.11233373e-16f,
                                        2.12468346e-16f,
                                        2.12449291e-16f,
                                        -1.21965411f,
                                        -1.22678481f,
                                        -1.22667478f,
                                        -1.72485139f,
                                        -1.73493571f,
                                        -1.73478011f,
                                        -1.21965411f,
                                        -1.22678481f,
                                        -1.22667478f };
std::vector<float> correct_z_coords = { 0.020562f,   0.02052826f, 0.02299051f, 0.020562f,
                                        0.02052826f, 0.02299051f, 0.020562f,   0.02052826f,
                                        0.02299051f, 0.020562f,   0.02052826f, 0.02299051f,
                                        0.020562f,   0.02052826f, 0.02299051f, 0.020562f,
                                        0.02052826f, 0.02299051f, 0.020562f,   0.02052826f,
                                        0.02299051f, 0.020562f,   0.02052826f, 0.02299051f };

struct CopyValue : public viskores::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn, FieldOut);
  typedef _2 ExecutionSignature(_1);

  template <typename T>
  VISKORES_EXEC_CONT T&& operator()(T&& t) const
  {
    return std::forward<T>(t);
  }
};

template <typename T, typename S>
void verify_results(viskores::cont::ArrayHandle<viskores::Vec<T, 3>, S> const& handle)
{
  auto portal = handle.ReadPortal();
  VISKORES_TEST_ASSERT(portal.GetNumberOfValues() ==
                         static_cast<viskores::Id>(correct_x_coords.size()),
                       "coordinate portal size is incorrect");

  for (viskores::Id i = 0; i < handle.GetNumberOfValues(); ++i)
  {
    auto v = portal.Get(i);
    auto e = viskores::make_Vec(correct_x_coords[static_cast<std::size_t>(i)],
                                correct_y_coords[static_cast<std::size_t>(i)],
                                correct_z_coords[static_cast<std::size_t>(i)]);
    // std::cout << std::setprecision(4) << "computed " << v << " expected " << e << std::endl;
    VISKORES_TEST_ASSERT(test_equal(v, e), "incorrect conversion to Cartesian space");
  }
}

template <typename T>
void test_range(const viskores::cont::ArrayHandleXGCCoordinates<T>& handle)
{
  auto x_result = std::minmax_element(correct_x_coords.begin(), correct_x_coords.end());
  auto y_result = std::minmax_element(correct_y_coords.begin(), correct_y_coords.end());
  auto z_result = std::minmax_element(correct_z_coords.begin(), correct_z_coords.end());

  auto range = viskores::cont::ArrayRangeCompute(handle);
  auto rangePortal = range.ReadPortal();
  VISKORES_TEST_ASSERT(test_equal(rangePortal.Get(0).Min, *x_result.first), "incorrect min for x");
  VISKORES_TEST_ASSERT(test_equal(rangePortal.Get(0).Max, *x_result.second), "incorrect max for x");
  VISKORES_TEST_ASSERT(test_equal(rangePortal.Get(1).Min, *y_result.first), "incorrect min for y");
  VISKORES_TEST_ASSERT(test_equal(rangePortal.Get(1).Max, *y_result.second), "incorrect max for y");
  VISKORES_TEST_ASSERT(test_equal(rangePortal.Get(2).Min, *z_result.first), "incorrect min for z");
  VISKORES_TEST_ASSERT(test_equal(rangePortal.Get(2).Max, *z_result.second), "incorrect max for z");
}

int TestArrayHandleXGCCoordinates()
{
  const int numPlanes = 8;

  auto coords = viskores::cont::make_ArrayHandleXGCCoordinates(
    viskores::cont::make_ArrayHandle(points_rz, viskores::CopyFlag::Off), numPlanes, false);

  VISKORES_TEST_ASSERT(coords.GetNumberOfValues() ==
                         static_cast<viskores::Id>(((points_rz.size() / 2) * numPlanes)),
                       "coordinate size is incorrect");

  // Verify first that control is correct
  verify_results(coords);

  // Verify 1d scheduling by doing a copy to a viskores::ArrayHandle<Vec3>
  viskores::cont::ArrayHandle<viskores::Vec<float, 3>> output1D;
  viskores::worklet::DispatcherMapField<CopyValue> dispatcher;
  dispatcher.Invoke(coords, output1D);
  verify_results(output1D);

  test_range(coords);

  return 0;
}

} // end namespace anonymous

int UnitTestArrayHandleXGCCoordinates(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestArrayHandleXGCCoordinates, argc, argv);
}
