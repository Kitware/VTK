/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTexturedActor2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTexturedActor2D.h"

#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"

vtkStandardNewMacro(vtkTexturedActor2D);

vtkCxxSetObjectMacro(vtkTexturedActor2D, Texture, vtkTexture);

//-----------------------------------------------------------------------------
vtkTexturedActor2D::vtkTexturedActor2D()
{
  this->Texture = 0;
}

//-----------------------------------------------------------------------------
vtkTexturedActor2D::~vtkTexturedActor2D()
{
  this->SetTexture(0);
}

//-----------------------------------------------------------------------------
void vtkTexturedActor2D::ReleaseGraphicsResources(vtkWindow* win)
{
  this->Superclass::ReleaseGraphicsResources(win);

  // Pass this information to the texture.
  if (this->Texture)
    {
    this->Texture->ReleaseGraphicsResources(win);
    }
}

//-----------------------------------------------------------------------------
int vtkTexturedActor2D::RenderOverlay(vtkViewport* viewport)
{
  // Render the texture.
  vtkRenderer* ren = vtkRenderer::SafeDownCast(viewport);
  if (this->Texture)
    {
    this->Texture->Render(ren);
    }
  int result=this->Superclass::RenderOverlay(viewport);
  if (this->Texture)
    {
    this->Texture->PostRender(ren);
    }
   return result;
}

//-----------------------------------------------------------------------------
int vtkTexturedActor2D::RenderOpaqueGeometry(vtkViewport* viewport)
{
  // Render the texture.
  vtkRenderer* ren = vtkRenderer::SafeDownCast(viewport);
  if (this->Texture)
    {
    this->Texture->Render(ren);
    }
  int result=this->Superclass::RenderOpaqueGeometry(viewport);
  if (this->Texture)
    {
    this->Texture->PostRender(ren);
    }
  return result;
}

//-----------------------------------------------------------------------------
int vtkTexturedActor2D::RenderTranslucentPolygonalGeometry(
  vtkViewport* viewport)
{
  // Render the texture.
  vtkRenderer* ren = vtkRenderer::SafeDownCast(viewport);
  if (this->Texture)
    {
    this->Texture->Render(ren);
    }
  int result=this->Superclass::RenderTranslucentPolygonalGeometry(viewport);
  if (this->Texture)
    {
    this->Texture->PostRender(ren);
    }
  return result;
}

//-----------------------------------------------------------------------------
unsigned long int vtkTexturedActor2D::GetMTime()
{
  unsigned long int mTime = vtkActor2D::GetMTime();
  unsigned long int time;
  if (this->Texture)
    {
    time = this->Texture->GetMTime();
    mTime = (time > mTime ? time : mTime);
    }
  return mTime;
}

//-----------------------------------------------------------------------------
void vtkTexturedActor2D::ShallowCopy(vtkProp* prop)
{
  vtkTexturedActor2D* a = vtkTexturedActor2D::SafeDownCast(prop);
  if (a)
    {
    this->SetTexture(a->GetTexture());
    }

  // Now do superclass.
  this->Superclass::ShallowCopy(prop);
}

//-----------------------------------------------------------------------------
void vtkTexturedActor2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Texture: " << (this->Texture ? "" : "(none)") << endl;
  if (this->Texture)
    {
    this->Texture->PrintSelf(os, indent.GetNextIndent());
    }
}

