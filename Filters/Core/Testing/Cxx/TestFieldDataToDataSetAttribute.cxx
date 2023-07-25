// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkFieldDataToDataSetAttribute.h"

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkMolecule.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

static const std::string FIRST_NAME = "firstArray";
static const std::string SECOND_NAME = "secondArray";

static const int FIRST_VALUE = 13;
static const double SECOND_VALUE = -3.7;

static const vtkIdType TESTED_INDEX = 42;

namespace FieldDataToAttributeDataUtils
{
void AddFieldDataArrays(vtkDataObject* obj, double shift = 0)
{
  vtkNew<vtkIntArray> array1;
  array1->SetName(FIRST_NAME.c_str());
  vtkNew<vtkDoubleArray> array2;
  array2->SetName(SECOND_NAME.c_str());
  array1->InsertNextValue(FIRST_VALUE + shift);
  array2->InsertNextValue(SECOND_VALUE + shift);

  vtkFieldData* fieldData = obj->GetFieldData();
  fieldData->AddArray(array1);
  fieldData->AddArray(array2);
}

bool CheckOutput(vtkDataObject* output, vtkDataObject::AttributeTypes attributeType, int size,
  const std::string& name, double value)
{
  vtkDataSetAttributes* outAttribute = output->GetAttributes(attributeType);
  if (!outAttribute)
  {
    vtkLog(ERROR,
      "Cannot find attribute type " << vtkDataObject::GetAssociationTypeAsString(attributeType));
    return false;
  }

  if (outAttribute->GetNumberOfArrays() != size)
  {
    vtkLog(ERROR,
      "Wrong number of attribute arrays for type " << attributeType << ". Has "
                                                   << outAttribute->GetNumberOfArrays());
    return false;
  }

  vtkDataArray* outArray = outAttribute->GetArray(name.c_str());
  if (!outArray)
  {
    vtkLog(ERROR, "Cannot find array in output with name '" << name << "'");
    return false;
  }

  if (outArray->GetNumberOfTuples() != outAttribute->GetNumberOfTuples())
  {
    vtkLog(ERROR,
      "Wrong array size : " << outArray->GetNumberOfTuples() << ". Expected "
                            << outAttribute->GetNumberOfTuples());
    return false;
  }

  if (outArray->GetTuple1(TESTED_INDEX) != value)
  {
    vtkLog(ERROR,
      "Wrong value for array: has " << outArray->GetTuple1(TESTED_INDEX) << "instead of " << value);
    return false;
  }

  return true;
}

bool TestDataObject(vtkDataObject* obj, vtkDataObject::AttributeTypes attributeType)
{
  AddFieldDataArrays(obj);

  vtkNew<vtkFieldDataToDataSetAttribute> forwarder;
  forwarder->SetInputData(obj);
  forwarder->SetOutputFieldType(attributeType);
  forwarder->Update();

  bool ret = true;

  vtkDataObject* output = forwarder->GetOutput();

  // Some data object can have default data arrays. For instance molecules have default AtomData
  // array (atomic number) and BondData array (bond order).
  int numberOfArrays = obj->GetAttributes(attributeType)->GetNumberOfArrays();

  ret = ret && CheckOutput(output, attributeType, numberOfArrays + 2, FIRST_NAME, FIRST_VALUE);
  ret = ret && CheckOutput(output, attributeType, numberOfArrays + 2, SECOND_NAME, SECOND_VALUE);

  forwarder->ProcessAllArraysOff();
  forwarder->AddFieldDataArray(SECOND_NAME.c_str());
  forwarder->Update();

  ret = ret && CheckOutput(output, attributeType, numberOfArrays + 1, SECOND_NAME, SECOND_VALUE);

  if (!ret)
  {
    vtkLog(ERROR, "Test fails for " << vtkDataObject::GetAssociationTypeAsString(attributeType));
  }

  return ret;
}

void TestPointCellData()
{
  vtkNew<vtkImageData> image;
  // create more than TESTED_INDEX elements
  image->SetDimensions(10, 10, 10);

  TestDataObject(image, vtkDataObject::CELL);
  TestDataObject(image, vtkDataObject::POINT);
}

void TestRowData()
{
  vtkNew<vtkTable> table;
  // create more than TESTED_INDEX elements
  table->SetNumberOfRows(2 * TESTED_INDEX);

  TestDataObject(table, vtkDataObject::ROW);
}

void TestVertexEdgeData()
{
  vtkNew<vtkMolecule> molecule;
  // create more than TESTED_INDEX elements
  for (int idx = 0; idx < TESTED_INDEX * 2; idx++)
  {
    molecule->AppendAtom();
  }

  TestDataObject(molecule, vtkDataObject::VERTEX);
  TestDataObject(molecule, vtkDataObject::EDGE);
}

void TestMultiBlock()
{
  vtkNew<vtkImageData> image;
  image->SetDimensions(10, 10, 10);
  AddFieldDataArrays(image);

  vtkNew<vtkImageData> image2;
  image->SetDimensions(10, 10, 10);
  const double shiftValue = 1;
  AddFieldDataArrays(image2, shiftValue);

  vtkNew<vtkMultiBlockDataSet> mblock;
  mblock->SetBlock(0, image);
  mblock->SetBlock(1, image2);

  const auto attributeType = vtkDataObject::POINT;
  vtkNew<vtkFieldDataToDataSetAttribute> forwarder;
  forwarder->SetInputData(mblock);
  forwarder->SetOutputFieldType(attributeType);
  forwarder->Update();
  vtkDataObject* output = forwarder->GetOutput();
  auto outMB = vtkMultiBlockDataSet::SafeDownCast(output);

  const int numberOfOutArrays = 2;
  bool ret =
    CheckOutput(outMB->GetBlock(0), attributeType, numberOfOutArrays, FIRST_NAME, FIRST_VALUE);
  ret = ret &&
    CheckOutput(outMB->GetBlock(0), attributeType, numberOfOutArrays, SECOND_NAME, SECOND_VALUE);
  ret = ret &&
    CheckOutput(
      outMB->GetBlock(1), attributeType, numberOfOutArrays, FIRST_NAME, FIRST_VALUE + shiftValue);
  ret = ret &&
    CheckOutput(
      outMB->GetBlock(1), attributeType, numberOfOutArrays, SECOND_NAME, SECOND_VALUE + shiftValue);
  if (!ret)
  {
    vtkLog(ERROR, "Test fails for vtkMultiBlockDataSet");
  }
}
};

int TestFieldDataToDataSetAttribute(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  FieldDataToAttributeDataUtils::TestPointCellData();
  FieldDataToAttributeDataUtils::TestRowData();
  FieldDataToAttributeDataUtils::TestVertexEdgeData();
  FieldDataToAttributeDataUtils::TestMultiBlock();
  return EXIT_SUCCESS;
}
