/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergePoints.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMergePoints.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkMergePoints, "1.35");
vtkStandardNewMacro(vtkMergePoints);

// Determine whether point given by x[3] has been inserted into points list.
// Return id of previously inserted point if this is true, otherwise return
// -1.
vtkIdType vtkMergePoints::IsInsertedPoint(const float x[3])
{
  int i, ijk0, ijk1, ijk2;
  vtkIdType idx;
  vtkIdList *bucket;
  //
  //  Locate bucket that point is in.
  //
  ijk0 = (int) ((float) ((x[0] - this->Bounds[0]) / 
           (this->Bounds[1] - this->Bounds[0])) * (this->Divisions[0] - 1));
  ijk1 = (int) ((float) ((x[1] - this->Bounds[2]) / 
           (this->Bounds[3] - this->Bounds[2])) * (this->Divisions[1] - 1));
  ijk2 = (int) ((float) ((x[2] - this->Bounds[4]) / 
           (this->Bounds[5] - this->Bounds[4])) * (this->Divisions[2] - 1));


  idx = ijk0 + ijk1*this->Divisions[0] + 
        ijk2*this->Divisions[0]*this->Divisions[1];

  bucket = this->HashTable[idx];

  if ( ! bucket )
    {
    return -1;
    }
  else // see whether we've got duplicate point
    {
    //
    // Check the list of points in that bucket.
    //
    vtkIdType ptId;
    float *pt;
    int NumberOfIds = bucket->GetNumberOfIds ();

    // For efficiency reasons, we break the data abstraction for points
    // and ids (we are assuming vtkPoints stores a FloatArray and vtkIdList
    // is storing ints).
    vtkFloatArray *dataArray = (vtkFloatArray *)this->Points->GetData();
    vtkIdType *idArray = bucket->GetPointer(0);

    for (i=0; i < NumberOfIds; i++) 
      {
      // These lines have been replaced by the two line following the comment.
      // The new code is much faster since it avoids dereferencing in the
      // inner loops.
      //ptId = bucket->GetId(i);
      //pt = this->Points->GetPoint(ptId);
      ptId = idArray[i];
      pt = dataArray->GetTuple(ptId);
      if ( x[0] == pt[0] && x[1] == pt[1] && x[2] == pt[2] )
        {
        return ptId;
        }
      }
    }

  return -1;
}


int
vtkMergePoints::InsertUniquePoint(const float x[3], vtkIdType &id)
{
  int i, ijk0, ijk1, ijk2;
  vtkIdType idx;
  vtkIdList *bucket;

  //
  //  Locate bucket that point is in.
  //
  ijk0 = (int) ((float) ((x[0] - this->Bounds[0]) / 
           (this->Bounds[1] - this->Bounds[0])) * (this->Divisions[0] - 1));
  ijk1 = (int) ((float) ((x[1] - this->Bounds[2]) / 
           (this->Bounds[3] - this->Bounds[2])) * (this->Divisions[1] - 1));
  ijk2 = (int) ((float) ((x[2] - this->Bounds[4]) / 
           (this->Bounds[5] - this->Bounds[4])) * (this->Divisions[2] - 1));

  idx = ijk0 + ijk1*this->Divisions[0] + 
        ijk2*this->Divisions[0]*this->Divisions[1];

  bucket = this->HashTable[idx];

  if (bucket) // see whether we've got duplicate point
    {
    //
    // Check the list of points in that bucket.
    //
    vtkIdType ptId;
    float *pt;
    int NumberOfIds = bucket->GetNumberOfIds ();

    // For efficiency reasons, we break the data abstraction for points
    // and ids (we are assuming vtkPoints stores a FloatArray and vtkIdList
    // is storing ints).
    vtkFloatArray *dataArray = (vtkFloatArray *)this->Points->GetData();
    vtkIdType *idArray = bucket->GetPointer(0);
    
    for (i=0; i < NumberOfIds; i++) 
      {
      // These lines have been replaced by the two line following the comment.
      // The new code is much faster since it avoids dereferencing in the
      // inner loops.
      //ptId = bucket->GetId(i);
      //pt = this->Points->GetPoint(ptId);
      ptId = idArray[i];
      pt = dataArray->GetTuple(ptId);
      if ( x[0] == pt[0] && x[1] == pt[1] && x[2] == pt[2] )
        {
        // point is already in the list, return 0 and set the id parameter
        id = ptId;
        return 0;
        }
      }
    }
  else
    {
    // create a bucket point list and insert the point
    bucket = vtkIdList::New();
    bucket->Allocate(this->NumberOfPointsPerBucket/2,
                     this->NumberOfPointsPerBucket/3);
    this->HashTable[idx] = bucket;
    }

  // point has to be added
  bucket->InsertNextId(this->InsertionPointId);
  this->Points->InsertPoint(this->InsertionPointId,x);
  id = this->InsertionPointId++;

  return 1;
}
