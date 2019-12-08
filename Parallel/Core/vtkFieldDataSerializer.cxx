/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkFieldDataSerializer.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkFieldDataSerializer.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkStructuredData.h"
#include "vtkStructuredExtent.h"

#include <cassert> // For assert()
#include <cstring> // For memcpy

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
    vtkFieldDataSerializer::SerializeDataArray(dataArray, bytestream);
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
    vtkFieldDataSerializer::SerializeDataArray(subSet, bytestream);
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
    vtkFieldDataSerializer::SerializeDataArray(subSet, bytestream);
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
    vtkFieldDataSerializer::DeserializeDataArray(bytestream, dataArray);
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

  vtkIdType idx = 0;
  for (; idx < tupleIds->GetNumberOfIds(); ++idx)
  {
    vtkIdType tupleIdx = tupleIds->GetId(idx);
    assert("pre: tuple ID is out-of bounds" && (tupleIdx >= 0) &&
      (tupleIdx < inputDataArray->GetNumberOfTuples()));

    subSetArray->SetTuple(idx, tupleIdx, inputDataArray);
  } // END for all tuples to extract
  return (subSetArray);
}

//------------------------------------------------------------------------------
void vtkFieldDataSerializer::SerializeDataArray(
  vtkDataArray* dataArray, vtkMultiProcessStream& bytestream)
{
  if (dataArray == nullptr)
  {
    vtkGenericWarningMacro("data array is nullptr!");
    return;
  }

  // STEP 0: Serialize array information
  int dataType = dataArray->GetDataType();
  int numComp = dataArray->GetNumberOfComponents();
  int numTuples = dataArray->GetNumberOfTuples();

  // serialize array information
  bytestream << dataType << numTuples << numComp;
  bytestream << std::string(dataArray->GetName());

  // STEP 1: Push the raw data into the bytestream
  // TODO: Add more cases for more datatypes here (?)
  unsigned int size = numComp * numTuples;
  if (dataArray->IsA("vtkFloatArray"))
  {
    bytestream.Push(static_cast<vtkFloatArray*>(dataArray)->GetPointer(0), size);
  }
  else if (dataArray->IsA("vtkDoubleArray"))
  {
    bytestream.Push(static_cast<vtkDoubleArray*>(dataArray)->GetPointer(0), size);
  }
  else if (dataArray->IsA("vtkIntArray"))
  {
    bytestream.Push(static_cast<vtkIntArray*>(dataArray)->GetPointer(0), size);
  }
  else if (dataArray->IsA("vtkIdTypeArray"))
  {
    bytestream.Push(static_cast<vtkIdTypeArray*>(dataArray)->GetPointer(0), size);
  }
  else
  {
    assert("ERROR: cannot serialize data of given type" && false);
    cerr << "Cannot serialize data of type=" << dataArray->GetDataType() << endl;
  }
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
    vtkFieldDataSerializer::DeserializeDataArray(bytestream, dataArray);
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

  // STEP 0: Deserialize array information
  int dataType, numTuples, numComp;
  std::string name;

  bytestream >> dataType >> numTuples >> numComp >> name;
  assert("pre: numComp >= 1" && (numComp >= 1));

  // STEP 1: Construct vtkDataArray object
  dataArray = vtkDataArray::CreateDataArray(dataType);
  dataArray->SetNumberOfComponents(numComp);
  dataArray->SetNumberOfTuples(numTuples);
  dataArray->SetName(name.c_str());

  // STEP 2: Extract raw data to vtkDataArray
  // TODO: Add more cases for more datatypes here (?)
  unsigned int size = numTuples * numComp;
  void* rawPtr = dataArray->GetVoidPointer(0);
  assert("pre: raw pointer is nullptr!" && (rawPtr != nullptr));
  switch (dataType)
  {
    case VTK_FLOAT:
    {
      float* data = static_cast<float*>(rawPtr);
      bytestream.Pop(data, size);
    }
    break;
    case VTK_DOUBLE:
    {
      double* data = static_cast<double*>(rawPtr);
      bytestream.Pop(data, size);
    }
    break;
    case VTK_INT:
    {
      int* data = static_cast<int*>(rawPtr);
      bytestream.Pop(data, size);
    }
    break;
    case VTK_ID_TYPE:
    {
      vtkIdType* data = static_cast<vtkIdType*>(rawPtr);
      bytestream.Pop(data, size);
    }
    break;
    default:
      assert("ERROR: cannot serialize data of given type" && false);
      cerr << "Cannot serialize data of type=" << dataArray->GetDataType() << endl;
  }
}
