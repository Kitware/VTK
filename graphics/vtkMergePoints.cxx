/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergePoints.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkMergePoints.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkMergePoints* vtkMergePoints::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMergePoints");
  if(ret)
    {
    return (vtkMergePoints*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMergePoints;
}




// Determine whether point given by x[3] has been inserted into points list.
// Return id of previously inserted point if this is true, otherwise return
// -1.
int vtkMergePoints::IsInsertedPoint(const float x[3])
{
  int i, ijk0, ijk1, ijk2;
  int idx;
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
    int ptId;
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
  int idx;
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
    int ptId;
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
