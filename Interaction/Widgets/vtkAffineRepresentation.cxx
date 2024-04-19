// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAffineRepresentation.h"
#include "vtkObjectFactory.h"
#include "vtkTransform.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkAffineRepresentation::vtkAffineRepresentation()
{
  this->InteractionState = vtkAffineRepresentation::Outside;
  this->Tolerance = 15;
  this->Transform = vtkTransform::New();
}

//------------------------------------------------------------------------------
vtkAffineRepresentation::~vtkAffineRepresentation()
{
  this->Transform->Delete();
}

//------------------------------------------------------------------------------
void vtkAffineRepresentation::ShallowCopy(vtkProp* prop)
{
  vtkAffineRepresentation* rep = vtkAffineRepresentation::SafeDownCast(prop);
  if (rep)
  {
    this->SetTolerance(rep->GetTolerance());
  }
  this->Superclass::ShallowCopy(prop);
}

//------------------------------------------------------------------------------
void vtkAffineRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  // Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Tolerance: " << this->Tolerance << "\n";
}
VTK_ABI_NAMESPACE_END
