/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinearKernel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLinearKernel.h"
#include "vtkAbstractPointLocator.h"
#include "vtkObjectFactory.h"
#include "vtkIdList.h"
#include "vtkDoubleArray.h"

vtkStandardNewMacro(vtkLinearKernel);

//----------------------------------------------------------------------------
vtkLinearKernel::vtkLinearKernel()
{
}


//----------------------------------------------------------------------------
vtkLinearKernel::~vtkLinearKernel()
{
}


//----------------------------------------------------------------------------
vtkIdType vtkLinearKernel::
ComputeWeights(double*, vtkIdList *pIds, vtkDoubleArray *prob,
               vtkDoubleArray *weights)
{
  vtkIdType numPts = pIds->GetNumberOfIds();
  double *p = (prob ? prob->GetPointer(0) : NULL);
  weights->SetNumberOfTuples(numPts);
  double *w = weights->GetPointer(0);
  double weight = 1.0 / static_cast<double>(numPts);

  if ( ! prob ) //standard linear interpolation
  {
    for (vtkIdType i=0; i < numPts; ++i)
    {
      w[i] = weight;
    }
  }

  else //weight by probability
  {
    double sum=0.0;
    for (vtkIdType i=0; i < numPts; ++i)
    {
      w[i] = weight * p[i];
      sum += w[i];
    }
    // Now normalize
    if ( this->NormalizeWeights && sum != 0.0 )
    {
      for (vtkIdType i=0; i < numPts; ++i)
      {
        w[i] /= sum;
      }
    }
  }

  return numPts;
}

//----------------------------------------------------------------------------
void vtkLinearKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
