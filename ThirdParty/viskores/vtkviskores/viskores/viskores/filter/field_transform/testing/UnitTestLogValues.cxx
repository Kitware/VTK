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
#include <viskores/filter/field_transform/LogValues.h>

namespace
{
const viskores::Id dim = 10;
using BaseType = viskores::filter::field_transform::LogValues::LogBase;

template <typename T>
viskores::cont::DataSet MakeLogValuesTestDataSet()
{
  viskores::cont::DataSet dataSet;

  std::vector<T> pointarray;
  std::vector<T> cellarray;

  for (viskores::Id j = 0; j < dim; ++j)
  {
    for (viskores::Id i = 0; i < dim; ++i)
    {
      pointarray.push_back(static_cast<T>((j * dim + i) * 0.1));
      cellarray.push_back(static_cast<T>((i * dim + j) * 0.1));
    }
  }

  dataSet.AddPointField("pointScalarField", pointarray);
  dataSet.AddCellField("cellScalarField", cellarray);

  return dataSet;
}

void TestLogGeneral(
  BaseType base,
  std::string ActiveFieldName,
  viskores::cont::Field::Association association = viskores::cont::Field::Association::Any)
{
  viskores::cont::DataSet input = MakeLogValuesTestDataSet<viskores::FloatDefault>();
  viskores::filter::field_transform::LogValues filter;

  std::string LogFieldName = ActiveFieldName + "LogValues";

  filter.SetActiveField(ActiveFieldName, association);
  filter.SetOutputFieldName(LogFieldName);
  filter.SetBaseValue(base);

  viskores::cont::DataSet output = filter.Execute(input);

  viskores::cont::ArrayHandle<viskores::FloatDefault> rawArrayHandle;
  viskores::cont::ArrayHandle<viskores::FloatDefault> logArrayHandle;

  input.GetField(ActiveFieldName, association).GetData().AsArrayHandle(rawArrayHandle);
  output.GetField(LogFieldName, association).GetData().AsArrayHandle(logArrayHandle);

  auto rawPortal = rawArrayHandle.ReadPortal();
  auto logPortal = logArrayHandle.ReadPortal();

  typedef viskores::FloatDefault (*LogFuncPtr)(viskores::FloatDefault);
  LogFuncPtr LogFunc = nullptr;

  switch (base)
  {
    case BaseType::E:
    {
      LogFunc = &viskores::Log;
      break;
    }
    case BaseType::TWO:
    {
      LogFunc = &viskores::Log2;
      break;
    }
    case BaseType::TEN:
    {
      LogFunc = &viskores::Log10;
      break;
    }
    default:
    {
      VISKORES_TEST_ASSERT("unsupported base value");
    }
  }

  for (viskores::Id i = 0; i < rawArrayHandle.GetNumberOfValues(); ++i)
  {
    auto raw = rawPortal.Get(i);
    auto logv = logPortal.Get(i);
    if (raw == 0)
    {
      VISKORES_TEST_ASSERT(test_equal(logv, LogFunc(filter.GetMinValue())),
                           "log value was wrong for min value");
      continue;
    }
    VISKORES_TEST_ASSERT(test_equal(logv, LogFunc(raw)), "log value was wrong for test");
  }
}

void TestLogValues()
{
  std::string pointScalarField = "pointScalarField";
  using AsscoType = viskores::cont::Field::Association;

  TestLogGeneral(BaseType::TEN, pointScalarField, AsscoType::Points);
  TestLogGeneral(BaseType::TWO, pointScalarField, AsscoType::Points);
  TestLogGeneral(BaseType::E, pointScalarField, AsscoType::Points);

  std::string cellScalarField = "cellScalarField";
  TestLogGeneral(BaseType::TEN, cellScalarField, AsscoType::Cells);
  TestLogGeneral(BaseType::TWO, cellScalarField, AsscoType::Cells);
  TestLogGeneral(BaseType::E, cellScalarField, AsscoType::Cells);
}

} // anonymous namespace

int UnitTestLogValues(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestLogValues, argc, argv);
}
