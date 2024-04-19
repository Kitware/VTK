// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGraphGeodesicPath.h"

#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkGraphGeodesicPath::vtkGraphGeodesicPath()
{
  this->StartVertex = 0;
  this->EndVertex = 0;
}

//------------------------------------------------------------------------------
vtkGraphGeodesicPath::~vtkGraphGeodesicPath() = default;

//------------------------------------------------------------------------------
void vtkGraphGeodesicPath::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "StartVertex: " << this->StartVertex << endl;
  os << indent << "EndVertex: " << this->EndVertex << endl;
}
VTK_ABI_NAMESPACE_END
