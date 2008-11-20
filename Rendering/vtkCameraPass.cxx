/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCameraPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCameraPass.h"
#include "vtkObjectFactory.h"
#include <assert.h>
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkgl.h"

vtkCxxRevisionMacro(vtkCameraPass, "1.1");
vtkStandardNewMacro(vtkCameraPass);
vtkCxxSetObjectMacro(vtkCameraPass,DelegatePass,vtkRenderPass);

// ----------------------------------------------------------------------------
vtkCameraPass::vtkCameraPass()
{
  this->DelegatePass=0;
}

// ----------------------------------------------------------------------------
vtkCameraPass::~vtkCameraPass()
{
  if(this->DelegatePass!=0)
    {
      this->DelegatePass->Delete();
    }
}

// ----------------------------------------------------------------------------
void vtkCameraPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "DelegatePass:";
  if(this->DelegatePass!=0)
    {
    this->DelegatePass->PrintSelf(os,indent);
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
void vtkCameraPass::Render(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  this->NumberOfRenderedProps=0;

  this->UpdateCamera(s->GetRenderer());
  
  if(this->DelegatePass!=0)
    {
    this->DelegatePass->Render(s);
    this->NumberOfRenderedProps+=
      this->DelegatePass->GetNumberOfRenderedProps();
    }
  else
    {
    vtkWarningMacro(<<" no delegate.");
    }
  
  // clean up the model view matrix set up by the camera 
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkCameraPass::ReleaseGraphicsResources(vtkWindow *w)
{
  assert("pre: w_exists" && w!=0);
  if(this->DelegatePass!=0)
    {
    this->DelegatePass->ReleaseGraphicsResources(w);
    }
}
