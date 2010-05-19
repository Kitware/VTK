/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSequencePass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSequencePass.h"
#include "vtkObjectFactory.h"
#include <assert.h>
#include "vtkRenderPassCollection.h"

vtkStandardNewMacro(vtkSequencePass);
vtkCxxSetObjectMacro(vtkSequencePass,Passes,vtkRenderPassCollection);

// ----------------------------------------------------------------------------
vtkSequencePass::vtkSequencePass()
{
  this->Passes=0;
}

// ----------------------------------------------------------------------------
vtkSequencePass::~vtkSequencePass()
{
  if(this->Passes!=0)
    {
      this->Passes->Delete();
    }
}

// ----------------------------------------------------------------------------
void vtkSequencePass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Passes:";
  if(this->Passes!=0)
    {
    this->Passes->PrintSelf(os,indent);
    }
  else
    {
    os << "(none)" <<endl;
    }
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkSequencePass::Render(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  this->NumberOfRenderedProps=0;
  if(this->Passes!=0)
    {
      this->Passes->InitTraversal();
      vtkRenderPass *p=this->Passes->GetNextRenderPass();
      while(p!=0)
        {
          p->Render(s);
          this->NumberOfRenderedProps+=p->GetNumberOfRenderedProps();
          p=this->Passes->GetNextRenderPass();
        }
    }
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkSequencePass::ReleaseGraphicsResources(vtkWindow *w)
{
  assert("pre: w_exists" && w!=0);
  
  if(this->Passes!=0)
    {
    this->Passes->InitTraversal();
    vtkRenderPass *p=this->Passes->GetNextRenderPass();
    while(p!=0)
      {
      p->ReleaseGraphicsResources(w);
      p=this->Passes->GetNextRenderPass();
      }
    }
}
