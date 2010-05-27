/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include  "vtkReebGraphSimplificationMetric.h"

#include  "vtkInformation.h"
#include  "vtkInformationVector.h"

vtkCxxRevisionMacro(vtkReebGraphSimplificationMetric, "$Revision$");
vtkStandardNewMacro(vtkReebGraphSimplificationMetric);

//----------------------------------------------------------------------------
vtkReebGraphSimplificationMetric::vtkReebGraphSimplificationMetric()
{
  this->LowerBound = 0;
  this->UpperBound = 1;
}

//----------------------------------------------------------------------------
vtkReebGraphSimplificationMetric::~vtkReebGraphSimplificationMetric()
{
}

//----------------------------------------------------------------------------
void vtkReebGraphSimplificationMetric::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
double vtkReebGraphSimplificationMetric::ComputeMetric(
  vtkDataSet *mesh, vtkDataArray *scalarField, vtkIdType startCriticalPoint, 
  vtkAbstractArray *vertexList, vtkIdType endCriticalPoint)
{
  printf("too bad, wrong code\n");
  return 0;
}
