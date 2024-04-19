// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDefaultPass.h"
#include "vtkObjectFactory.h"
#include "vtkProp.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDefaultPass);

//------------------------------------------------------------------------------
vtkDefaultPass::vtkDefaultPass() = default;

//------------------------------------------------------------------------------
vtkDefaultPass::~vtkDefaultPass() = default;

//------------------------------------------------------------------------------
void vtkDefaultPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkDefaultPass::Render(const vtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  this->NumberOfRenderedProps = 0;
  this->RenderOpaqueGeometry(s);
  this->RenderTranslucentPolygonalGeometry(s);
  this->RenderVolumetricGeometry(s);
  this->RenderOverlay(s);
}

//------------------------------------------------------------------------------
// Description:
// Opaque pass without key checking.
// \pre s_exists: s!=0
void vtkDefaultPass::RenderOpaqueGeometry(const vtkRenderState* s)
{
  assert("pre s_exits" && s != nullptr);

  int c = s->GetPropArrayCount();
  int i = 0;
  while (i < c)
  {
    int rendered = s->GetPropArray()[i]->RenderOpaqueGeometry(s->GetRenderer());
    this->NumberOfRenderedProps += rendered;
    ++i;
  }
}

//------------------------------------------------------------------------------
// Description:
// Opaque pass with key checking.
// \pre s_exists: s!=0
void vtkDefaultPass::RenderFilteredOpaqueGeometry(const vtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  int c = s->GetPropArrayCount();
  int i = 0;
  while (i < c)
  {
    vtkProp* p = s->GetPropArray()[i];
    if (p->HasKeys(s->GetRequiredKeys()))
    {
      int rendered = p->RenderFilteredOpaqueGeometry(s->GetRenderer(), s->GetRequiredKeys());
      this->NumberOfRenderedProps += rendered;
    }
    ++i;
  }
}

//------------------------------------------------------------------------------
// Description:
// Translucent pass without key checking.
// \pre s_exists: s!=0
void vtkDefaultPass::RenderTranslucentPolygonalGeometry(const vtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  int c = s->GetPropArrayCount();
  int i = 0;
  while (i < c)
  {
    vtkProp* p = s->GetPropArray()[i];
    int rendered = p->RenderTranslucentPolygonalGeometry(s->GetRenderer());
    this->NumberOfRenderedProps += rendered;
    ++i;
  }
}

//------------------------------------------------------------------------------
// Description:
// Translucent pass with key checking.
// \pre s_exists: s!=0
void vtkDefaultPass::RenderFilteredTranslucentPolygonalGeometry(const vtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  int c = s->GetPropArrayCount();
  int i = 0;
  while (i < c)
  {
    vtkProp* p = s->GetPropArray()[i];
    if (p->HasKeys(s->GetRequiredKeys()))
    {
      int rendered =
        p->RenderFilteredTranslucentPolygonalGeometry(s->GetRenderer(), s->GetRequiredKeys());
      this->NumberOfRenderedProps += rendered;
    }
    ++i;
  }
}

//------------------------------------------------------------------------------
// Description:
// Volume pass without key checking.
// \pre s_exists: s!=0
void vtkDefaultPass::RenderVolumetricGeometry(const vtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  int c = s->GetPropArrayCount();
  int i = 0;
  while (i < c)
  {
    int rendered = s->GetPropArray()[i]->RenderVolumetricGeometry(s->GetRenderer());
    this->NumberOfRenderedProps += rendered;
    ++i;
  }
}

//------------------------------------------------------------------------------
// Description:
// Translucent pass with key checking.
// \pre s_exists: s!=0
void vtkDefaultPass::RenderFilteredVolumetricGeometry(const vtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  int c = s->GetPropArrayCount();
  int i = 0;
  while (i < c)
  {
    vtkProp* p = s->GetPropArray()[i];
    if (p->HasKeys(s->GetRequiredKeys()))
    {
      int rendered = p->RenderFilteredVolumetricGeometry(s->GetRenderer(), s->GetRequiredKeys());
      this->NumberOfRenderedProps += rendered;
    }
    ++i;
  }
}

//------------------------------------------------------------------------------
// Description:
// Overlay pass without key checking.
// \pre s_exists: s!=0
void vtkDefaultPass::RenderOverlay(const vtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  int c = s->GetPropArrayCount();
  int i = 0;
  while (i < c)
  {
    int rendered = s->GetPropArray()[i]->RenderOverlay(s->GetRenderer());
    this->NumberOfRenderedProps += rendered;
    ++i;
  }
}

//------------------------------------------------------------------------------
// Description:
// Overlay pass with key checking.
// \pre s_exists: s!=0
void vtkDefaultPass::RenderFilteredOverlay(const vtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  int c = s->GetPropArrayCount();
  int i = 0;
  while (i < c)
  {
    vtkProp* p = s->GetPropArray()[i];
    if (p->HasKeys(s->GetRequiredKeys()))
    {
      int rendered = p->RenderFilteredOverlay(s->GetRenderer(), s->GetRequiredKeys());
      this->NumberOfRenderedProps += rendered;
    }
    ++i;
  }
}
VTK_ABI_NAMESPACE_END
