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
#include "vtkDataSet.h"
#include "vtkMath.h"

vtkCxxSetObjectMacro(vtkSPHKernel,DensityArray,vtkDataArray);
vtkCxxSetObjectMacro(vtkSPHKernel,MassArray,vtkDataArray);

//----------------------------------------------------------------------------
vtkSPHKernel::vtkSPHKernel()
{
  this->RequiresInitialization = true;
  this->SpatialStep = 0.001;
  this->Dimension = 3;
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
// At this point, the spatial step, the dimension of the kernel, the cutoff
// factor, and the sigma normalization factor should be known.
void vtkSPHKernel::
Initialize(vtkAbstractPointLocator *loc, vtkDataSet *ds, vtkPointData *attr)
{
  this->Superclass::Initialize(loc, ds, attr);

  // this->CutoffFactor should have been set by subclass
  this->Cutoff = this->CutoffFactor * this->SpatialStep;
  this->DistNorm = 1.0 / this->SpatialStep;
  this->DimNorm = this->Sigma * pow(this->DistNorm,this->Dimension);
  this->DefaultVolume = this->SpatialStep * this->SpatialStep * this->SpatialStep;

  // See if local mass and density information is provided
  this->UseArraysForVolume = ((this->DensityArray && this->MassArray) ? true : false);
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
vtkIdType vtkSPHKernel::
ComputeWeights(double x[3], vtkIdList *pIds, vtkDoubleArray *weights)
{
  vtkIdType numPts = pIds->GetNumberOfIds();
  int i;
  vtkIdType id;
  double d, y[3];
  weights->SetNumberOfTuples(numPts);
  double *w = weights->GetPointer(0);
  double KW, volume=this->DefaultVolume;

  // Compute SPH coefficients.
  for (i=0; i<numPts; ++i)
    {
    id = pIds->GetId(i);
    this->DataSet->GetPoint(id,y);
    d = sqrt( vtkMath::Distance2BetweenPoints(x,y) );

    KW = this->ComputeFunctionWeight(d*this->DistNorm);

    w[i] = this->DimNorm * KW * volume;
    }//over all neighbor points

  return numPts;
}

//----------------------------------------------------------------------------
vtkIdType vtkSPHKernel::
ComputeGradWeights(double x[3], vtkIdList *pIds, vtkDoubleArray *weights,
                   vtkDoubleArray *gradWeights)
{
  vtkIdType numPts = pIds->GetNumberOfIds();
  int i;
  vtkIdType id;
  double d, y[3];
  weights->SetNumberOfTuples(numPts);
  double *w = weights->GetPointer(0);
  gradWeights->SetNumberOfTuples(numPts);
  double *gw = gradWeights->GetPointer(0);
  double KW, GW, volume=this->DefaultVolume;

  // Compute SPH coefficients for data and deriative data
  for (i=0; i<numPts; ++i)
    {
    id = pIds->GetId(i);
    this->DataSet->GetPoint(id,y);
    d = sqrt( vtkMath::Distance2BetweenPoints(x,y) );

    KW = this->ComputeFunctionWeight(d*this->DistNorm);
    GW = this->ComputeGradientWeight(d*this->DistNorm);

    w[i] = this->DimNorm * KW * volume;
    gw[i] = this->DimNorm * GW * volume;
    }//over all neighbor points

  return numPts;
}

//----------------------------------------------------------------------------
void vtkSPHKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Spatial Step: " << this->SpatialStep << "\n";
  os << indent << "Dimension: " << this->Dimension << "\n";
  os << indent << "Cutoff Factor: " << this->CutoffFactor << "\n";
  os << indent << "Sigma: " << this->Sigma << "\n";
}
