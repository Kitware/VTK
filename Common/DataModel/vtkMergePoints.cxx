// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMergePoints.h"

#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkMergePoints);

namespace
{
//------------------------------------------------------------------------------
// Search for a point within an array, return -1 if not found.
template <class ArrayT>
vtkIdType vtkMergePointsFindPointInArray(const vtkIdType* idArray, vtkIdType nbOfIds,
  ArrayT* dataArray, const typename ArrayT::ValueType x[3])
{
  typename ArrayT::ValueType* data = dataArray->GetPointer(0);
  for (vtkIdType i = 0; i < nbOfIds; ++i)
  {
    vtkIdType ptId = idArray[i];
    typename ArrayT::ValueType* pt = data + 3 * ptId;
    if (x[0] == pt[0] && x[1] == pt[1] && x[2] == pt[2])
    {
      return ptId;
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
// Find the Id of the point within a bucket, return -1 if not found.
vtkIdType vtkMergePointsFindPointInBucket(vtkIdList* bucket, vtkPoints* points, const double x[3])
{
  //
  // Check the list of points in that bucket.
  //
  vtkIdType ptId = -1;
  vtkIdType nbOfIds = bucket->GetNumberOfIds();

  // For efficiency reasons, we break the vtkPoints abstraction and dig
  // down to the underlying float or double data storage.
  vtkDataArray* dataArray = points->GetData();
  vtkIdType* idArray = bucket->GetPointer(0);
  int dtype = dataArray->GetDataType();
  if (dtype == VTK_DOUBLE)
  {
    vtkDoubleArray* doubleArray = static_cast<vtkDoubleArray*>(dataArray);
    ptId = vtkMergePointsFindPointInArray(idArray, nbOfIds, doubleArray, x);
  }
  else if (dtype == VTK_FLOAT)
  {
    float f[3];
    f[0] = static_cast<float>(x[0]);
    f[1] = static_cast<float>(x[1]);
    f[2] = static_cast<float>(x[2]);
    vtkFloatArray* floatArray = static_cast<vtkFloatArray*>(dataArray);
    ptId = vtkMergePointsFindPointInArray(idArray, nbOfIds, floatArray, f);
  }
  else
  {
    // Using the double interface
    double pt[3];
    for (vtkIdType i = 0; i < nbOfIds; i++)
    {
      vtkIdType checkId = idArray[i];
      dataArray->GetTuple(checkId, pt);
      if (x[0] == pt[0] && x[1] == pt[1] && x[2] == pt[2])
      {
        ptId = checkId;
        break;
      }
    }
  }
  return ptId;
}

} // end namespace

//------------------------------------------------------------------------------
// Determine whether point given by x[3] has been inserted into points list.
// Return id of previously inserted point if this is true, otherwise return
// -1.
vtkIdType vtkMergePoints::IsInsertedPoint(const double x[3])
{
  //
  //  Locate bucket that point is in.
  //
  vtkIdType idx = this->GetBucketIndex(x);
  vtkIdList* bucket = this->HashTable[idx];

  // see whether we've got duplicate point
  vtkIdType ptId = -1;
  if (bucket)
  {
    //
    // Check the list of points in that bucket.
    //
    ptId = vtkMergePointsFindPointInBucket(bucket, this->Points, x);
  }

  return ptId;
}

//------------------------------------------------------------------------------
int vtkMergePoints::InsertUniquePoint(const double x[3], vtkIdType& id)
{
  //
  //  Locate bucket that point is in.
  //
  vtkIdType idx = this->GetBucketIndex(x);
  vtkIdList* bucket = this->HashTable[idx];

  if (bucket) // see whether we've got duplicate point
  {
    //
    // Check the list of points in that bucket.
    //
    vtkIdType ptId = vtkMergePointsFindPointInBucket(bucket, this->Points, x);
    if (ptId != -1)
    {
      // point is already in the list, return 0 and set the id parameter
      id = ptId;
      return 0;
    }
  }
  else
  {
    // create a bucket point list and insert the point
    bucket = vtkIdList::New();
    bucket->Allocate(this->NumberOfPointsPerBucket / 2, this->NumberOfPointsPerBucket / 3);
    this->HashTable[idx] = bucket;
  }

  // point has to be added
  bucket->InsertNextId(this->InsertionPointId);
  this->Points->InsertPoint(this->InsertionPointId, x);
  id = this->InsertionPointId++;

  return 1;
}

//------------------------------------------------------------------------------
void vtkMergePoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
