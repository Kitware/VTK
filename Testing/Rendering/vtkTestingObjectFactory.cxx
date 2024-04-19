// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTestingObjectFactory.h"
#include "vtkTestingInteractor.h"
#include "vtkVersion.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTestingObjectFactory);

VTK_CREATE_CREATE_FUNCTION(vtkTestingInteractor);

vtkTestingObjectFactory::vtkTestingObjectFactory()
{
  this->RegisterOverride("vtkRenderWindowInteractor", "vtkTestingInteractor",
    "Overrides for testing", 1, vtkObjectFactoryCreatevtkTestingInteractor);
}

const char* vtkTestingObjectFactory::GetVTKSourceVersion()
{
  return VTK_SOURCE_VERSION;
}

void vtkTestingObjectFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Description: " << this->GetDescription() << endl;
}
VTK_ABI_NAMESPACE_END
