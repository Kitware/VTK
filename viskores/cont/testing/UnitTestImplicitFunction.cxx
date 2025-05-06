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

#include <viskores/ImplicitFunction.h>

#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

#include <viskores/worklet/WorkletMapField.h>

#include <array>

namespace
{

class EvaluateImplicitFunction : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut, FieldOut, ExecObject);
  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename VecType, typename ScalarType, typename FunctionType>
  VISKORES_EXEC void operator()(const VecType& point,
                                ScalarType& val,
                                VecType& gradient,
                                const FunctionType& function) const
  {
    val = function.Value(point);
    gradient = function.Gradient(point);
  }
};

constexpr std::array<viskores::Vec3f, 8> points_g = { { { 0, 0, 0 },
                                                        { 1, 0, 0 },
                                                        { 1, 0, 1 },
                                                        { 0, 0, 1 },
                                                        { 0, 1, 0 },
                                                        { 1, 1, 0 },
                                                        { 1, 1, 1 },
                                                        { 0, 1, 1 } } };

template <typename ImplicitFunctionType>
void EvaluateOnCoordinates(
  const ImplicitFunctionType& function,
  viskores::cont::ArrayHandle<viskores::FloatDefault>& values,
  viskores::cont::ArrayHandle<viskores::Vec<viskores::FloatDefault, 3>>& gradients)
{
  viskores::cont::Invoker invoke;
  auto points = viskores::cont::make_ArrayHandle(
    points_g.data(), static_cast<viskores::Id>(points_g.size()), viskores::CopyFlag::Off);
  invoke(EvaluateImplicitFunction{}, points, values, gradients, function);
}

template <typename ImplicitFunctorType>
void Try(ImplicitFunctorType& function,
         const std::array<viskores::FloatDefault, 8>& expectedValues,
         const std::array<viskores::Vec3f, 8>& expectedGradients)
{
  auto expectedValuesArray =
    viskores::cont::make_ArrayHandle(expectedValues.data(), 8, viskores::CopyFlag::Off);
  auto expectedGradientsArray =
    viskores::cont::make_ArrayHandle(expectedGradients.data(), 8, viskores::CopyFlag::Off);

  {
    viskores::cont::ArrayHandle<viskores::FloatDefault> values;
    viskores::cont::ArrayHandle<viskores::Vec<viskores::FloatDefault, 3>> gradients;
    EvaluateOnCoordinates(function, values, gradients);

    VISKORES_TEST_ASSERT(test_equal_ArrayHandles(values, expectedValuesArray));
    VISKORES_TEST_ASSERT(test_equal_ArrayHandles(gradients, expectedGradientsArray));
  }

  {
    viskores::ImplicitFunctionMultiplexer<ImplicitFunctorType> functionChoose(function);
    viskores::cont::ArrayHandle<viskores::FloatDefault> values;
    viskores::cont::ArrayHandle<viskores::Vec<viskores::FloatDefault, 3>> gradients;
    EvaluateOnCoordinates(functionChoose, values, gradients);

    VISKORES_TEST_ASSERT(test_equal_ArrayHandles(values, expectedValuesArray));
    VISKORES_TEST_ASSERT(test_equal_ArrayHandles(gradients, expectedGradientsArray));
  }

  {
    viskores::ImplicitFunctionGeneral functionChoose(function);
    viskores::cont::ArrayHandle<viskores::FloatDefault> values;
    viskores::cont::ArrayHandle<viskores::Vec<viskores::FloatDefault, 3>> gradients;
    EvaluateOnCoordinates(functionChoose, values, gradients);

    VISKORES_TEST_ASSERT(test_equal_ArrayHandles(values, expectedValuesArray));
    VISKORES_TEST_ASSERT(test_equal_ArrayHandles(gradients, expectedGradientsArray));
  }
}

