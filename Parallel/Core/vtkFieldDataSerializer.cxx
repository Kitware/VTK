// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkFieldDataSerializer.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIntArray.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkStructuredData.h"
#include "vtkStructuredExtent.h"

#include <cassert> // For assert()
#include <cstring> // For memcpy

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkFieldDataSerializer);

//------------------------------------------------------------------------------
vtkFieldDataSerializer::vtkFieldDataSerializer() = default;

//------------------------------------------------------------------------------
vtkFieldDataSerializer::~vtkFieldDataSerializer() = default;

//------------------------------------------------------------------------------
void vtkFieldDataSerializer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkFieldDataSerializer::SerializeMetaData(
  vtkFieldData* fieldData, vtkMultiProcessStream& bytestream)
{
  if (fieldData == nullptr)
  {
    vtkGenericWarningMacro("Field data is nullptr!");
    return;
  }

  // STEP 0: Write the number of arrays
  bytestream << fieldData->GetNumberOfArrays();

  // STEP 1: Loop through each array and write the metadata
  for (int array = 0; array < fieldData->GetNumberOfArrays(); ++array)
  {
    vtkDataArray* dataArray = fieldData->GetArray(array);
    assert("pre: data array should not be nullptr!" && (dataArray != nullptr));

    int dataType = dataArray->GetDataType();
    int numComp = dataArray->GetNumberOfComponents();
    int numTuples = dataArray->GetNumberOfTuples();

    // serialize array information
    bytestream << dataType << numTuples << numComp;
    bytestream << std::string(dataArray->GetName());
  } // END for all arrays
}

//------------------------------------------------------------------------------
void vtkFieldDataSerializer::DeserializeMetaData(vtkMultiProcessStream& bytestream,
  vtkStringArray* names, vtkIntArray* datatypes, vtkIntArray* dimensions)
{
  if (bytestream.Empty())
  {
    vtkGenericWarningMacro("ByteStream is empty");
    return;
  }

  if ((names == nullptr) || (datatypes == nullptr) || (dimensions == nullptr))
  {
    vtkGenericWarningMacro("ERROR: caller must pre-allocation names/datatypes/dimensions!");
    return;
  }

  // STEP 0: Extract the number of arrays
  int NumberOfArrays;
  bytestream >> NumberOfArrays;
  if (NumberOfArrays == 0)
  {
    return;
  }

  // STEP 1: Allocate output data-structures
  names->SetNumberOfValues(NumberOfArrays);
  datatypes->SetNumberOfValues(NumberOfArrays);
  dimensions->SetNumberOfComponents(2);
  dimensions->SetNumberOfTuples(NumberOfArrays);

  std::string* namesPtr = names->GetPointer(0);
  int* datatypesPtr = datatypes->GetPointer(0);
  int* dimensionsPtr = dimensions->GetPointer(0);

  // STEP 2: Extract metadata for each array in corresponding output arrays
  for (int arrayIdx = 0; arrayIdx < NumberOfArrays; ++arrayIdx)
  {
    bytestream >> datatypesPtr[arrayIdx] >> dimensionsPtr[arrayIdx * 2] >>
      dimensionsPtr[arrayIdx * 2 + 1] >> namesPtr[arrayIdx];
  } // END for all arrays
}

//------------------------------------------------------------------------------
void vtkFieldDataSerializer::Serialize(vtkFieldData* fieldData, vtkMultiProcessStream& bytestream)
{
  if (fieldData == nullptr)
  {
    vtkGenericWarningMacro("Field data is nullptr!");
    return;
  }

  // STEP 0: Write the number of arrays
  bytestream << fieldData->GetNumberOfArrays();

  if (fieldData->GetNumberOfArrays() == 0)
  {
    return;
  }

  // STEP 1: Loop through each array and serialize its metadata
  for (int array = 0; array < fieldData->GetNumberOfArrays(); ++array)
  {
    vtkDataArray* dataArray = fieldData->GetArray(array);
    bytestream.Push(dataArray);
  } // END for all arrays
}

