/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProperty.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLRenderer.h"

#include "vtkOpenGLHelper.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLTexture.h"
#include "vtkTexture.h"

#include "vtkOpenGLError.h"

#include <cassert>

vtkStandardNewMacro(vtkOpenGLProperty);

vtkOpenGLProperty::vtkOpenGLProperty() = default;

vtkOpenGLProperty::~vtkOpenGLProperty() = default;

// ----------------------------------------------------------------------------
// Implement base class method.
void vtkOpenGLProperty::Render(vtkActor* anActor, vtkRenderer* ren)
{
  // turn on/off backface culling
  vtkOpenGLState* ostate = static_cast<vtkOpenGLRenderer*>(ren)->GetState();
  if (!this->BackfaceCulling && !this->FrontfaceCulling)
  {
    ostate->vtkglDisable(GL_CULL_FACE);
  }
  else if (this->BackfaceCulling)
  {
    ostate->vtkglCullFace(GL_BACK);
    ostate->vtkglEnable(GL_CULL_FACE);
  }
  else // if both front & back culling on, will fall into backface culling
  {    // if you really want both front and back, use the Actor's visibility flag
    ostate->vtkglCullFace(GL_FRONT);
    ostate->vtkglEnable(GL_CULL_FACE);
  }

  this->RenderTextures(anActor, ren);
  this->Superclass::Render(anActor, ren);
}

//-----------------------------------------------------------------------------
bool vtkOpenGLProperty::RenderTextures(vtkActor*, vtkRenderer* ren)
{
  // render any textures.
  auto textures = this->GetAllTextures();
  for (auto ti : textures)
  {
    ti.second->Render(ren);
  }

  vtkOpenGLCheckErrorMacro("failed after Render");

  return (!textures.empty());
}

//-----------------------------------------------------------------------------
void vtkOpenGLProperty::PostRender(vtkActor* actor, vtkRenderer* renderer)
{
  vtkOpenGLClearErrorMacro();

  // Reset the face culling now we are done, leaking into text actor etc.
  if (this->BackfaceCulling || this->FrontfaceCulling)
  {
    static_cast<vtkOpenGLRenderer*>(renderer)->GetState()->vtkglDisable(GL_CULL_FACE);
  }

  // deactivate any textures.
  auto textures = this->GetAllTextures();
  for (auto ti : textures)
  {
    ti.second->PostRender(renderer);
  }

  this->Superclass::PostRender(actor, renderer);

  vtkOpenGLCheckErrorMacro("failed after PostRender");
}

//-----------------------------------------------------------------------------
// Implement base class method.
void vtkOpenGLProperty::BackfaceRender(vtkActor* vtkNotUsed(anActor), vtkRenderer* vtkNotUsed(ren))
{
}

//-----------------------------------------------------------------------------
void vtkOpenGLProperty::ReleaseGraphicsResources(vtkWindow* win)
{
  // release any textures.
  auto textures = this->GetAllTextures();
  for (auto ti : textures)
  {
    ti.second->ReleaseGraphicsResources(win);
  }

  this->Superclass::ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------------
void vtkOpenGLProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
