/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergePoints2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkMergePoints2D.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkMergePoints2D* vtkMergePoints2D::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMergePoints2D");
  if(ret)
    {
    return (vtkMergePoints2D*)ret;
    }
  // If the factory was unable to create the object, then create it here.
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