void TestBox()
{
  std::cout << "Testing viskores::Box\n";

  std::cout << "  default box" << std::endl;
  viskores::Box box;
  Try(box,
      { { -0.5f, 0.5f, 0.707107f, 0.5f, 0.5f, 0.707107f, 0.866025f, 0.707107f } },
      { { viskores::Vec3f{ -1.0f, 0.0f, 0.0f },
          viskores::Vec3f{ 1.0f, 0.0f, 0.0f },
          viskores::Vec3f{ 0.707107f, 0.0f, 0.707107f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 0.0f, 1.0f, 0.0f },
          viskores::Vec3f{ 0.707107f, 0.707107f, 0.0f },
          viskores::Vec3f{ 0.57735f, 0.57735f, 0.57735f },
          viskores::Vec3f{ 0.0f, 0.707107f, 0.707107f } } });

  std::cout << "  Specified min/max box" << std::endl;
  box.SetMinPoint({ 0.0f, -0.5f, -0.5f });
  box.SetMaxPoint({ 1.5f, 1.5f, 0.5f });
  Try(box,
      { { 0.0f, -0.5f, 0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.5f } },
      { { viskores::Vec3f{ -1.0f, 0.0f, 0.0f },
          viskores::Vec3f{ 1.0f, 0.0f, 0.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
          viskores::Vec3f{ -1.0f, 0.0f, 0.0f },
          viskores::Vec3f{ 1.0f, 0.0f, 0.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f } } });

  std::cout << "  Specified bounds box" << std::endl;
  box.SetBounds(
    { viskores::Range(0.0, 1.5), viskores::Range(-0.5, 1.5), viskores::Range(-0.5, 0.5) });
  Try(box,
      { { 0.0f, -0.5f, 0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.5f } },
      { { viskores::Vec3f{ -1.0f, 0.0f, 0.0f },
          viskores::Vec3f{ 1.0f, 0.0f, 0.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
          viskores::Vec3f{ -1.0f, 0.0f, 0.0f },
          viskores::Vec3f{ 1.0f, 0.0f, 0.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f } } });
}

void TestCylinder()
{
  std::cout << "Testing viskores::Cylinder\n";

  std::cout << "  Default cylinder" << std::endl;
  viskores::Cylinder cylinder;
  Try(cylinder,
      { { -0.25f, 0.75f, 1.75f, 0.75f, -0.25f, 0.75f, 1.75f, 0.75f } },
      { { viskores::Vec3f{ 0.0f, 0.0f, 0.0f },
          viskores::Vec3f{ 2.0f, 0.0f, 0.0f },
          viskores::Vec3f{ 2.0f, 0.0f, 2.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 2.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 0.0f },
          viskores::Vec3f{ 2.0f, 0.0f, 0.0f },
          viskores::Vec3f{ 2.0f, 0.0f, 2.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 2.0f } } });

  std::cout << "  Translated, scaled cylinder" << std::endl;
  cylinder.SetCenter({ 0.0f, 0.0f, 1.0f });
  cylinder.SetAxis({ 0.0f, 1.0f, 0.0f });
  cylinder.SetRadius(1.0f);
  Try(cylinder,
      { { 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, -1.0f } },
      { { viskores::Vec3f{ 0.0f, 0.0f, -2.0f },
          viskores::Vec3f{ 2.0f, 0.0f, -2.0f },
          viskores::Vec3f{ 2.0f, 0.0f, 0.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 0.0f },
          viskores::Vec3f{ 0.0f, 0.0f, -2.0f },
          viskores::Vec3f{ 2.0f, 0.0f, -2.0f },
          viskores::Vec3f{ 2.0f, 0.0f, 0.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 0.0f } } });

  std::cout << "  Non-unit axis" << std::endl;
  cylinder.SetCenter({ 0.0f, 0.0f, 0.0f });
  cylinder.SetAxis({ 1.0f, 1.0f, 0.0f });
  cylinder.SetRadius(1.0f);
  Try(cylinder,
      { { -1.0f, -0.5f, 0.5f, 0.0f, -0.5f, -1.0f, 0.0f, 0.5f } },
      { { viskores::Vec3f{ 0.0f, 0.0f, 0.0f },
          viskores::Vec3f{ 1.0f, -1.0f, 0.0f },
          viskores::Vec3f{ 1.0f, -1.0f, 2.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 2.0f },
          viskores::Vec3f{ -1.0f, 1.0f, 0.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 0.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 2.0f },
          viskores::Vec3f{ -1.0f, 1.0f, 2.0f } } });
}

void TestFrustum()
{
  std::cout << "Testing viskores::Frustum\n";

  std::cout << "  With corner points" << std::endl;
  viskores::Vec3f cornerPoints[8] = {
    { -0.5f, 0.0f, -0.5f }, // 0
    { -0.5f, 0.0f, 0.5f },  // 1
    { 0.5f, 0.0f, 0.5f },   // 2
    { 0.5f, 0.0f, -0.5f },  // 3
    { -0.5f, 1.0f, -0.5f }, // 4
    { -0.5f, 1.0f, 0.5f },  // 5
    { 1.5f, 1.0f, 0.5f },   // 6
    { 1.5f, 1.0f, -0.5f }   // 7
  };
  viskores::Frustum frustum{ cornerPoints };
  Try(frustum,
      { { 0.0f, 0.353553f, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f, 0.5f } },
      { { viskores::Vec3f{ 0.0f, -1.0f, 0.0f },
          viskores::Vec3f{ 0.707107f, -0.707107f, 0.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 0.0f, 1.0f, 0.0f },
          viskores::Vec3f{ 0.0f, 1.0f, 0.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f } } });

  std::cout << "  With 6 planes" << std::endl;
  viskores::Vec3f planePoints[6] = { { 0.0f, 0.0f, 0.0f },  { 1.0f, 1.0f, 0.0f },
                                     { -0.5f, 0.0f, 0.0f }, { 0.5f, 0.0f, 0.0f },
                                     { 0.0f, 0.0f, -0.5f }, { 0.0f, 0.0f, 0.5f } };
  viskores::Vec3f planeNormals[6] = { { 0.0f, -1.0f, 0.0f }, { 0.707107f, 0.707107f, 0.0f },
                                      { -1.0f, 0.0f, 0.0f }, { 0.707107f, -0.707107f, 0.0f },
                                      { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, 1.0f } };
  frustum.SetPlanes(planePoints, planeNormals);
  Try(frustum,
      { { 0.0f, 0.353553f, 0.5f, 0.5f, -0.5f, 0.0f, 0.5f, 0.5f } },
      { { viskores::Vec3f{ 0.0f, -1.0f, 0.0f },
          viskores::Vec3f{ 0.707107f, -0.707107f, 0.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
          viskores::Vec3f{ -1.0f, 0.0f, 0.0f },
          viskores::Vec3f{ 0.707107f, 0.707107f, 0.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f } } });
}

void TestPlane()
{
  std::cout << "Testing viskores::Plane\n";

  std::cout << "  Default plane" << std::endl;
  viskores::Plane plane;
  Try(plane,
      { { 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f } },
      { { viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 1.0f } } });

  std::cout << "  Normal of length 2" << std::endl;
  plane.SetOrigin({ 1.0f, 1.0f, 1.0f });
  plane.SetNormal({ 0.0f, 0.0f, 2.0f });
  Try(plane,
      { { -2.0f, -2.0f, 0.0f, 0.0f, -2.0f, -2.0f, 0.0f, 0.0f } },
      { { viskores::Vec3f{ 0.0f, 0.0f, 2.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 2.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 2.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 2.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 2.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 2.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 2.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 2.0f } } });

  std::cout << "  Oblique plane" << std::endl;
  plane.SetOrigin({ 0.5f, 0.5f, 0.5f });
  plane.SetNormal({ 1.0f, 0.0f, 1.0f });
  Try(plane,
      { { -1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f } },
      { { viskores::Vec3f{ 1.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 1.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 1.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 1.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 1.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 1.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 1.0f, 0.0f, 1.0f },
          viskores::Vec3f{ 1.0f, 0.0f, 1.0f } } });

  std::cout << "  Another oblique plane" << std::endl;
  plane.SetNormal({ -1.0f, 0.0f, -1.0f });
  Try(plane,
      { { 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f } },
      { { viskores::Vec3f{ -1.0f, 0.0f, -1.0f },
          viskores::Vec3f{ -1.0f, 0.0f, -1.0f },
          viskores::Vec3f{ -1.0f, 0.0f, -1.0f },
          viskores::Vec3f{ -1.0f, 0.0f, -1.0f },
          viskores::Vec3f{ -1.0f, 0.0f, -1.0f },
          viskores::Vec3f{ -1.0f, 0.0f, -1.0f },
          viskores::Vec3f{ -1.0f, 0.0f, -1.0f },
          viskores::Vec3f{ -1.0f, 0.0f, -1.0f } } });
}

void TestSphere()
{
  std::cout << "Testing viskores::Sphere\n";

  std::cout << "  Default sphere" << std::endl;
  viskores::Sphere sphere;
  Try(sphere,
      { { -0.25f, 0.75f, 1.75f, 0.75f, 0.75f, 1.75f, 2.75f, 1.75f } },
      { { viskores::Vec3f{ 0.0f, 0.0f, 0.0f },
          viskores::Vec3f{ 2.0f, 0.0f, 0.0f },
          viskores::Vec3f{ 2.0f, 0.0f, 2.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 2.0f },
          viskores::Vec3f{ 0.0f, 2.0f, 0.0f },
          viskores::Vec3f{ 2.0f, 2.0f, 0.0f },
          viskores::Vec3f{ 2.0f, 2.0f, 2.0f },
          viskores::Vec3f{ 0.0f, 2.0f, 2.0f } } });

  std::cout << "  Shifted and scaled sphere" << std::endl;
  sphere.SetCenter({ 1.0f, 1.0f, 1.0f });
  sphere.SetRadius(1.0f);
  Try(sphere,
      { { 2.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f } },
      { { viskores::Vec3f{ -2.0f, -2.0f, -2.0f },
          viskores::Vec3f{ 0.0f, -2.0f, -2.0f },
          viskores::Vec3f{ 0.0f, -2.0f, 0.0f },
          viskores::Vec3f{ -2.0f, -2.0f, 0.0f },
          viskores::Vec3f{ -2.0f, 0.0f, -2.0f },
          viskores::Vec3f{ 0.0f, 0.0f, -2.0f },
          viskores::Vec3f{ 0.0f, 0.0f, 0.0f },
          viskores::Vec3f{ -2.0f, 0.0f, 0.0f } } });
}

void TestMultiPlane()
{
  std::cout << "Testing viskores::MultiPlane\n";
  std::cout << "  3 axis aligned planes intersected at (1, 1, 1)" << std::endl;
  viskores::MultiPlane<3> TriplePlane;
  //insert xy plane
  TriplePlane.AddPlane(viskores::Vec3f{ 1.0f, 1.0f, 0.0f }, viskores::Vec3f{ 0.0f, 0.0f, 1.0f });
  //insert yz plane
  TriplePlane.AddPlane(viskores::Vec3f{ 0.0f, 1.0f, 1.0f }, viskores::Vec3f{ 1.0f, 0.0f, 0.0f });
  //insert xz plane
  TriplePlane.AddPlane(viskores::Vec3f{ 1.0f, 0.0f, 1.0f }, viskores::Vec3f{ 0.0f, 1.0f, 0.0f });
  Try(TriplePlane,
      { { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f } },
      { {
        viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
        viskores::Vec3f{ 1.0f, 0.0f, 0.0f },
        viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
        viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
        viskores::Vec3f{ 0.0f, 1.0f, 0.0f },
        viskores::Vec3f{ 1.0f, 0.0f, 0.0f },
        viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
        viskores::Vec3f{ 0.0f, 0.0f, 1.0f },
      } });
  std::cout << "  test MultiPlane copy" << std::endl;
  viskores::MultiPlane<4> QuadPlane1(TriplePlane);
  viskores::MultiPlane<4> QuadPlane2 = TriplePlane;
  for (int i = 0; i < 3; i++)
  {
    VISKORES_TEST_ASSERT(QuadPlane1.GetPlane(i).GetOrigin() == TriplePlane.GetPlane(i).GetOrigin());
    VISKORES_TEST_ASSERT(QuadPlane1.GetPlane(i).GetNormal() == TriplePlane.GetPlane(i).GetNormal());
    VISKORES_TEST_ASSERT(QuadPlane2.GetPlane(i).GetOrigin() == TriplePlane.GetPlane(i).GetOrigin());
    VISKORES_TEST_ASSERT(QuadPlane1.GetPlane(i).GetNormal() == TriplePlane.GetPlane(i).GetNormal());
  }
}

void Run()
{
  TestBox();
  TestCylinder();
  TestFrustum();
  TestPlane();
  TestSphere();
  TestMultiPlane();
}

} // anonymous namespace

int UnitTestImplicitFunction(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
