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
  this->Radius = 1.0;
}


//----------------------------------------------------------------------------
vtkLinearKernel::~vtkLinearKernel()
{
}


//----------------------------------------------------------------------------
vtkIdType vtkLinearKernel::
ComputeBasis(double x[3], vtkIdList *pIds)
{
  this->Locator->FindPointsWithinRadius(this->Radius, x, pIds);
  return pIds->GetNumberOfIds();
}

//----------------------------------------------------------------------------
vtkIdType vtkLinearKernel::
ComputeWeights(double*, vtkIdList *pIds, vtkDoubleArray *weights)
{
  vtkIdType numPts = pIds->GetNumberOfIds();
  double w = 1.0 / static_cast<double>(numPts);

  weights->SetNumberOfTuples(numPts);
  for (vtkIdType i=0; i < numPts; ++i)
    {
    weights->SetValue(i,w);
    }

  return numPts;
}

//----------------------------------------------------------------------------
void vtkLinearKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}
