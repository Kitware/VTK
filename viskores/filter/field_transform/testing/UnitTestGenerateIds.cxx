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
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/field_transform/GenerateIds.h>
#include <viskores/source/Tangle.h>

namespace
{

void CheckField(const viskores::cont::UnknownArrayHandle& array,
                viskores::Id expectedSize,
                bool isFloat)
{
  VISKORES_TEST_ASSERT(array.GetNumberOfValues() == expectedSize);
  if (isFloat)
  {
    VISKORES_TEST_ASSERT(array.IsBaseComponentType<viskores::FloatDefault>());
    VISKORES_TEST_ASSERT(test_equal_ArrayHandles(
      array.AsArrayHandle<viskores::cont::ArrayHandle<viskores::FloatDefault>>(),
      viskores::cont::ArrayHandleIndex(expectedSize)));
  }
  else
  {
    VISKORES_TEST_ASSERT(array.IsBaseComponentType<viskores::Id>());
    VISKORES_TEST_ASSERT(
      test_equal_ArrayHandles(array.AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>(),
                              viskores::cont::ArrayHandleIndex(expectedSize)));
  }
}

void TryGenerateIds(
  viskores::filter::field_transform::GenerateIds& filter, // Why is Filter::Execute not const?
  const viskores::cont::DataSet& input)
{
  viskores::cont::DataSet output = filter.Execute(input);
  VISKORES_TEST_ASSERT(output.GetNumberOfPoints() == input.GetNumberOfPoints());
  VISKORES_TEST_ASSERT(output.GetNumberOfCells() == input.GetNumberOfCells());

  viskores::IdComponent expectedNumberOfFields = input.GetNumberOfFields();
  if (filter.GetGeneratePointIds())
  {
    ++expectedNumberOfFields;
  }
  if (filter.GetGenerateCellIds())
  {
    ++expectedNumberOfFields;
  }
  VISKORES_TEST_ASSERT(expectedNumberOfFields == output.GetNumberOfFields());

  if (filter.GetGeneratePointIds())
  {
    CheckField(output.GetPointField(filter.GetPointFieldName()).GetData(),
               output.GetNumberOfPoints(),
               filter.GetUseFloat());
  }

  if (filter.GetGeneratePointIds())
  {
    CheckField(output.GetPointField(filter.GetPointFieldName()).GetData(),
               output.GetNumberOfPoints(),
               filter.GetUseFloat());
  }

  if (filter.GetGenerateCellIds())
  {
    CheckField(output.GetCellField(filter.GetCellFieldName()).GetData(),
               output.GetNumberOfCells(),
               filter.GetUseFloat());
  }
}

void TestGenerateIds()
{
  viskores::source::Tangle tangle;
  tangle.SetCellDimensions({ 8, 8, 8 });
  viskores::cont::DataSet input = tangle.Execute();
  viskores::filter::field_transform::GenerateIds filter;

  TryGenerateIds(filter, input);

  filter.SetUseFloat(true);
  TryGenerateIds(filter, input);

  filter.SetUseFloat(false);
  filter.SetGenerateCellIds(false);
  filter.SetPointFieldName("indices");
  TryGenerateIds(filter, input);

  filter.SetGenerateCellIds(true);
  filter.SetGeneratePointIds(false);
  filter.SetCellFieldName("cell-indices");
  TryGenerateIds(filter, input);
}

} // anonymous namespace

int UnitTestGenerateIds(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestGenerateIds, argc, argv);
}