//------------------------------------------------------------------------------
void vtkFieldDataSerializer::SerializeTuples(
  vtkIdList* tupleIds, vtkFieldData* fieldData, vtkMultiProcessStream& bytestream)
{
  if (fieldData == nullptr)
  {
    vtkGenericWarningMacro("Field data is nullptr!");
    return;
  }

  // STEP 0: Write the number of arrays
  bytestream << fieldData->GetNumberOfArrays();

  if (fieldData->GetNumberOfArrays() == 0)
  {
    return;
  }

  // STEP 1: Loop through each array, extract the data on the selected tuples
  // and serialize it
  for (int array = 0; array < fieldData->GetNumberOfArrays(); ++array)
  {
    vtkDataArray* dataArray = fieldData->GetArray(array);

    // STEP 2: For each array extract only the selected tuples, i.e., a subset
    vtkDataArray* subSet = vtkFieldDataSerializer::ExtractSelectedTuples(tupleIds, dataArray);
    assert("pre: subset array is nullptr!" && (subSet != nullptr));

    // STEP 3: Serialize only a subset of the data
    bytestream.Push(subSet);
    subSet->Delete();
  } // END for all arrays
}

//------------------------------------------------------------------------------
void vtkFieldDataSerializer::SerializeSubExtent(
  int subext[6], int gridExtent[6], vtkFieldData* fieldData, vtkMultiProcessStream& bytestream)
{
  if (fieldData == nullptr)
  {
    vtkGenericWarningMacro("Field data is nullptr!");
    return;
  }

  // STEP 0: Write the number of arrays
  bytestream << fieldData->GetNumberOfArrays();

  if (fieldData->GetNumberOfArrays() == 0)
  {
    return;
  }

  // STEP 1: Loop through each array, extract the data within the subext
  // and serialize it
  for (int array = 0; array < fieldData->GetNumberOfArrays(); ++array)
  {
    vtkDataArray* dataArray = fieldData->GetArray(array);

    // STEP 2: Extract the data within the requested sub-extent
    vtkDataArray* subSet =
      vtkFieldDataSerializer::ExtractSubExtentData(subext, gridExtent, dataArray);
    assert("pre: subset array is nullptr!" && (subSet != nullptr));

    // STEP 3: Serialize only a subset of the data
    bytestream.Push(subSet);
    subSet->Delete();
  } // END for all arrays
}

//------------------------------------------------------------------------------
void vtkFieldDataSerializer::DeSerializeToSubExtent(
  int subext[6], int gridExtent[6], vtkFieldData* fieldData, vtkMultiProcessStream& bytestream)
{
  assert("pre: sub-extent outside grid-extent" && vtkStructuredExtent::Smaller(subext, gridExtent));

  if (fieldData == nullptr)
  {
    vtkGenericWarningMacro("Field data is nullptr!");
    return;
  }

  int numArrays = 0;
  bytestream >> numArrays;
  assert("post: numArrays mismatch!" && (numArrays == fieldData->GetNumberOfArrays()));

  int ijk[3];
  for (int array = 0; array < numArrays; ++array)
  {
    vtkDataArray* dataArray = nullptr;
    bytestream.Pop(dataArray);
    assert("post: dataArray is nullptr!" && (dataArray != nullptr));
    assert("post: fieldData does not have array!" && fieldData->HasArray(dataArray->GetName()));

    vtkDataArray* targetArray = fieldData->GetArray(dataArray->GetName());
    assert("post: ncomp mismatch!" &&
      (dataArray->GetNumberOfComponents() == targetArray->GetNumberOfComponents()));

    for (ijk[0] = subext[0]; ijk[0] <= subext[1]; ++ijk[0])
    {
      for (ijk[1] = subext[2]; ijk[1] <= subext[3]; ++ijk[1])
      {
        for (ijk[2] = subext[4]; ijk[2] <= subext[5]; ++ijk[2])
        {
          vtkIdType sourceIdx = vtkStructuredData::ComputePointIdForExtent(subext, ijk);
          assert("post: sourceIdx out-of-bounds!" && (sourceIdx >= 0) &&
            (sourceIdx < dataArray->GetNumberOfTuples()));

          vtkIdType targetIdx = vtkStructuredData::ComputePointIdForExtent(gridExtent, ijk);
          assert("post: targetIdx out-of-bounds!" && (targetIdx >= 0) &&
            (targetIdx < targetArray->GetNumberOfTuples()));

          targetArray->SetTuple(targetIdx, sourceIdx, dataArray);
        } // END for all k
      }   // END for all j
    }     // END for all i

    dataArray->Delete();
  } // END for all arrays
}

