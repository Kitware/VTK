// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkReebGraphSimplificationMetric.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkReebGraphSimplificationMetric);

//------------------------------------------------------------------------------
vtkReebGraphSimplificationMetric::vtkReebGraphSimplificationMetric()
{
  this->LowerBound = 0;
  this->UpperBound = 1;
}

//------------------------------------------------------------------------------
vtkReebGraphSimplificationMetric::~vtkReebGraphSimplificationMetric() = default;

//------------------------------------------------------------------------------
void vtkReebGraphSimplificationMetric::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Upper Bound: " << this->UpperBound << "\n";
  os << indent << "Lower Bound: " << this->LowerBound << "\n";
}

//------------------------------------------------------------------------------
double vtkReebGraphSimplificationMetric::ComputeMetric(vtkDataSet* vtkNotUsed(mesh),
  vtkDataArray* vtkNotUsed(scalarField), vtkIdType vtkNotUsed(startCriticalPoint),
  vtkAbstractArray* vtkNotUsed(vertexList), vtkIdType vtkNotUsed(endCriticalPoint))
{
  printf("too bad, wrong code\n");
  return 0;
}
VTK_ABI_NAMESPACE_END
