/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitVolume.cxx
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
#include "vtkImplicitVolume.h"
#include "vtkVoxel.h"

// Description
// Construct an vtkImplicitVolume with no initial volume; the OutValue
// set to a large negative number; and the OutGradient set to (0,0,1).
vtkImplicitVolume::vtkImplicitVolume()
{
  this->Volume = NULL;
  this->OutValue = -VTK_LARGE_FLOAT;

  this->OutGradient[0] = 0.0;
  this->OutGradient[1] = 0.0;
  this->OutGradient[2] = 1.0;
}

// Description
// Evaluate the ImplicitVolume. This returns the interpolated scalar value
// at x[3].
float vtkImplicitVolume::EvaluateFunction(float x[3])
{
  vtkScalars *scalars;
  int i, ijk[3];
  int numPts;
  float pcoords[3], weights[8], s;
  static vtkIdList ptIds(8);

  // See if a volume is defined
  if ( !this->Volume ||
  !(scalars = this->Volume->GetPointData()->GetScalars()) )
    {
    vtkErrorMacro(<<"Can't evaluate volume!");
    return this->OutValue;
    }

  // Find the cell that contains xyz and get it
  if ( this->Volume->ComputeStructuredCoordinates(x,ijk,pcoords) )
    {
    this->Volume->GetCellPoints(this->Volume->ComputeCellId(ijk),ptIds);
    vtkVoxel::InterpolationFunctions(pcoords,weights);

    numPts = ptIds.GetNumberOfIds ();
    for (s=0.0, i=0; i < numPts; i++)
      {
      s += scalars->GetScalar(ptIds.GetId(i)) * weights[i];
      }
    return s;
    }

  else
    {
    return this->OutValue;
    }
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
  vtkScalars *scalars;
  int i, ijk[3];
  float pcoords[3], weights[8], *v;
  static vtkFloatVectors gradient(8);

  // See if a volume is defined
  if ( !this->Volume ||
  !(scalars = this->Volume->GetPointData()->GetScalars()) )
    {
    vtkErrorMacro(<<"Can't evaluate volume!");
    return;
    }

  // Find the cell that contains xyz and get it
  if ( this->Volume->ComputeStructuredCoordinates(x,ijk,pcoords) )
    {
    vtkVoxel::InterpolationFunctions(pcoords,weights);
    this->Volume->GetVoxelGradient(ijk[0], ijk[1], ijk[2], scalars, gradient);

    n[0] = n[1] = n[2] = 0.0;
    for (i=0; i < 8; i++)
      {
      v = gradient.GetVector(i);
      n[0] += v[0] * weights[i];
      n[1] += v[1] * weights[i];
      n[2] += v[2] * weights[i];
      }
    }

  else
    { // use outside value
    for ( i=0; i < 3; i++ ) n[i] = this->OutGradient[i];
    }
}

void vtkImplicitVolume::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImplicitFunction::PrintSelf(os,indent);

  os << indent << "Out Value: " << this->OutValue << "\n";
  os << indent << "Out Gradient: (" << this->OutGradient[0] << ", " 
     << this->OutGradient[1] << ", " << this->OutGradient[2] << ")\n";
}
