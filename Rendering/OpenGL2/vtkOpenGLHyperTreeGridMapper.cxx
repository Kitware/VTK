// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLHyperTreeGridMapper.h"

#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositePolyDataMapper.h" // For Mapper3D
#include "vtkObjectFactory.h"           // For the macro
#include "vtkOpenGLPolyDataMapper.h"    // For PDMapper

VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkOpenGLHyperTreeGridMapper);

//------------------------------------------------------------------------------
vtkOpenGLHyperTreeGridMapper::vtkOpenGLHyperTreeGridMapper()
{
  this->Mapper = vtkSmartPointer<vtkCompositePolyDataMapper>::New();
  auto cpdm = vtkCompositePolyDataMapper::SafeDownCast(this->Mapper);
  vtkNew<vtkCompositeDataDisplayAttributes> compositeAttributes;
  cpdm->SetCompositeDataDisplayAttributes(compositeAttributes);
}

//------------------------------------------------------------------------------
void vtkOpenGLHyperTreeGridMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
