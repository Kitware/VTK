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
#include "vtkObjectFactory.h"
#include "vtkFieldData.h"
#include "vtkDataArray.h"
#include "vtkIdList.h"
#include "vtkStructuredData.h"
#include "vtkStringArray.h"
#include "vtkIntArray.h"
#include "vtkMultiProcessStream.h"

#include <cassert> // For assert()
#include <cstring> // For memcpy

vtkStandardNewMacro(vtkFieldDataSerializer);

//------------------------------------------------------------------------------
vtkFieldDataSerializer::vtkFieldDataSerializer()
{

}

//------------------------------------------------------------------------------
vtkFieldDataSerializer::~vtkFieldDataSerializer()
{

}

//------------------------------------------------------------------------------
void vtkFieldDataSerializer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//------------------------------------------------------------------------------
void vtkFieldDataSerializer::SerializeMetaData(
    vtkFieldData *fieldData, vtkMultiProcessStream& bytestream)
{
  if( fieldData == NULL )
    {
    vtkGenericWarningMacro("Field data is NULL!");
    return;
    }

  // STEP 0: Write the number of arrays
  bytestream << fieldData->GetNumberOfArrays();

  // STEP 1: Loop through each array and write the metadata
  for( int array=0; array < fieldData->GetNumberOfArrays(); ++array )
    {
    vtkDataArray *dataArray = fieldData->GetArray( array );
    assert("pre: data array should not be NULL!" && (dataArray != NULL));

    int dataType  = dataArray->GetDataType();
    int numComp   = dataArray->GetNumberOfComponents();
    int numTuples = dataArray->GetNumberOfTuples();

    // serialize array information
    bytestream << dataType << numTuples << numComp;
    bytestream << std::string( dataArray->GetName() );
    } // END for all arrays
}

//------------------------------------------------------------------------------
void vtkFieldDataSerializer::DeserializeMetaData(
    vtkMultiProcessStream& bytestream,
    vtkStringArray *names,
    vtkIntArray *datatypes,
    vtkIntArray *dimensions)
{
  if( bytestream.Empty() )
    {
    vtkGenericWarningMacro("ByteStream is empty");
    return;
    }

  if( (names == NULL) || (datatypes == NULL) || (dimensions == NULL) )
    {
    vtkGenericWarningMacro(
        "ERROR: caller must pre-allocation names/datatypes/dimensions!");
    return;
    }

  // STEP 0: Extract the number of arrays
  int NumberOfArrays;
  bytestream >> NumberOfArrays;
  if( NumberOfArrays == 0 )
    {
    return;
    }

  // STEP 1: Allocate output data-structures
  names->SetNumberOfValues(NumberOfArrays);
  datatypes->SetNumberOfValues(NumberOfArrays);
  dimensions->SetNumberOfComponents(2);
  dimensions->SetNumberOfTuples(NumberOfArrays);

  std::string *namesPtr = static_cast<std::string*>(names->GetVoidPointer(0));
  int *datatypesPtr     = static_cast<int*>(datatypes->GetVoidPointer(0));
  int *dimensionsPtr    = static_cast<int*>(dimensions->GetVoidPointer(0));

  // STEP 2: Extract metadata for each array in corresponding output arrays
  for( int arrayIdx=0; arrayIdx < NumberOfArrays; ++arrayIdx )
    {
    bytestream >> datatypesPtr[ arrayIdx ] >> dimensionsPtr[arrayIdx*2] >>
                  dimensionsPtr[arrayIdx*2+1] >> namesPtr[ arrayIdx ];
    } // END for all arrays
}

//------------------------------------------------------------------------------
void vtkFieldDataSerializer::Serialize(
    vtkFieldData *fieldData, vtkMultiProcessStream& bytestream)
{
  if( fieldData == NULL )
    {
    vtkGenericWarningMacro("Field data is NULL!");
    return;
    }

  // STEP 0: Write the number of arrays
  bytestream << fieldData->GetNumberOfArrays();

  if( fieldData->GetNumberOfArrays() == 0 )
    {
    return;
    }

  // STEP 1: Loop through each array and serialize its metadata
  for( int array=0; array < fieldData->GetNumberOfArrays(); ++array )
    {
    vtkDataArray *dataArray = fieldData->GetArray( array );
    vtkFieldDataSerializer::SerializeDataArray( dataArray, bytestream );
    } // END for all arrays
}

//------------------------------------------------------------------------------
void vtkFieldDataSerializer::SerializeTuples(
    vtkIdList *tupleIds, vtkFieldData *fieldData,
    vtkMultiProcessStream& bytestream )
{
  if( fieldData == NULL )
   {
   vtkGenericWarningMacro("Field data is NULL!");
   return;
   }

  // STEP 0: Write the number of arrays
  bytestream << fieldData->GetNumberOfArrays();

  if( fieldData->GetNumberOfArrays() == 0 )
    {
    return;
    }

  // STEP 1: Loop through each array, extract the data on the selected tuples
  // and serialize it
  for( int array=0; array < fieldData->GetNumberOfArrays(); ++array )
   {
   vtkDataArray *dataArray = fieldData->GetArray( array );

   // STEP 2: For each array extract only the selected tuples, i.e., a subset
   vtkDataArray *subSet = NULL;
   subSet = vtkFieldDataSerializer::ExtractSelectedTuples(tupleIds,dataArray);
   assert("pre: subset array is NULL!" && (subSet != NULL) );

   // STEP 3: Serialize only a subset of the data
   vtkFieldDataSerializer::SerializeDataArray( subSet, bytestream );
   subSet->Delete();
   } // END for all arrays

}

