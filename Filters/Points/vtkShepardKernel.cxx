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
#include "vtkMathUtilities.h"

vtkStandardNewMacro(vtkShepardKernel);

//----------------------------------------------------------------------------
vtkShepardKernel::vtkShepardKernel()
{
  this->PowerParameter = 2.0;
}


//----------------------------------------------------------------------------
vtkShepardKernel::~vtkShepardKernel()
{
}


//----------------------------------------------------------------------------
vtkIdType vtkShepardKernel::
ComputeWeights(double x[3], vtkIdList *pIds, vtkDoubleArray *prob,
               vtkDoubleArray *weights)
{
  vtkIdType numPts = pIds->GetNumberOfIds();
  int i;
  vtkIdType id;
  double d, y[3], sum=0.0;
  weights->SetNumberOfTuples(numPts);
  double *p = (prob ? prob->GetPointer(0) : NULL);
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
    if ( vtkMathUtilities::FuzzyCompare(d, 0.0, std::numeric_limits<double>::epsilon()*256.0 )) //precise hit on existing point
    {
      pIds->SetNumberOfIds(1);
      pIds->SetId(0,id);
      weights->SetNumberOfTuples(1);
      weights->SetValue(0,1.0);
      return 1;
    }
    else
    {
      w[i] = (p ? p[i]/d : 1.0/d); //take into account probability if provided
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
void vtkShepardKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Power Parameter: "
     << this->GetPowerParameter() << "\n";
}
