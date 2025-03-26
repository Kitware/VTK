// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUProperty.h"
#include "vtkWebGPURenderer.h"

#include "vtkObjectFactory.h"
#include "vtkTexture.h"

#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkWebGPUProperty);

vtkWebGPUProperty::vtkWebGPUProperty() = default;

vtkWebGPUProperty::~vtkWebGPUProperty() = default;

//------------------------------------------------------------------------------
// Implement base class method.
void vtkWebGPUProperty::Render(vtkActor*, vtkRenderer*) {}

//------------------------------------------------------------------------------
bool vtkWebGPUProperty::RenderTextures(vtkActor*, vtkRenderer*)
{
  return false;
}

//------------------------------------------------------------------------------
void vtkWebGPUProperty::PostRender(vtkActor*, vtkRenderer*) {}

//------------------------------------------------------------------------------
// Implement base class method.
void vtkWebGPUProperty::BackfaceRender(vtkActor* vtkNotUsed(anActor), vtkRenderer* vtkNotUsed(ren))
{
}

//------------------------------------------------------------------------------
void vtkWebGPUProperty::ReleaseGraphicsResources(vtkWindow*) {}

//------------------------------------------------------------------------------
void vtkWebGPUProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
