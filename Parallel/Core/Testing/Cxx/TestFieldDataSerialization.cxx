// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME TestFieldDataSerialization.cxx -- Test for vtkFieldDataSerializer
//
// .SECTION Description
//  Simple tests for serialization/de-serialization of field data.

#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFieldDataSerializer.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkMathUtilities.h"
#include "vtkMultiProcessStream.h"
#include "vtkPointData.h"
#include "vtkStringArray.h"
#include "vtkTestUtilities.h"

#include <cassert>
#include <sstream>

#include <iostream>

//------------------------------------------------------------------------------
vtkPointData* GetEmptyField()
{
  vtkPointData* field = vtkPointData::New();
  return (field);
}

//------------------------------------------------------------------------------
vtkIntArray* GetSampleIntArray(const int numTuples, const int numComp)
{
  assert("pre: numTuples > 0" && (numTuples > 0));
  assert("pre: numComp > 0" && (numComp > 0));

  vtkIntArray* array = vtkIntArray::New();
  array->SetNumberOfComponents(numComp);
  array->SetNumberOfTuples(numTuples);

  std::ostringstream oss;
  oss << "SampleIntArray-" << numComp;
  array->SetName(oss.str().c_str());

  int* ptr = array->GetPointer(0);
  int idx = 0;
  for (int i = 0; i < numTuples; ++i)
  {
    for (int j = 0; j < numComp; ++j)
    {
      ptr[i * numComp + j] = idx;
      ++idx;
    } // END for all components
  }   // END for all tuples

  return (array);
}

//------------------------------------------------------------------------------
vtkDoubleArray* GetSampleDoubleArray(const int numTuples, const int numComp)
{
  assert("pre: numTuples > 0" && (numTuples > 0));
  assert("pre: numComp > 0" && (numComp > 0));

  vtkDoubleArray* array = vtkDoubleArray::New();
  array->SetNumberOfComponents(numComp);
  array->SetNumberOfTuples(numTuples);

  std::ostringstream oss;
  oss << "SampleDoubleArray-" << numComp;
  array->SetName(oss.str().c_str());

  double* ptr = array->GetPointer(0);
  double val = 0.5;
  for (int i = 0; i < numTuples; ++i)
  {
    for (int j = 0; j < numComp; ++j)
    {
      ptr[i * numComp + j] = val;
      ++val;
    } // END for all components
  }   // END for all tuples

  return (array);
}

//------------------------------------------------------------------------------
vtkFloatArray* GetSampleFloatArray(const int numTuples, const int numComp)
{
  assert("pre: numTuples > 0" && (numTuples > 0));
  assert("pre: numComp > 0" && (numComp > 0));

  vtkFloatArray* array = vtkFloatArray::New();
  array->SetNumberOfComponents(numComp);
  array->SetNumberOfTuples(numTuples);

  std::ostringstream oss;
  oss << "SampleFloatArray-" << numComp;
  array->SetName(oss.str().c_str());

  float* ptr = array->GetPointer(0);
  float val = 0.5;
  for (int i = 0; i < numTuples; ++i)
  {
    for (int j = 0; j < numComp; ++j)
    {
      ptr[i * numComp + j] = val;
      ++val;
    } // END for all components
  }   // END for all tuples

  return (array);
}

//------------------------------------------------------------------------------
vtkPointData* GetSamplePointData(const int numTuples)
{
  assert("pre: numTuples > 0" && (numTuples > 0));

  // STEP 0: Get int field
  vtkPointData* field = vtkPointData::New();
  vtkIntArray* intDataArray = GetSampleIntArray(numTuples, 1);
  field->AddArray(intDataArray);
  intDataArray->Delete();

  // STEP 1: Get double field
  vtkDoubleArray* doubleDataArray = GetSampleDoubleArray(numTuples, 3);
  field->AddArray(doubleDataArray);
  doubleDataArray->Delete();

  // STEP 2: Get float field
  vtkFloatArray* floatDataArray = GetSampleFloatArray(numTuples, 2);
  field->AddArray(floatDataArray);
  floatDataArray->Delete();

  return (field);
}

