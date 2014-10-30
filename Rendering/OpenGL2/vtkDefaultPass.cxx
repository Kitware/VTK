/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDefaultPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDefaultPass.h"
#include "vtkObjectFactory.h"
#include <cassert>
#include "vtkRenderState.h"
#include "vtkProp.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkDefaultPass);

// ----------------------------------------------------------------------------
vtkDefaultPass::vtkDefaultPass()
{
}

// ----------------------------------------------------------------------------
vtkDefaultPass::~vtkDefaultPass()
{
}

// ----------------------------------------------------------------------------
void vtkDefaultPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkDefaultPass::Render(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  this->NumberOfRenderedProps=0;
  this->RenderOpaqueGeometry(s);
  this->RenderTranslucentPolygonalGeometry(s);
  this->RenderVolumetricGeometry(s);
  this->RenderOverlay(s);
}

// ----------------------------------------------------------------------------
// Description:
// Opaque pass without key checking.
// \pre s_exists: s!=0
void vtkDefaultPass::RenderOpaqueGeometry(const vtkRenderState *s)
{
  assert("pre s_exits" && s!=0);

  // initialize to false
  this->SetLastRenderingUsedDepthPeeling(s->GetRenderer(), false);

  int c=s->GetPropArrayCount();
  int i=0;
  while(i<c)
    {
    int rendered=s->GetPropArray()[i]->RenderOpaqueGeometry(s->GetRenderer());
    this->NumberOfRenderedProps+=rendered;
    ++i;
    }
}

// ----------------------------------------------------------------------------
// Description:
// Opaque pass with key checking.
// \pre s_exists: s!=0
void vtkDefaultPass::RenderFilteredOpaqueGeometry(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  // initialize to false
  this->SetLastRenderingUsedDepthPeeling(s->GetRenderer(), false);

  int c=s->GetPropArrayCount();
  int i=0;
  while(i<c)
    {
    vtkProp *p=s->GetPropArray()[i];
    if(p->HasKeys(s->GetRequiredKeys()))
      {
      int rendered=
        p->RenderFilteredOpaqueGeometry(s->GetRenderer(),s->GetRequiredKeys());
      this->NumberOfRenderedProps+=rendered;
      }
    ++i;
    }
}

// ----------------------------------------------------------------------------
// Description:
// Translucent pass without key checking.
// \pre s_exists: s!=0
void vtkDefaultPass::RenderTranslucentPolygonalGeometry(
  const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  int c=s->GetPropArrayCount();
  int i=0;
  while(i<c)
    {
    vtkProp *p=s->GetPropArray()[i];
    int rendered=p->RenderTranslucentPolygonalGeometry(s->GetRenderer());
    this->NumberOfRenderedProps+=rendered;
    ++i;
    }
}

// ----------------------------------------------------------------------------
// Description:
// Translucent pass with key checking.
// \pre s_exists: s!=0
void vtkDefaultPass::RenderFilteredTranslucentPolygonalGeometry(
  const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  int c=s->GetPropArrayCount();
  int i=0;
  while(i<c)
    {
    vtkProp *p=s->GetPropArray()[i];
    if(p->HasKeys(s->GetRequiredKeys()))
      {
      int rendered=p->RenderFilteredTranslucentPolygonalGeometry(s->GetRenderer(),s->GetRequiredKeys());
      this->NumberOfRenderedProps+=rendered;
      }
    ++i;
    }
}

// ----------------------------------------------------------------------------
// Description:
// Volume pass without key checking.
// \pre s_exists: s!=0
void vtkDefaultPass::RenderVolumetricGeometry(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  int c=s->GetPropArrayCount();
  int i=0;
  while(i<c)
    {
    int rendered=
      s->GetPropArray()[i]->RenderVolumetricGeometry(s->GetRenderer());
    this->NumberOfRenderedProps+=rendered;
    ++i;
    }
}

// ----------------------------------------------------------------------------
// Description:
// Translucent pass with key checking.
// \pre s_exists: s!=0
void vtkDefaultPass::RenderFilteredVolumetricGeometry(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  int c=s->GetPropArrayCount();
  int i=0;
  while(i<c)
    {
    vtkProp *p=s->GetPropArray()[i];
    if(p->HasKeys(s->GetRequiredKeys()))
      {
      int rendered=
        p->RenderFilteredVolumetricGeometry(s->GetRenderer(),
                                            s->GetRequiredKeys());
      this->NumberOfRenderedProps+=rendered;
      }
    ++i;
    }
}

// ----------------------------------------------------------------------------
// Description:
// Overlay pass without key checking.
// \pre s_exists: s!=0
void vtkDefaultPass::RenderOverlay(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  int c=s->GetPropArrayCount();
  int i=0;
  while(i<c)
    {
    int rendered=s->GetPropArray()[i]->RenderOverlay(s->GetRenderer());
    this->NumberOfRenderedProps+=rendered;
    ++i;
    }
}

// ----------------------------------------------------------------------------
// Description:
// Overlay pass with key checking.
// \pre s_exists: s!=0
void vtkDefaultPass::RenderFilteredOverlay(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  int c=s->GetPropArrayCount();
  int i=0;
  while(i<c)
    {
    vtkProp *p=s->GetPropArray()[i];
    if(p->HasKeys(s->GetRequiredKeys()))
      {
      int rendered=
        p->RenderFilteredOverlay(s->GetRenderer(),s->GetRequiredKeys());
      this->NumberOfRenderedProps+=rendered;
      }
    ++i;
    }
}
