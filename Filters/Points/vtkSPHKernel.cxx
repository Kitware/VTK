/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSPHKernel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSPHKernel.h"
#include "vtkAbstractPointLocator.h"
#include "vtkObjectFactory.h"
#include "vtkIdList.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkDataArray.h"
#include "vtkMath.h"

vtkCxxSetObjectMacro(vtkSPHKernel,DensityArray,vtkDataArray);
vtkCxxSetObjectMacro(vtkSPHKernel,MassArray,vtkDataArray);

//----------------------------------------------------------------------------
vtkSPHKernel::vtkSPHKernel()
{
  this->SpatialStep = 0.001;
  this->Dimension = 3;
  this->RequiresInitialization = true;
  this->DensityArray = NULL;
  this->MassArray = NULL;
}

//----------------------------------------------------------------------------
vtkSPHKernel::~vtkSPHKernel()
{
  this->SetDensityArray(NULL);
  this->SetMassArray(NULL);
}

//----------------------------------------------------------------------------
void vtkSPHKernel::
Initialize(vtkAbstractPointLocator *loc, vtkDataSet *ds, vtkPointData *attr)
{
  // this->CutoffFactor should have been set by subclass
  this->Cutoff = this->CutoffFactor * this->SpatialStep;

  if ( this->Dimension == 1 )
    {
    this->FacW = (1.0/120.0) * pow(this->CutoffFactor/this->Cutoff,1);
    }
  else if ( this->Dimension == 2 )
    {
    this->FacW = (7.0/(478.0*vtkMath::Pi())) * pow(this->CutoffFactor/this->Cutoff,2);
    }
  else //if ( this->Dimension == 3 )
    {
    this->FacW = (1.0/(120.0*vtkMath::Pi())) * pow(this->CutoffFactor/this->Cutoff,3);
    }

  this->GradW = -this->CutoffFactor/this->Cutoff * 5.0*this->FacW;
  this->NormDist = 1.0 / this->SpatialStep;
  this->DefaultVolume = this->SpatialStep * this->SpatialStep * this->SpatialStep;

  // Make sure the density array is defined
  this->UseArraysForVolume = ((this->DensityArray && this->MassArray) ? true : false);

  this->Superclass::Initialize(loc, ds, attr);
}

//----------------------------------------------------------------------------
// Radius around point is 3 * smoothing length
vtkIdType vtkSPHKernel::
ComputeBasis(double x[3], vtkIdList *pIds)
{
  this->Locator->FindPointsWithinRadius(this->Cutoff, x, pIds);
  return pIds->GetNumberOfIds();
}

//----------------------------------------------------------------------------
void vtkSPHKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Spatial Step: " << this->SpatialStep << "\n";
}
