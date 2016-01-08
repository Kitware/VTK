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

vtkStandardNewMacro(vtkGaussianKernel);

//----------------------------------------------------------------------------
vtkGaussianKernel::vtkGaussianKernel()
{
  this->Radius = 1.0;
  this->Sharpness = 2.0;
}


//----------------------------------------------------------------------------
vtkGaussianKernel::~vtkGaussianKernel()
{
}


//----------------------------------------------------------------------------
vtkIdType vtkGaussianKernel::
ComputeWeights(double x[3], vtkIdList *pIds, vtkDoubleArray *weights)
{
  this->Locator->FindPointsWithinRadius(this->Radius, x, pIds);
  vtkIdType numPts = pIds->GetNumberOfIds();

  if ( numPts >= 1 ) //Use Gaussian kernel
    {
    int i;
    vtkIdType id;
    double d2, y[3], sum = 0.0;
    double h2 = (this->Radius*this->Radius) / (this->Sharpness*this->Sharpness);
    weights->SetNumberOfTuples(numPts);
    double *w = weights->GetPointer(0);

    for (i=0; i<numPts; ++i)
      {
      id = pIds->GetId(i);
      this->DataSet->GetPoint(id,y);
      d2 = vtkMath::Distance2BetweenPoints(x,y);

      if ( d2 == 0.0 ) //precise hit on existing point
        {
        pIds->SetNumberOfIds(1);
        pIds->SetId(0,id);
        weights->SetNumberOfTuples(1);
        weights->SetValue(0,1.0);
        return 1;
        }
      else
        {
        w[i] = exp(-(d2/h2));
        sum += w[i];
        }
      }//over all points

    // Normalize
    for (i=0; i<numPts; ++i)
      {
      w[i] /= sum;
      }

    return numPts;
    }//using Gaussian Kernel

  else //null point
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
void vtkGaussianKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}