//------------------------------------------------------------------------------
void vtkFieldDataSerializer::SerializeSubExtent(
    int subext[6], int gridExtent[6], vtkFieldData *fieldData,
    vtkMultiProcessStream& bytestream)
{
  if( fieldData == NULL )
   {
   vtkGenericWarningMacro("Field data is NULL!");
   return;
   }

  // STEP 0: Write the number of arrays
  bytestream << fieldData->GetNumberOfArrays();

  if( fieldData->GetNumberOfArrays() == 0 )
    {
    return;
    }

  // STEP 1: Loop through each array, extract the data within the subext
  // and serialize it
  for( int array=0; array < fieldData->GetNumberOfArrays(); ++array )
    {
    vtkDataArray *dataArray = fieldData->GetArray( array );

    // STEP 2: Extract the data within the requested sub-extent
    vtkDataArray *subSet = NULL;
    subSet = vtkFieldDataSerializer::ExtractSubExtentData(
        subext,gridExtent,dataArray);
    assert("pre: subset array is NULL!" && (subSet != NULL) );

    // STEP 3: Serialize only a subset of the data
    vtkFieldDataSerializer::SerializeDataArray( subSet, bytestream );
    subSet->Delete();
    } // END for all arrays

}

//------------------------------------------------------------------------------
vtkDataArray* vtkFieldDataSerializer::ExtractSubExtentData(
    int subext[6], int gridExtent[6], vtkDataArray *inputDataArray )
{
  if( inputDataArray == NULL )
    {
    vtkGenericWarningMacro("input data array is NULL!");
    return NULL;
    }

  // STEP 0: Acquire structured data description, i.e, XY_PLANE, XYZ_GRID etc.
  int description = vtkStructuredData::GetDataDescriptionFromExtent(gridExtent);

  // STEP 1: Allocate subset array
  vtkDataArray *subSetArray =
      vtkDataArray::CreateDataArray( inputDataArray->GetDataType() );
  subSetArray->SetName( inputDataArray->GetName() );
  subSetArray->SetNumberOfComponents( inputDataArray->GetNumberOfComponents());
  subSetArray->SetNumberOfTuples(
      vtkStructuredData::GetNumberOfNodes(subext,description));

  int ijk[3];
  for( ijk[0]=subext[0]; ijk[0] <= subext[1]; ++ijk[0] )
    {
    for( ijk[1]=subext[2]; ijk[1] <= subext[3]; ++ijk[1] )
      {
      for( ijk[2]=subext[4]; ijk[2] <= subext[5]; ++ijk[2] )
        {
        // Compute the source index from the grid extent. Note, this could be
        // a cell index if the incoming gridExtent and subext are cell extents.
        vtkIdType sourceIdx =
            vtkStructuredData::ComputePointIdForExtent(
                gridExtent,ijk,description);
        assert("pre: source index is out-of-bounds" &&
               (sourceIdx >= 0) &&
               (sourceIdx < inputDataArray->GetNumberOfTuples()));

        // Compute the target index in the subset array. Likewise, this could be
        // either a cell index or a node index depending on what gridExtent or
        // subext represent.
        vtkIdType targetIdx =
            vtkStructuredData::ComputePointIdForExtent(
                subext,ijk,description);
        assert("pre: target index is out-of-bounds" &&
               (targetIdx >= 0) &&
               (targetIdx < subSetArray->GetNumberOfTuples()));

        subSetArray->SetTuple( targetIdx, sourceIdx, inputDataArray );
        } // END for all k
      } // END for all j
    } // END for all i

  return(subSetArray);
}

//------------------------------------------------------------------------------
vtkDataArray* vtkFieldDataSerializer::ExtractSelectedTuples(
    vtkIdList *tupleIds, vtkDataArray *inputDataArray )
{
  vtkDataArray *subSetArray =
      vtkDataArray::CreateDataArray( inputDataArray->GetDataType() );
  subSetArray->SetName( inputDataArray->GetName() );
  subSetArray->SetNumberOfComponents( inputDataArray->GetNumberOfComponents());
  subSetArray->SetNumberOfTuples(tupleIds->GetNumberOfIds());

  vtkIdType idx = 0;
  for( ; idx < tupleIds->GetNumberOfIds(); ++idx )
    {
    vtkIdType tupleIdx = tupleIds->GetId(idx);
    assert("pre: tuple ID is out-of bounds" &&
           (tupleIdx >= 0) && (tupleIdx < inputDataArray->GetNumberOfTuples()));

    subSetArray->SetTuple( idx, tupleIdx, inputDataArray );
    } // END for all tuples to extract
  return( subSetArray );
}

