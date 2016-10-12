/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGaussianKernel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGaussianKernel.h"
#include "vtkAbstractPointLocator.h"
#include "vtkObjectFactory.h"
#include "vtkIdList.h"
#include "vtkDoubleArray.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"

vtkStandardNewMacro(vtkGaussianKernel);

//----------------------------------------------------------------------------
vtkGaussianKernel::vtkGaussianKernel()
{
  this->Sharpness = 2.0;
  this->F2 = this->Sharpness / this->Radius;
}


//----------------------------------------------------------------------------
vtkGaussianKernel::~vtkGaussianKernel()
{
}

//----------------------------------------------------------------------------
void vtkGaussianKernel::
Initialize(vtkAbstractPointLocator *loc, vtkDataSet *ds, vtkPointData *pd)
{
  this->Superclass::Initialize(loc, ds, pd);

  this->F2 = this->Sharpness / this->Radius;
  this->F2 = this->F2 * this->F2;
}

//----------------------------------------------------------------------------
vtkIdType vtkGaussianKernel::
ComputeWeights(double x[3], vtkIdList *pIds, vtkDoubleArray *prob,
               vtkDoubleArray *weights)
{
  vtkIdType numPts = pIds->GetNumberOfIds();
  int i;
  vtkIdType id;
  double d2, y[3], sum = 0.0;
  weights->SetNumberOfTuples(numPts);
  double *p = (prob ? prob->GetPointer(0) : NULL);
  double *w = weights->GetPointer(0);
  double f2=this->F2;

  for (i=0; i<numPts; ++i)
  {
    id = pIds->GetId(i);
    this->DataSet->GetPoint(id,y);
    d2 = vtkMath::Distance2BetweenPoints(x,y);

    if ( vtkMathUtilities::FuzzyCompare(d2, 0.0, std::numeric_limits<double>::epsilon()*256.0 )) //precise hit on existing point
    {
      pIds->SetNumberOfIds(1);
      pIds->SetId(0,id);
      weights->SetNumberOfTuples(1);
      weights->SetValue(0,1.0);
      return 1;
    }
    else
    {
      w[i] = (p ? p[i]*exp(-f2 * d2) : exp(-f2 * d2));
      sum += w[i];
    }
  }//over all points

  // Normalize
  if ( this->NormalizeWeights && sum != 0.0 )
  {
    for (i=0; i<numPts; ++i)
    {
      w[i] /= sum;
    }
  }

  return numPts;
}

//----------------------------------------------------------------------------
void vtkGaussianKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Sharpness: " << this->GetSharpness() << endl;
}
