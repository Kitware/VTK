/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitVolume.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImplicitVolume.h"

#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkVoxel.h"

vtkStandardNewMacro(vtkImplicitVolume);
vtkCxxSetObjectMacro(vtkImplicitVolume,Volume,vtkImageData);

//----------------------------------------------------------------------------
// Construct an vtkImplicitVolume with no initial volume; the OutValue
// set to a large negative number; and the OutGradient set to (0,0,1).
vtkImplicitVolume::vtkImplicitVolume()
{
  this->Volume = NULL;
  this->OutValue = VTK_DOUBLE_MIN;

  this->OutGradient[0] = 0.0;
  this->OutGradient[1] = 0.0;
  this->OutGradient[2] = 1.0;
  
  this->PointIds = vtkIdList::New();
  this->PointIds->Allocate(8);
}

//----------------------------------------------------------------------------
vtkImplicitVolume::~vtkImplicitVolume()
{
  if (this->Volume)
    {
    this->Volume->Delete();
    this->Volume = NULL;
    }
  this->PointIds->Delete();
}

//----------------------------------------------------------------------------
// Evaluate the ImplicitVolume. This returns the interpolated scalar value
// at x[3].
double vtkImplicitVolume::EvaluateFunction(double x[3])
{
  vtkDataArray *scalars;
  int ijk[3];
  vtkIdType numPts, i;
  double pcoords[3], weights[8], s;

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
    this->Volume->GetCellPoints(this->Volume->ComputeCellId(ijk),this->PointIds);
    vtkVoxel::InterpolationFunctions(pcoords,weights);

    numPts = this->PointIds->GetNumberOfIds ();
    for (s=0.0, i=0; i < numPts; i++)
      {
      s += scalars->GetComponent(this->PointIds->GetId(i),0) * weights[i];
      }
    return s;
    }
  else
    {
    return this->OutValue;
    }
}

//----------------------------------------------------------------------------
unsigned long vtkImplicitVolume::GetMTime()
{
  unsigned long mTime = this->vtkImplicitFunction::GetMTime();
  unsigned long volumeMTime;

  if ( this->Volume != NULL )
    {
    this->Volume->RequestExactExtentOn();
    this->Volume->UpdateInformation();
    this->Volume->SetUpdateExtentToWholeExtent();
    this->Volume->Update();
    volumeMTime = this->Volume->GetMTime();
    mTime = ( volumeMTime > mTime ? volumeMTime : mTime );
    }

  return mTime;
}


//----------------------------------------------------------------------------
// Evaluate ImplicitVolume gradient.
void vtkImplicitVolume::EvaluateGradient(double x[3], double n[3])
{
  vtkDataArray *scalars;
  int i, ijk[3];
  double pcoords[3], weights[8], *v;
  vtkDoubleArray *gradient; 
  
  gradient = vtkDoubleArray::New();
  gradient->SetNumberOfComponents(3);
  gradient->SetNumberOfTuples(8);

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
      v = gradient->GetTuple(i);
      n[0] += v[0] * weights[i];
      n[1] += v[1] * weights[i];
      n[2] += v[2] * weights[i];
      }
    }

  else
    { // use outside value
    for ( i=0; i < 3; i++ )
      {
      n[i] = this->OutGradient[i];
      }
    }
  gradient->Delete();
}

//----------------------------------------------------------------------------
void vtkImplicitVolume::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Out Value: " << this->OutValue << "\n";
  os << indent << "Out Gradient: (" << this->OutGradient[0] << ", " 
     << this->OutGradient[1] << ", " << this->OutGradient[2] << ")\n";

  if ( this->Volume )
    {
    os << indent << "Volume: " << this->Volume << "\n";
    }
  else
    {
    os << indent << "Volume: (none)\n";
    }
}
