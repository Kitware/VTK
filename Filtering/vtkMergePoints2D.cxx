/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergePoints2D.cxx
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
#include "vtkMergePoints2D.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkMergePoints2D, "1.11");

vtkMergePoints2D* vtkMergePoints2D::New() 
{ 
  vtkGenericWarningMacro("MergePoints2D is being deprecated in VTK 4.1. There is no replacement for it.");
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMergePoints2D"); 
  if(ret) 
      { 
      return static_cast<vtkMergePoints2D*>(ret); 
      } 
    return new vtkMergePoints2D; 
}

// Determine whether point given by x[2] has been inserted into points list.
// Return id of previously inserted point if this is true, otherwise return
// -1.
int vtkMergePoints2D::IsInsertedPoint(float x[2])
{
  int i, ijk0, ijk1;
  int idx;
  vtkIdList *bucket;
  //
  //  Locate bucket that point is in.
  //
  ijk0 = (int) ((float) ((x[0] - this->Bounds[0]) / 
           (this->Bounds[1] - this->Bounds[0])) * (this->Divisions[0] - 1));
  ijk1 = (int) ((float) ((x[1] - this->Bounds[2]) / 
           (this->Bounds[3] - this->Bounds[2])) * (this->Divisions[1] - 1));

  idx = ijk0 + ijk1*this->Divisions[0];

  bucket = this->HashTable[idx];

  //
  //  Make sure candidate point is in bounds.  If not, it is outside.
  //
  for (i=0; i<2; i++)
    {
    if ( x[i] < this->Bounds[2*i] || x[i] > this->Bounds[2*i+1] )
      {
      return -1;
      }
    }

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

    for (i=0; i < NumberOfIds; i++) 
      {
      ptId = bucket->GetId(i);
      pt = this->Points->GetPoint(ptId);
      if (x[0] == pt[0] && x[1] == pt[1])
        {
        return ptId;
        }
      }
    }

  return -1;
}

