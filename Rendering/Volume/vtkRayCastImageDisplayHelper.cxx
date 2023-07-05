// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkRayCastImageDisplayHelper.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
// Return nullptr if no override is supplied.
VTK_ABI_NAMESPACE_BEGIN
vtkAbstractObjectFactoryNewMacro(vtkRayCastImageDisplayHelper);
//------------------------------------------------------------------------------

// Construct a new vtkRayCastImageDisplayHelper with default values
vtkRayCastImageDisplayHelper::vtkRayCastImageDisplayHelper()
{
  this->PreMultipliedColors = 1;
  this->PixelScale = 1.0;
}

// Destruct a vtkRayCastImageDisplayHelper - clean up any memory used
vtkRayCastImageDisplayHelper::~vtkRayCastImageDisplayHelper() = default;

void vtkRayCastImageDisplayHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PreMultiplied Colors: " << (this->PreMultipliedColors ? "On" : "Off") << endl;

  os << indent << "Pixel Scale: " << this->PixelScale << endl;
}
VTK_ABI_NAMESPACE_END
