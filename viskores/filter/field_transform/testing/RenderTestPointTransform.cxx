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

#include <viskores/Math.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/io/VTKDataSetReader.h>

#include <viskores/filter/field_transform/PointTransform.h>
#include <viskores/filter/vector_analysis/VectorMagnitude.h>

#include <viskores/rendering/testing/RenderTest.h>
#include <viskores/rendering/testing/Testing.h>

namespace
{
void TestPointTransform()
{
  std::cout << "Generate Image for PointTransform filter with Translation" << std::endl;

  auto pathname =
    viskores::cont::testing::Testing::DataPath("unstructured/PointTransformTestDataSet.vtk");
  viskores::io::VTKDataSetReader reader(pathname);
  viskores::cont::DataSet dataSet = reader.ReadDataSet();

  viskores::filter::field_transform::PointTransform pointTransform;
  pointTransform.SetOutputFieldName("translation");
  pointTransform.SetTranslation(viskores::Vec3f(1, 1, 1));

  auto result = pointTransform.Execute(dataSet);

  // Need to take the magnitude of the "translation" field.
  // ColorMap only works with scalar fields (1 component)
  viskores::filter::vector_analysis::VectorMagnitude vectorMagnitude;
  vectorMagnitude.SetActiveField("translation");
  vectorMagnitude.SetOutputFieldName("pointvar");
  result = vectorMagnitude.Execute(result);
  result.PrintSummary(std::cout);

  viskores::rendering::testing::RenderTestOptions testOptions;
  testOptions.ColorTable = viskores::cont::ColorTable::Preset::Inferno;
  testOptions.EnableAnnotations = false;
  viskores::rendering::testing::RenderTest(
    result, "pointvar", "filter/point-transform.png", testOptions);
}
} // namespace

int RenderTestPointTransform(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestPointTransform, argc, argv);
}
