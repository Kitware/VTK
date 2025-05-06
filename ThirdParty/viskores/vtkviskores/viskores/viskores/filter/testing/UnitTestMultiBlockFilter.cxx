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
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

#include <viskores/filter/clean_grid/CleanGrid.h>
#include <viskores/filter/contour/ClipWithField.h>
#include <viskores/filter/contour/Contour.h>
#include <viskores/filter/vector_analysis/Gradient.h>

#include <viskores/io/VTKDataSetReader.h>
#include <viskores/source/Tangle.h>

namespace
{
template <typename T>
viskores::FloatDefault ValueDifference(const T& a, const T& b)
{
  return viskores::Abs(a - b);
}
template <typename T>
viskores::FloatDefault ValueDifference(const viskores::Vec<T, 3>& a, const viskores::Vec<T, 3>& b)
{
  return viskores::Abs(a[0] - b[0]) + viskores::Abs(a[1] - b[1]) + viskores::Abs(a[2] - b[2]);
}

template <typename ArrayType>
void ValidateField(const ArrayType& truthField, const ArrayType& resultField)
{
  VISKORES_TEST_ASSERT(truthField.GetNumberOfValues() == resultField.GetNumberOfValues(),
                       "Wrong number of field values");
  const viskores::FloatDefault tol = static_cast<viskores::FloatDefault>(1e-3);

  viskores::Id numPts = truthField.GetNumberOfValues();
  const auto truthPortal = truthField.ReadPortal();
  const auto resultPortal = resultField.ReadPortal();
  for (viskores::Id j = 0; j < numPts; j++)
    VISKORES_TEST_ASSERT(ValueDifference(truthPortal.Get(j), resultPortal.Get(j)) < tol,
                         "Wrong value in field");
}

void ValidateResults(const viskores::cont::PartitionedDataSet& truth,
                     const viskores::cont::PartitionedDataSet& result,
                     const std::string& varName,
                     bool isScalar = true)
{
  VISKORES_TEST_ASSERT(truth.GetNumberOfPartitions() == result.GetNumberOfPartitions());
  viskores::Id numDS = truth.GetNumberOfPartitions();
  for (viskores::Id i = 0; i < numDS; i++)
  {
    auto truthDS = truth.GetPartition(i);
    auto resultDS = result.GetPartition(i);

    VISKORES_TEST_ASSERT(truthDS.GetNumberOfPoints() == resultDS.GetNumberOfPoints(),
                         "Wrong number of points");
    VISKORES_TEST_ASSERT(truthDS.GetNumberOfCells() == resultDS.GetNumberOfCells(),
                         "Wrong number of cells");
    VISKORES_TEST_ASSERT(resultDS.HasField(varName), "Missing field");

    if (isScalar)
    {
      viskores::cont::ArrayHandle<viskores::Float32> truthField, resultField;
      truthDS.GetField(varName).GetData().AsArrayHandle(truthField);
      resultDS.GetField(varName).GetData().AsArrayHandle(resultField);
      ValidateField(truthField, resultField);
    }
    else
    {
      viskores::cont::ArrayHandle<viskores::Vec<viskores::Float32, 3>> truthField, resultField;
      truthDS.GetField(varName).GetData().AsArrayHandle(truthField);
      resultDS.GetField(varName).GetData().AsArrayHandle(resultField);
      ValidateField(truthField, resultField);
    }
  }
}
} //namespace


void TestMultiBlockFilter()
{
  viskores::cont::PartitionedDataSet pds;

  for (int i = 0; i < 10; i++)
  {
    viskores::Id3 dims(10 + i, 10 + i, 10 + i);
    viskores::source::Tangle tangle;
    tangle.SetCellDimensions(dims);
    pds.AppendPartition(tangle.Execute());
  }

  std::cout << "ClipWithField" << std::endl;
  std::vector<viskores::cont::PartitionedDataSet> results;
  std::vector<bool> flags = { false, true };
  for (const auto doThreading : flags)
  {
    viskores::filter::contour::ClipWithField clip;
    clip.SetRunMultiThreadedFilter(doThreading);
    clip.SetClipValue(0.0);
    clip.SetActiveField("tangle");
    clip.SetFieldsToPass("tangle", viskores::cont::Field::Association::Points);
    auto result = clip.Execute(pds);
    VISKORES_TEST_ASSERT(result.GetNumberOfPartitions() == pds.GetNumberOfPartitions());
    results.push_back(result);
  }
  ValidateResults(results[0], results[1], "tangle");

  std::cout << "Contour" << std::endl;
  results.clear();
  for (const auto doThreading : flags)
  {
    viskores::filter::contour::Contour mc;
    mc.SetRunMultiThreadedFilter(doThreading);
    mc.SetGenerateNormals(true);
    mc.SetIsoValue(0, 0.5);
    mc.SetActiveField("tangle");
    mc.SetFieldsToPass("tangle", viskores::cont::Field::Association::Points);
    auto result = mc.Execute(pds);
    VISKORES_TEST_ASSERT(result.GetNumberOfPartitions() == pds.GetNumberOfPartitions());
    results.push_back(result);
  }
  ValidateResults(results[0], results[1], "tangle");

  std::cout << "CleanGrid" << std::endl;
  results.clear();
  for (const auto doThreading : flags)
  {
    viskores::filter::clean_grid::CleanGrid clean;
    clean.SetRunMultiThreadedFilter(doThreading);
    clean.SetCompactPointFields(true);
    clean.SetMergePoints(true);
    auto result = clean.Execute(pds);
    VISKORES_TEST_ASSERT(result.GetNumberOfPartitions() == pds.GetNumberOfPartitions());
    results.push_back(result);
  }
  ValidateResults(results[0], results[1], "tangle");

  std::cout << "Gradient" << std::endl;
  results.clear();
  for (const auto doThreading : flags)
  {
    viskores::filter::vector_analysis::Gradient grad;
    grad.SetRunMultiThreadedFilter(doThreading);
    grad.SetComputePointGradient(true);
    grad.SetActiveField("tangle");
    grad.SetOutputFieldName("gradient");
    auto result = grad.Execute(pds);
    VISKORES_TEST_ASSERT(result.GetNumberOfPartitions() == pds.GetNumberOfPartitions());
    results.push_back(result);
  }
  ValidateResults(results[0], results[1], "gradient", false);
}

int UnitTestMultiBlockFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestMultiBlockFilter, argc, argv);
}