//------------------------------------------------------------------------------
void vtkFieldDataSerializer::SerializeDataArray(
    vtkDataArray *dataArray, vtkMultiProcessStream& bytestream)
{
  if( dataArray == NULL )
    {
    vtkGenericWarningMacro("data array is NULL!");
    return;
    }

  // STEP 0: Serialize array information
  int dataType  = dataArray->GetDataType();
  int numComp   = dataArray->GetNumberOfComponents();
  int numTuples = dataArray->GetNumberOfTuples();

  // serialize array information
  bytestream << dataType << numTuples << numComp;
  bytestream << std::string( dataArray->GetName() );

  // STEP 1: Push the raw data into the bytestream
  // TODO: Add more cases for more datatypes here (?)
  unsigned int size = numComp*numTuples;
  switch( dataArray->GetDataType() )
    {
    case VTK_FLOAT:
       bytestream.Push(static_cast<float*>(dataArray->GetVoidPointer(0)),size);
       break;
     case VTK_DOUBLE:
       bytestream.Push(static_cast<double*>(dataArray->GetVoidPointer(0)),size);
       break;
     case VTK_INT:
       bytestream.Push(static_cast<int*>(dataArray->GetVoidPointer(0)),size);
       break;
     default:
       assert("ERROR: cannot serialize data of given type" && false);
       cerr << "Canot serialize data of type="
            << dataArray->GetDataType() << endl;

    }
}

//------------------------------------------------------------------------------
void vtkFieldDataSerializer::Deserialize(
    vtkMultiProcessStream& bytestream, vtkFieldData *fieldData)
{
  if( fieldData == NULL )
    {
    vtkGenericWarningMacro("FieldData is NULL!");
    return;
    }

  if( bytestream.Empty() )
    {
    vtkGenericWarningMacro("Bytestream is empty!");
    return;
    }

  // STEP 0: Get the number of arrays
  int numberOfArrays = 0;
  bytestream >> numberOfArrays;

  if( numberOfArrays == 0 )
    {
    return;
    }

  // STEP 1: Loop and deserialize each array
  for( int array=0; array < numberOfArrays; ++array )
    {
    vtkDataArray *dataArray = NULL;
    vtkFieldDataSerializer::DeserializeDataArray( bytestream,dataArray );
    assert("post: deserialized data array should not be NULL!" &&
            (dataArray != NULL));
    fieldData->AddArray( dataArray );
    dataArray->Delete();
    } // END for all arrays
}

//------------------------------------------------------------------------------
void vtkFieldDataSerializer::DeserializeDataArray(
    vtkMultiProcessStream& bytestream,
    vtkDataArray *&dataArray)
{
  if( bytestream.Empty() )
    {
    vtkGenericWarningMacro("Bytestream is empty!");
    return;
    }

  // STEP 0: Deserialize array information
  int dataType, numTuples, numComp;
  std::string name;

  bytestream >> dataType >> numTuples >> numComp >> name;

  // STEP 1: Construct vtkDataArray object
  dataArray = vtkDataArray::CreateDataArray( dataType );
  dataArray->SetNumberOfComponents( numComp );
  dataArray->SetNumberOfTuples( numTuples );
  dataArray->SetName( name.c_str() );

  // STEP 2: Extract raw data to vtkDataArray
  // TODO: Add more cases for more datatypes here (?)
  unsigned int size = 0;
  switch( dataType )
    {
    case VTK_FLOAT:
      {
      float *data = NULL;
      bytestream.Pop(data,size);
      assert("pre: deserialized raw data array is NULL" && (data != NULL) );

      float *dataArrayPtr = static_cast<float*>(dataArray->GetVoidPointer(0));
      assert("pre: data array pointer is NULL!" && (dataArrayPtr != NULL) );

      std::memcpy(dataArrayPtr,data,size*sizeof(float));
      delete [] data;
      }
      break;
    case VTK_DOUBLE:
      {
      double *data = NULL;
      bytestream.Pop(data,size);
      assert("pre: deserialized raw data array is NULL" && (data != NULL) );

      double *dataArrayPtr = static_cast<double*>(dataArray->GetVoidPointer(0));
      assert("pre: data array pointer is NULL!" && (dataArrayPtr != NULL) );

      std::memcpy(dataArrayPtr,data,size*sizeof(double));
      delete [] data;
      }
      break;
    case VTK_INT:
      {
      int *data = NULL;
      bytestream.Pop(data,size);
      assert("pre: deserialized raw data array is NULL" && (data != NULL) );

      int *dataArrayPtr = static_cast<int*>(dataArray->GetVoidPointer(0));
      assert("pre: data array pointer is NULL!" && (dataArrayPtr != NULL) );

      std::memcpy(dataArrayPtr,data,size*sizeof(int));
      delete [] data;
      }
      break;
    default:
      assert("ERROR: cannot serialize data of given type" && false);
      cerr << "Canot serialize data of type="
           << dataArray->GetDataType() << endl;
    }
}
