// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWindowNode.h"

#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkRendererNode.h"
#include "vtkUnsignedCharArray.h"

//============================================================================
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkWindowNode);

//------------------------------------------------------------------------------
vtkWindowNode::vtkWindowNode()
{
  this->Size[0] = 0;
  this->Size[1] = 0;
  this->ColorBuffer = vtkUnsignedCharArray::New();
  this->ZBuffer = vtkFloatArray::New();
}

//------------------------------------------------------------------------------
vtkWindowNode::~vtkWindowNode()
{
  this->ColorBuffer->Delete();
  this->ColorBuffer = nullptr;
  this->ZBuffer->Delete();
  this->ZBuffer = nullptr;
}

//------------------------------------------------------------------------------
void vtkWindowNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkWindowNode::Build(bool prepass)
{
  if (prepass)
  {
    vtkRenderWindow* mine = vtkRenderWindow::SafeDownCast(this->GetRenderable());
    if (!mine)
    {
      return;
    }

    this->PrepareNodes();
    this->AddMissingNodes(mine->GetRenderers());
    this->RemoveUnusedNodes();
  }
}

//------------------------------------------------------------------------------
void vtkWindowNode::Synchronize(bool prepass)
{
  if (prepass)
  {
    vtkRenderWindow* mine = vtkRenderWindow::SafeDownCast(this->GetRenderable());
    if (!mine)
    {
      return;
    }
    const int* sz = mine->GetSize();
    this->Size[0] = sz[0];
    this->Size[1] = sz[1];
    auto const& renderers = this->GetChildren();
    for (auto ren : renderers)
    {
      vtkRendererNode* child = vtkRendererNode::SafeDownCast(ren);
      child->SetSize(this->Size);
    }
  }
}
VTK_ABI_NAMESPACE_END