//------------------------------------------------------------------------------
vtkDataArray* vtkFieldDataSerializer::ExtractSubExtentData(
  int subext[6], int gridExtent[6], vtkDataArray* inputDataArray)
{
  if (inputDataArray == nullptr)
  {
    vtkGenericWarningMacro("input data array is nullptr!");
    return nullptr;
  }

  // STEP 0: Acquire structured data description, i.e, XY_PLANE, XYZ_GRID etc.
  int description = vtkStructuredData::GetDataDescriptionFromExtent(gridExtent);

  // STEP 1: Allocate subset array
  vtkDataArray* subSetArray = vtkDataArray::CreateDataArray(inputDataArray->GetDataType());
  subSetArray->SetName(inputDataArray->GetName());
  subSetArray->SetNumberOfComponents(inputDataArray->GetNumberOfComponents());
  subSetArray->SetNumberOfTuples(vtkStructuredData::GetNumberOfPoints(subext, description));

  int ijk[3];
  for (ijk[0] = subext[0]; ijk[0] <= subext[1]; ++ijk[0])
  {
    for (ijk[1] = subext[2]; ijk[1] <= subext[3]; ++ijk[1])
    {
      for (ijk[2] = subext[4]; ijk[2] <= subext[5]; ++ijk[2])
      {
        // Compute the source index from the grid extent. Note, this could be
        // a cell index if the incoming gridExtent and subext are cell extents.
        vtkIdType sourceIdx =
          vtkStructuredData::ComputePointIdForExtent(gridExtent, ijk, description);
        assert("pre: source index is out-of-bounds" && (sourceIdx >= 0) &&
          (sourceIdx < inputDataArray->GetNumberOfTuples()));

        // Compute the target index in the subset array. Likewise, this could be
        // either a cell index or a node index depending on what gridExtent or
        // subext represent.
        vtkIdType targetIdx = vtkStructuredData::ComputePointIdForExtent(subext, ijk, description);
        assert("pre: target index is out-of-bounds" && (targetIdx >= 0) &&
          (targetIdx < subSetArray->GetNumberOfTuples()));

        subSetArray->SetTuple(targetIdx, sourceIdx, inputDataArray);
      } // END for all k
    }   // END for all j
  }     // END for all i

  return (subSetArray);
}

//------------------------------------------------------------------------------
vtkDataArray* vtkFieldDataSerializer::ExtractSelectedTuples(
  vtkIdList* tupleIds, vtkDataArray* inputDataArray)
{
  vtkDataArray* subSetArray = vtkDataArray::CreateDataArray(inputDataArray->GetDataType());
  subSetArray->SetName(inputDataArray->GetName());
  subSetArray->SetNumberOfComponents(inputDataArray->GetNumberOfComponents());
  subSetArray->SetNumberOfTuples(tupleIds->GetNumberOfIds());
  subSetArray->InsertTuplesStartingAt(0, tupleIds, inputDataArray);
  return subSetArray;
}

//------------------------------------------------------------------------------
void vtkFieldDataSerializer::SerializeDataArray(
  vtkDataArray* dataArray, vtkMultiProcessStream& bytestream)
{
  bytestream.Push(dataArray);
}

//------------------------------------------------------------------------------
void vtkFieldDataSerializer::Deserialize(vtkMultiProcessStream& bytestream, vtkFieldData* fieldData)
{
  if (fieldData == nullptr)
  {
    vtkGenericWarningMacro("FieldData is nullptr!");
    return;
  }

  if (bytestream.Empty())
  {
    vtkGenericWarningMacro("Bytestream is empty!");
    return;
  }

  // STEP 0: Get the number of arrays
  int numberOfArrays = 0;
  bytestream >> numberOfArrays;

  if (numberOfArrays == 0)
  {
    return;
  }

  // STEP 1: Loop and deserialize each array
  for (int array = 0; array < numberOfArrays; ++array)
  {
    vtkDataArray* dataArray = nullptr;
    bytestream.Pop(dataArray);
    assert("post: deserialized data array should not be nullptr!" && (dataArray != nullptr));
    fieldData->AddArray(dataArray);
    dataArray->Delete();
  } // END for all arrays
}

//------------------------------------------------------------------------------
void vtkFieldDataSerializer::DeserializeDataArray(
  vtkMultiProcessStream& bytestream, vtkDataArray*& dataArray)
{
  if (bytestream.Empty())
  {
    vtkGenericWarningMacro("Bytestream is empty!");
    return;
  }
  bytestream.Pop(dataArray);
}
VTK_ABI_NAMESPACE_END
