/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergePoints.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkMergePoints.hh"

// Description:
// Merge points together if they are exactly coincident. Return a list 
// that maps unmerged point ids into new point ids. User is responsible 
// for freeing list (use delete []). Make sure you've provided a dataset
// before invoking this method.
int *vtkMergePoints::MergePoints()
{
  int ptId, i, j;
  int numPts;
  int *index;
  int newPtId;
  float *pt, *p;
  int ijk[3];
  int cno;
  vtkIdList *ptIds;

  vtkDebugMacro(<<"Merging points");

  if ( this->DataSet == NULL || 
  (numPts=this->DataSet->GetNumberOfPoints()) < 1 ) return NULL;

  this->BuildLocator(); // subdivides if necessary

  index = new int[numPts];
  for (i=0; i < numPts; i++) index[i] = -1;

  newPtId = 0; // renumbering points
//
//  Traverse each point, find bucket that point is in, check the list of
//  points in that bucket for merging.  Also need to search all
//  neighboring buckets within the tolerance.  The number and level of
//  neighbors to search depends upon the tolerance and the bucket width.
//
  for ( i=0; i < numPts; i++ ) //loop over all points
    {
    // Only try to merge the point if it hasn't yet been merged.
    if ( index[i] == -1 ) 
      {
      p = this->DataSet->GetPoint(i);
      index[i] = newPtId;

      for (j=0; j<3; j++) 
        ijk[j] = (int) ((float)((p[j] - this->Bounds[2*j])*0.999 / 
              (this->Bounds[2*j+1] - this->Bounds[2*j])) * this->Divisions[j]);

      cno = ijk[0] + ijk[1]*this->Divisions[0] + 
            ijk[2]*this->Divisions[0]*this->Divisions[1];

      if ( (ptIds = this->HashTable[cno]) != NULL )
        {
        for (j=0; j < ptIds->GetNumberOfIds(); j++) 
          {
          ptId = ptIds->GetId(j);
          pt = this->DataSet->GetPoint(ptId);

          if ( index[ptId] == -1 && pt[0] == p[0] && pt[1] == p[1]
          && pt[2] == p[2] )
            {
            index[ptId] = newPtId;
            }
          }
        }
      newPtId++;
      } // if point hasn't been merged
    } // for all points

  return index;
}

// Description:
// Determine whether point given by x[3] has been inserted into points list.
// Return id of previously inserted point if this is true, otherwise return
// -1.
int vtkMergePoints::IsInsertedPoint(float x[3])
{
  int i, ijk[3];
  int idx;
  vtkIdList *bucket;
//
//  Locate bucket that point is in.
//
  for (i=0; i<3; i++) 
    {
    ijk[i] = (int) ((float) ((x[i] - this->Bounds[2*i])*0.999 / 
             (this->Bounds[2*i+1] - this->Bounds[2*i])) * this->Divisions[i]);
    }
  idx = ijk[0] + ijk[1]*this->Divisions[0] + 
        ijk[2]*this->Divisions[0]*this->Divisions[1];

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

    for (i=0; i < bucket->GetNumberOfIds(); i++) 
      {
      ptId = bucket->GetId(i);
      pt = this->Points->GetPoint(ptId);
      if ( x[0] == pt[0] && x[1] == pt[1] && x[2] == pt[2] ) return ptId;
      }
    }

  return -1;
}

