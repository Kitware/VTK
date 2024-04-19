// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSequencePass.h"
#include "vtkObjectFactory.h"
#include "vtkRenderPassCollection.h"
#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSequencePass);
vtkCxxSetObjectMacro(vtkSequencePass, Passes, vtkRenderPassCollection);

//------------------------------------------------------------------------------
vtkSequencePass::vtkSequencePass()
{
  this->Passes = nullptr;
}

//------------------------------------------------------------------------------
vtkSequencePass::~vtkSequencePass()
{
  if (this->Passes)
  {
    this->Passes->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkSequencePass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Passes:";
  if (this->Passes != nullptr)
  {
    this->Passes->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
}

//------------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkSequencePass::Render(const vtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  this->NumberOfRenderedProps = 0;
  if (this->Passes)
  {
    this->Passes->InitTraversal();
    vtkRenderPass* p = this->Passes->GetNextRenderPass();
    while (p)
    {
      p->Render(s);
      this->NumberOfRenderedProps += p->GetNumberOfRenderedProps();
      p = this->Passes->GetNextRenderPass();
    }
  }
}

//------------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkSequencePass::ReleaseGraphicsResources(vtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);

  if (this->Passes)
  {
    this->Passes->InitTraversal();
    vtkRenderPass* p = this->Passes->GetNextRenderPass();
    while (p)
    {
      p->ReleaseGraphicsResources(w);
      p = this->Passes->GetNextRenderPass();
    }
  }
}
VTK_ABI_NAMESPACE_END
