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
vtkCxxSetObjectMacro(vtkSPHKernel,ShepardSumArray,vtkFloatArray);

//----------------------------------------------------------------------------
vtkSPHKernel::vtkSPHKernel()
{
  this->SpatialStep = 0.1;
  this->Dimension = 3;
  this->RequiresInitialization = true;
  this->DensityArray = NULL;
  this->MassArray = NULL;
  this->ShepardSumArray = NULL;
  this->ShepardSum = NULL;
}

//----------------------------------------------------------------------------
vtkSPHKernel::~vtkSPHKernel()
{
  this->SetDensityArray(NULL);
  this->SetMassArray(NULL);
  this->SetShepardSumArray(NULL);
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
  this->H = this->Cutoff / this->CutoffFactor;
  this->NormDist = 1.0 / this->H;

  // Make sure the density array is defined
  if ( this->DensityArray == NULL )
    {
    vtkErrorMacro("Density array required but not specified");
    return;
    }
  if ( this->MassArray == NULL )
    {
    // compute mass from local volume
    }
  if ( this->ShepardSumArray )
    {
    this->ShepardSum = this->ShepardSumArray->GetPointer(0);
    }

  this->Superclass::Initialize(loc, ds, attr);
}

//----------------------------------------------------------------------------
// Radius around point is 3 * smoothing length
vtkIdType vtkSPHKernel::
ComputeBasis(double x[3], vtkIdList *pIds)
{
  this->Locator->FindPointsWithinRadius(3.0*this->H, x, pIds);
  return pIds->GetNumberOfIds();
}

//----------------------------------------------------------------------------
void vtkSPHKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}
