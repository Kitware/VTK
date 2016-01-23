/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShepardKernel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkShepardKernel.h"
#include "vtkAbstractPointLocator.h"
#include "vtkObjectFactory.h"
#include "vtkIdList.h"
#include "vtkDoubleArray.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkShepardKernel);

//----------------------------------------------------------------------------
vtkShepardKernel::vtkShepardKernel()
{
  this->Radius = 1.0;
  this->PowerParameter = 2.0;
}


//----------------------------------------------------------------------------
vtkShepardKernel::~vtkShepardKernel()
{
}


//----------------------------------------------------------------------------
vtkIdType vtkShepardKernel::
ComputeWeights(double x[3], vtkIdList *pIds, vtkDoubleArray *weights)
{
  this->Locator->FindPointsWithinRadius(this->Radius, x, pIds);
  vtkIdType numPts = pIds->GetNumberOfIds();

  if ( numPts >= 1 ) //Use Shepard kernel
    {
    int i;
    vtkIdType id;
    double d, y[3], sum = 0.0;
    weights->SetNumberOfTuples(numPts);
    double *w = weights->GetPointer(0);

    for (i=0; i<numPts; ++i)
      {
      id = pIds->GetId(i);
      this->DataSet->GetPoint(id,y);
      if ( this->PowerParameter == 2.0 )
        {
        d = vtkMath::Distance2BetweenPoints(x,y);
        }
      else
        {
        d = pow(sqrt(vtkMath::Distance2BetweenPoints(x,y)), this->PowerParameter);
        }
      if ( d == 0.0 ) //precise hit on existing point
        {
        pIds->SetNumberOfIds(1);
        pIds->SetId(0,id);
        weights->SetNumberOfTuples(1);
        weights->SetValue(0,1.0);
        return 1;
        }
      else
        {
        w[i] = 1.0 / d;
        sum += w[i];
        }
      }//over all points

    // Normalize
    for (i=0; i<numPts; ++i)
      {
      w[i] /= sum;
      }
    return numPts;
    }//using Shepard Kernel

  else //hit null point
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
void vtkShepardKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Power Parameter: "
     << this->PowerParameter << "\n";
}
