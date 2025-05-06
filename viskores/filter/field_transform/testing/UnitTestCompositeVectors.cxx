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
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/field_transform/CompositeVectors.h>

namespace
{
template <typename ScalarType>
viskores::cont::DataSet MakeDataSet(viskores::IdComponent numArrays)
{
  viskores::cont::DataSet dataSet;

  viskores::IdComponent arrayLen = 100;

  for (viskores::IdComponent fieldIndex = 0; fieldIndex < numArrays; ++fieldIndex)
  {
    std::vector<ScalarType> pointarray;
    std::vector<ScalarType> cellarray;

    for (viskores::Id valueIndex = 0; valueIndex < arrayLen; ++valueIndex)
    {
      pointarray.push_back(static_cast<ScalarType>(fieldIndex * 1.1 + valueIndex * 1.1));
      cellarray.push_back(static_cast<ScalarType>(fieldIndex * 2.1 + valueIndex * 2.1));
    }

    dataSet.AddPointField("pointArray" + std::to_string(fieldIndex), pointarray);
    dataSet.AddCellField("cellArray" + std::to_string(fieldIndex), cellarray);
  }

  return dataSet;
}

template <typename ScalarType, typename VecType>
void CheckResults(viskores::cont::DataSet inDataSet,
                  const std::vector<std::string> FieldNames,
                  const std::string compositedName)
{
  //there are only three fields for this testing , it is ok to use viskores::IdComponent
  viskores::IdComponent numComponents = static_cast<viskores::IdComponent>(FieldNames.size());
  auto compositedField = inDataSet.GetField(compositedName);
  viskores::IdComponent compositedFieldLen =
    static_cast<viskores::IdComponent>(compositedField.GetNumberOfValues());

  using ArrayHandleType = viskores::cont::ArrayHandle<VecType>;
  ArrayHandleType compFieldArrayCopy;
  compositedField.GetData().AsArrayHandle(compFieldArrayCopy);

  auto compFieldReadPortal = compFieldArrayCopy.ReadPortal();

  for (viskores::IdComponent componentIndex = 0; componentIndex < numComponents; componentIndex++)
  {
    auto field = inDataSet.GetField(FieldNames[componentIndex]);
    VISKORES_TEST_ASSERT(compositedField.GetAssociation() == field.GetAssociation(),
                         "Got bad association value.");

    viskores::IdComponent fieldLen = static_cast<viskores::IdComponent>(field.GetNumberOfValues());
    VISKORES_TEST_ASSERT(compositedFieldLen == fieldLen, "Got wrong field length.");

    viskores::cont::ArrayHandle<ScalarType> fieldArrayHandle;
    field.GetData().AsArrayHandle(fieldArrayHandle);
    auto fieldReadPortal = fieldArrayHandle.ReadPortal();

    for (viskores::Id valueIndex = 0; valueIndex < fieldLen; valueIndex++)
    {
      auto compFieldVec = compFieldReadPortal.Get(valueIndex);
      auto comFieldValue = compFieldVec[static_cast<viskores::UInt64>(componentIndex)];
      auto fieldValue = fieldReadPortal.Get(valueIndex);
      VISKORES_TEST_ASSERT(comFieldValue == fieldValue, "Got bad field value.");
    }
  }
}


template <typename ScalarType, typename VecType>
void TestCompositeVectors(viskores::IdComponent numComponents)
{
  viskores::cont::DataSet inDataSet = MakeDataSet<ScalarType>(numComponents);
  viskores::filter::field_transform::CompositeVectors filter;

  // For the first pass (point field), we are going to use the generic `SetActiveField` method
  // and let the filter figure out how many fields.
  std::vector<std::string> pointFieldNames;
  for (viskores::IdComponent componentIndex = 0; componentIndex < numComponents; componentIndex++)
  {
    pointFieldNames.push_back("pointArray" + std::to_string(componentIndex));
    filter.SetActiveField(componentIndex, "pointArray" + std::to_string(componentIndex));
  }
  filter.SetOutputFieldName("CompositedFieldPoint");
  viskores::cont::DataSet outDataSetPointAssoci = filter.Execute(inDataSet);

  CheckResults<ScalarType, VecType>(
    outDataSetPointAssoci, pointFieldNames, filter.GetOutputFieldName());

  // For the second pass (cell field), we will use the `SetFieldNameList` method.
  std::vector<std::string> cellFieldNames;
  for (viskores::IdComponent componentIndex = 0; componentIndex < numComponents; componentIndex++)
  {
    cellFieldNames.push_back("cellArray" + std::to_string(componentIndex));
  }
  filter.SetFieldNameList(cellFieldNames, viskores::cont::Field::Association::Cells);
  filter.SetOutputFieldName("CompositedFieldCell");

  viskores::cont::DataSet outDataSetCellAssoci = filter.Execute(inDataSet);
  CheckResults<ScalarType, VecType>(
    outDataSetCellAssoci, cellFieldNames, filter.GetOutputFieldName());
}

void CompositeVectors()
{
  TestCompositeVectors<viskores::FloatDefault, viskores::Vec2f>(2);
  TestCompositeVectors<viskores::FloatDefault, viskores::Vec3f>(3);
  TestCompositeVectors<viskores::FloatDefault, viskores::Vec<viskores::FloatDefault, 5>>(5);
  TestCompositeVectors<viskores::Id, viskores::Vec2i>(2);
  TestCompositeVectors<viskores::Id, viskores::Vec3i>(3);
  TestCompositeVectors<viskores::Id, viskores::Vec<viskores::Id, 5>>(5);
}

} // anonymous namespace

int UnitTestCompositeVectors(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(CompositeVectors, argc, argv);
}
