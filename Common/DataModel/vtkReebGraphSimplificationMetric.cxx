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
#include "vtkReebGraphSimplificationMetric.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

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
  os << indent << "Upper Bound: " << this->UpperBound << "\n";
  os << indent << "Lower Bound: " << this->LowerBound << "\n";
}

//----------------------------------------------------------------------------
double vtkReebGraphSimplificationMetric::ComputeMetric(vtkDataSet* vtkNotUsed(mesh), vtkDataArray* vtkNotUsed(scalarField), vtkIdType vtkNotUsed(startCriticalPoint), vtkAbstractArray* vtkNotUsed(vertexList), vtkIdType vtkNotUsed(endCriticalPoint))
{
  printf("too bad, wrong code\n");
  return 0;
}
