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

#include <viskores/filter/contour/Slice.h>

#include <viskores/ImplicitFunction.h>
#include <viskores/filter/geometry_refinement/Tetrahedralize.h>
#include <viskores/source/Wavelet.h>

#include <viskores/rendering/testing/RenderTest.h>
#include <viskores/rendering/testing/Testing.h>

namespace
{

void TestSliceStructuredPointsPlane()
{
  std::cout << "Generate Image for Slice by plane on structured points" << std::endl;

  viskores::source::Wavelet wavelet;
  wavelet.SetExtent(viskores::Id3(-8), viskores::Id3(8));
  auto ds = wavelet.Execute();

  viskores::Plane plane(viskores::Plane::Vector{ 1, 1, 1 });
  viskores::filter::contour::Slice slice;
  slice.SetImplicitFunction(plane);
  auto result = slice.Execute(ds);

  result.PrintSummary(std::cout);

  viskores::rendering::testing::RenderTestOptions testOptions;
  testOptions.EnableAnnotations = false;
  testOptions.DataViewPadding = 0.08;
  viskores::rendering::testing::RenderTest(
    result, "RTData", "filter/slice-structured-points-plane.png", testOptions);
}

void TestSliceStructuredPointsSphere()
{
  std::cout << "Generate Image for Slice by sphere on structured points" << std::endl;

  viskores::source::Wavelet wavelet;
  wavelet.SetExtent(viskores::Id3(-8), viskores::Id3(8));
  auto ds = wavelet.Execute();

  viskores::Sphere sphere(8.5f);
  viskores::filter::contour::Slice slice;
  slice.SetImplicitFunction(sphere);
  auto result = slice.Execute(ds);

  result.PrintSummary(std::cout);

  viskores::rendering::testing::RenderTestOptions testOptions;
  testOptions.EnableAnnotations = false;
  testOptions.DataViewPadding = 0.08;
  viskores::rendering::testing::RenderTest(
    result, "RTData", "filter/slice-structured-points-sphere.png", testOptions);
}

void TestSliceUnstructuredGridPlane()
{
  std::cout << "Generate Image for Slice by plane on unstructured grid" << std::endl;

  viskores::source::Wavelet wavelet;
  wavelet.SetExtent(viskores::Id3(-8), viskores::Id3(8));
  auto ds = wavelet.Execute();
  viskores::filter::geometry_refinement::Tetrahedralize tetrahedralize;
  ds = tetrahedralize.Execute(ds);

  viskores::Plane plane(viskores::Plane::Vector{ 1 });
  viskores::filter::contour::Slice slice;
  slice.SetImplicitFunction(plane);
  auto result = slice.Execute(ds);

  result.PrintSummary(std::cout);

  viskores::rendering::testing::RenderTestOptions testOptions;
  testOptions.EnableAnnotations = false;
  testOptions.DataViewPadding = 0.08;
  viskores::rendering::testing::RenderTest(
    result, "RTData", "filter/slice-unstructured-grid-plane.png", testOptions);
}

void TestSliceUnstructuredGridCylinder()
{
  std::cout << "Generate Image for Slice by cylinder on unstructured grid" << std::endl;

  viskores::source::Wavelet wavelet;
  wavelet.SetExtent(viskores::Id3(-8), viskores::Id3(8));
  auto ds = wavelet.Execute();
  viskores::filter::geometry_refinement::Tetrahedralize tetrahedralize;
  ds = tetrahedralize.Execute(ds);

  viskores::Cylinder cylinder(viskores::Cylinder::Vector{ 0, 1, 0 }, 8.5f);
  viskores::filter::contour::Slice slice;
  slice.SetImplicitFunction(cylinder);
  auto result = slice.Execute(ds);

  result.PrintSummary(std::cout);

  viskores::rendering::testing::RenderTestOptions testOptions;
  testOptions.EnableAnnotations = false;
  testOptions.DataViewPadding = 0.08;
  viskores::rendering::testing::RenderTest(
    result, "RTData", "filter/slice-unstructured-grid-cylinder.png", testOptions);
}

void TestSliceFilter()
{
  TestSliceStructuredPointsPlane();
  TestSliceStructuredPointsSphere();
  TestSliceUnstructuredGridPlane();
  TestSliceUnstructuredGridCylinder();
}

} // namespace

int RenderTestSliceFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestSliceFilter, argc, argv);
}
