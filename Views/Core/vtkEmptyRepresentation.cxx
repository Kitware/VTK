// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkEmptyRepresentation.h"

#include "vtkAnnotationLink.h"
#include "vtkConvertSelectionDomain.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkEmptyRepresentation);

vtkEmptyRepresentation::vtkEmptyRepresentation()
{
  this->ConvertDomains = vtkSmartPointer<vtkConvertSelectionDomain>::New();

  this->SetNumberOfInputPorts(0);
}

vtkEmptyRepresentation::~vtkEmptyRepresentation() = default;

//------------------------------------------------------------------------------
vtkAlgorithmOutput* vtkEmptyRepresentation::GetInternalAnnotationOutputPort(
  int vtkNotUsed(port), int vtkNotUsed(conn))
{
  this->ConvertDomains->SetInputConnection(0, this->GetAnnotationLink()->GetOutputPort(0));
  this->ConvertDomains->SetInputConnection(1, this->GetAnnotationLink()->GetOutputPort(1));

  return this->ConvertDomains->GetOutputPort();
}

void vtkEmptyRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
