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

vtkCxxSetObjectMacro(vtkSPHKernel,CutoffArray,vtkDataArray);
vtkCxxSetObjectMacro(vtkSPHKernel,DensityArray,vtkDataArray);
vtkCxxSetObjectMacro(vtkSPHKernel,MassArray,vtkDataArray);

//----------------------------------------------------------------------------
vtkSPHKernel::vtkSPHKernel()
{
  this->RequiresInitialization = true;
  this->SpatialStep = 0.001;
  this->Dimension = 3;
  this->CutoffArray = NULL;
  this->DensityArray = NULL;
  this->MassArray = NULL;
}

//----------------------------------------------------------------------------
vtkSPHKernel::~vtkSPHKernel()
{
  this->SetCutoffArray(NULL);
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
  this->NormFactor = this->Sigma * pow(this->DistNorm,this->Dimension);
  this->DefaultVolume = pow(this->SpatialStep,this->Dimension);

  // See if cutoff array is provided.
  if ( this->CutoffArray && this->CutoffArray->GetNumberOfComponents() == 1 )
  {
    this->UseCutoffArray = true;
  }
  else
  {
    this->UseCutoffArray = false;
  }

  // See if local mass and density information is provided
  if ( this->DensityArray && this->MassArray &&
       this->DensityArray->GetNumberOfComponents() == 1 &&
       this->MassArray->GetNumberOfComponents() == 1 )
  {
    this->UseArraysForVolume = true;
  }
  else
  {
    this->UseArraysForVolume = false;
  }
}

//----------------------------------------------------------------------------
// Radius around point is cutoff factor * smoothing length. That is unless
// cutoff array is provided.
vtkIdType vtkSPHKernel::
ComputeBasis(double x[3], vtkIdList *pIds, vtkIdType ptId)
{
  double cutoff;
  if ( this->UseCutoffArray )
  {
    this->CutoffArray->GetTuple(ptId,&cutoff);
  }
  else
  {
    cutoff = this->Cutoff;
  }

  this->Locator->FindPointsWithinRadius(cutoff, x, pIds);
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
  double KW, mass, density, volume;

  // Compute SPH coefficients.
  for (i=0; i<numPts; ++i)
  {
    id = pIds->GetId(i);
    this->DataSet->GetPoint(id,y);
    d = sqrt( vtkMath::Distance2BetweenPoints(x,y) );

    KW = this->ComputeFunctionWeight(d*this->DistNorm);

    if ( this->UseArraysForVolume )
    {
      this->MassArray->GetTuple(id,&mass);
      this->DensityArray->GetTuple(id,&density);
      volume = mass /density;
    }
    else
    {
      volume = this->DefaultVolume;
    }

    w[i] = this->NormFactor * KW * volume;
  }//over all neighbor points

  return numPts;
}

//----------------------------------------------------------------------------
vtkIdType vtkSPHKernel::
ComputeDerivWeights(double x[3], vtkIdList *pIds, vtkDoubleArray *weights,
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
    GW = this->ComputeDerivWeight(d*this->DistNorm);

    w[i] = this->NormFactor * KW * volume;
    gw[i] = this->NormFactor * GW * volume;
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

  os << indent << "Cutoff Array: " << this->CutoffArray<< "\n";
  os << indent << "Density Array: " << this->DensityArray << "\n";
  os << indent << "Mass Array: " << this->MassArray << "\n";
}
