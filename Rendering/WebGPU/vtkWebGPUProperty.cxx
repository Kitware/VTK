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
void vtkWebGPUProperty::Render(vtkActor* anActor, vtkRenderer* ren) {}

//------------------------------------------------------------------------------
bool vtkWebGPUProperty::RenderTextures(vtkActor*, vtkRenderer* ren) {}

//------------------------------------------------------------------------------
void vtkWebGPUProperty::PostRender(vtkActor* actor, vtkRenderer* renderer) {}

//------------------------------------------------------------------------------
// Implement base class method.
void vtkWebGPUProperty::BackfaceRender(vtkActor* vtkNotUsed(anActor), vtkRenderer* vtkNotUsed(ren))
{
}

//------------------------------------------------------------------------------
void vtkWebGPUProperty::ReleaseGraphicsResources(vtkWindow* win) {}

//------------------------------------------------------------------------------
void vtkWebGPUProperty::PrintSelf(ostream& os, vtkIndent indent) {}
VTK_ABI_NAMESPACE_END
