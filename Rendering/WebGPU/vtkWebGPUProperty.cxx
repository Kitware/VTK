/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGPUProperty.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
