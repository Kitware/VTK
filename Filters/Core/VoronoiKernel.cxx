/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoronoiKernel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVoronoiKernel.h"
#include "vtkAbstractPointLocator.h"

//----------------------------------------------------------------------------
vtkVoronoiKernel::vtkVoronoiKernel()
{
}


//----------------------------------------------------------------------------
vtkVoronoiKernel::~vtkVoronoiKernel()
{
}


//----------------------------------------------------------------------------
void vtkVoronoiKernel::
vtkIdType ComputeWeights(double x[3], vtkIdList *pIds,vtkDoubleArray *weights)
{
}


//----------------------------------------------------------------------------
void vtkVoronoiKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}
