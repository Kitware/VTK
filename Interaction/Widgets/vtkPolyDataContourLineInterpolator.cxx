// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPolyDataContourLineInterpolator.h"

#include "vtkContourRepresentation.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataCollection.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkPolyDataContourLineInterpolator::vtkPolyDataContourLineInterpolator()
{
  this->Polys = vtkPolyDataCollection::New();
}

//------------------------------------------------------------------------------
vtkPolyDataContourLineInterpolator::~vtkPolyDataContourLineInterpolator()
{
  this->Polys->Delete();
}

//------------------------------------------------------------------------------
void vtkPolyDataContourLineInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Polys: \n";
  this->Polys->PrintSelf(os, indent.GetNextIndent());
}
VTK_ABI_NAMESPACE_END
