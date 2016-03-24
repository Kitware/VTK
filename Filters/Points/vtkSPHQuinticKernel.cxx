/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSPHQuinticKernel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSPHQuinticKernel.h"
#include "vtkAbstractPointLocator.h"
#include "vtkObjectFactory.h"
#include "vtkIdList.h"
#include "vtkDoubleArray.h"
#include "vtkDataSet.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkSPHQuinticKernel);

//----------------------------------------------------------------------------
vtkSPHQuinticKernel::vtkSPHQuinticKernel()
{
  this->CutoffFactor = 3.0;
}

//----------------------------------------------------------------------------
vtkSPHQuinticKernel::~vtkSPHQuinticKernel()
{
}

//----------------------------------------------------------------------------
void vtkSPHQuinticKernel::
Initialize(vtkAbstractPointLocator *loc, vtkDataSet *ds, vtkPointData *attr)
{
  this->CutoffFactor = 3.0;
  this->Superclass::Initialize(loc,ds,attr);
}

//----------------------------------------------------------------------------
vtkIdType vtkSPHQuinticKernel::
ComputeWeights(double x[3], vtkIdList *pIds, vtkDoubleArray *weights)
{
  vtkIdType numPts = pIds->GetNumberOfIds();
  int i;
  vtkIdType id;
  double d, y[3];
  weights->SetNumberOfTuples(numPts);
  double *w = weights->GetPointer(0);
  double KW, den, mass;

  // Compute SPH coefficients. If requested, the summed Shepard weights
  // are computed as well.
  for (i=0; i<numPts; ++i)
    {
    id = pIds->GetId(i);
    this->DataSet->GetPoint(id,y);
    d = sqrt( vtkMath::Distance2BetweenPoints(x,y) );

    KW = vtkSPHQuinticKernel::ComputeFunctionWeight(d*this->NormDist);

    mass = 1.0;
    den = static_cast<double>(this->DensityArray->GetTuple1(id));
    w[i] = this->FacW * KW * (mass/den);
    }//over all neighbor points

  return numPts;
}

//----------------------------------------------------------------------------
vtkIdType vtkSPHQuinticKernel::
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
  double KW, GW, mass, den, mdF;

  for (i=0; i<numPts; ++i)
    {
    id = pIds->GetId(i);
    this->DataSet->GetPoint(id,y);
    d = sqrt( vtkMath::Distance2BetweenPoints(x,y) );

    KW = vtkSPHQuinticKernel::ComputeFunctionWeight(d*this->NormDist);
    GW = vtkSPHQuinticKernel::ComputeGradientWeight(d*this->NormDist);

    mass = 1.0;
    den = static_cast<double>(this->DensityArray->GetTuple1(id));
    mdF  = this->FacW * mass / den;
    w[i] = KW * mdF;
    gw[i] = GW * mdF;
    }//over all neighbor points

  return numPts;
}


//----------------------------------------------------------------------------
void vtkSPHQuinticKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Cutoff Factor: " << this->CutoffFactor << "\n";
}
