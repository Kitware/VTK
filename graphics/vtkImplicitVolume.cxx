/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitVolume.cxx
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
#include "vtkImplicitVolume.h"

static vtkPointData *aSamplePoint;
static vtkFloatScalars *aScalar;
static DataInitialized = 0;	// a flag to initialize the point data

// Description
// Construct an implicitVolume defined by a structured points dataset.
vtkImplicitVolume::vtkImplicitVolume()
{
  this->Volume = NULL;
  if (DataInitialized == 0)
    {
    aSamplePoint = new vtkPointData;
    aScalar = new vtkFloatScalars (8,1);
    aSamplePoint->InterpolateAllocate (aSamplePoint, 8, 1);
    aSamplePoint->SetScalars (aScalar);
    DataInitialized = 1;
    }
}

// Description
// Evaluate the ImplicitVolume. This returns the interpolated scalar value
// at x[3].
float vtkImplicitVolume::EvaluateFunction(float x[3])
{
  vtkPointData *pointData;
  vtkCell *cell;
  int subId;
  float pcoords[3], *weights=new float[this->Volume->GetMaxCellSize()];
  float scalar;

  // See if a volume is defined
  if (this->Volume == NULL)
    {
    vtkErrorMacro(<<"No volume defined!!");
    return -VTK_LARGE_FLOAT;
    }

  // Get the point data for the structured points.
  pointData = this->Volume->GetPointData ();

  // Find the cell that contains xyz and get it

  cell = this->Volume->FindAndGetCell(x,NULL,0.0,subId,pcoords,weights);
  if (cell)
    {
    // Interpolate the point data
    aSamplePoint->InterpolatePoint(pointData,0,&(cell->PointIds),weights);
    scalar = (aSamplePoint->GetScalars ())->GetScalar(0);
    }
  else
    {
    scalar = -VTK_LARGE_FLOAT;
    }

  delete [] weights;
  return scalar;
}

unsigned long vtkImplicitVolume::GetMTime()
{
  unsigned long mTime=this->vtkImplicitFunction::GetMTime();
  unsigned long volumeMTime;

  if ( this->Volume != NULL )
    {
    this->Volume->Update ();
    volumeMTime = this->Volume->GetMTime();
    mTime = ( volumeMTime > mTime ? volumeMTime : mTime );
    }

  return mTime;
}



// Description
// Evaluate ImplicitVolume gradient.
void vtkImplicitVolume::EvaluateGradient(float x[3], float n[3])
{
}

void vtkImplicitVolume::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImplicitFunction::PrintSelf(os,indent);
}