//------------------------------------------------------------------------------
int TestFieldDataMetaData()
{
  int rc = 0;

  // STEP 0: Construct the field data
  vtkPointData* field = GetSamplePointData(5);
  assert("pre: field is nullptr!" && (field != nullptr));

  // STEP 1: Serialize the field data in a bytestream
  vtkMultiProcessStream bytestream;
  vtkFieldDataSerializer::SerializeMetaData(field, bytestream);

  // STEP 2: De-serialize the metadata
  vtkStringArray* namesArray = vtkStringArray::New();
  vtkIntArray* datatypesArray = vtkIntArray::New();
  vtkIntArray* dimensionsArray = vtkIntArray::New();

  vtkFieldDataSerializer::DeserializeMetaData(
    bytestream, namesArray, datatypesArray, dimensionsArray);

  vtkIdType NumberOfArrays = namesArray->GetNumberOfValues();
  std::string* names = static_cast<std::string*>(namesArray->GetPointer(0));
  int* datatypes = datatypesArray->GetPointer(0);
  int* dimensions = dimensionsArray->GetPointer(0);

  // STEP 3: Check deserialized data with expected values
  if (NumberOfArrays != field->GetNumberOfArrays())
  {
    ++rc;
    std::cerr << "ERROR: NumberOfArrays=" << NumberOfArrays
              << " expected val=" << field->GetNumberOfArrays() << "\n";
  }
  assert("pre: names arrays is nullptr" && (names != nullptr));
  assert("pre: datatypes is nullptr" && (datatypes != nullptr));
  assert("pre: dimensions is nullptr" && (dimensions != nullptr));

  for (int i = 0; i < NumberOfArrays; ++i)
  {
    vtkDataArray* dataArray = field->GetArray(i);
    if (dataArray->GetName() != names[i])
    {
      rc++;
      std::cerr << "ERROR: Array name mismatch!\n";
    }
    if (dataArray->GetDataType() != datatypes[i])
    {
      rc++;
      std::cerr << "ERROR: Array data type mismatch!\n";
    }
    if (dataArray->GetNumberOfTuples() != dimensions[i * 2])
    {
      rc++;
      std::cerr << "ERROR: Array number of tuples mismatch!\n";
    }
    if (dataArray->GetNumberOfComponents() != dimensions[i * 2 + 1])
    {
      rc++;
      std::cerr << "ERROR: Array number of components mismatch!\n";
    }
  } // END for all arrays

  // STEP 4: Clean up memory
  namesArray->Delete();
  datatypesArray->Delete();
  dimensionsArray->Delete();
  field->Delete();

  return (rc);
}

//------------------------------------------------------------------------------
int TestFieldData()
{
  int rc = 0;

  vtkPointData* field = GetSamplePointData(5);
  assert("pre: field is nullptr!" && (field != nullptr));

  vtkMultiProcessStream bytestream;
  vtkFieldDataSerializer::Serialize(field, bytestream);
  if (bytestream.Empty())
  {
    std::cerr << "ERROR: failed to serialize field data, bytestream is empty!\n";
    rc++;
    return (rc);
  }

  vtkPointData* field2 = vtkPointData::New();
  vtkFieldDataSerializer::Deserialize(bytestream, field2);
  if (!vtkTestUtilities::CompareFieldData(field, field2))
  {
    std::cerr << "ERROR: fields are not equal!\n";
    rc++;
    return (rc);
  }
  else
  {
    std::cout << "Fields are equal!\n";
    std::cout.flush();
  }

  field->Delete();
  field2->Delete();
  return (rc);
}

//------------------------------------------------------------------------------
int TestFieldDataSerialization(int argc, char* argv[])
{
  // Resolve compiler warnings about unused vars
  static_cast<void>(argc);
  static_cast<void>(argv);

  int rc = 0;
  rc += TestFieldData();

  std::cout << "Testing metadata serialization...";
  rc += TestFieldDataMetaData();
  std::cout << "[DONE]\n";
  return (rc);
}
