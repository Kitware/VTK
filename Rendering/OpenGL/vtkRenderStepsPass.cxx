/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderStepsPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRenderStepsPass.h"
#include "vtkObjectFactory.h"
#include <cassert>

#include "vtkCameraPass.h"
#include "vtkLightsPass.h"
#include "vtkOpaquePass.h"
#include "vtkOverlayPass.h"
#include "vtkRenderPassCollection.h"
#include "vtkSequencePass.h"
#include "vtkTranslucentPass.h"
#include "vtkVolumetricPass.h"


vtkStandardNewMacro(vtkRenderStepsPass);

vtkCxxSetObjectMacro(vtkRenderStepsPass,CameraPass,vtkCameraPass);
vtkCxxSetObjectMacro(vtkRenderStepsPass,LightsPass,vtkRenderPass);
vtkCxxSetObjectMacro(vtkRenderStepsPass,OpaquePass,vtkRenderPass);
vtkCxxSetObjectMacro(vtkRenderStepsPass,TranslucentPass,vtkRenderPass);
vtkCxxSetObjectMacro(vtkRenderStepsPass,VolumetricPass,vtkRenderPass);
vtkCxxSetObjectMacro(vtkRenderStepsPass,OverlayPass,vtkRenderPass);
vtkCxxSetObjectMacro(vtkRenderStepsPass,PostProcessPass,vtkRenderPass);

// ----------------------------------------------------------------------------
vtkRenderStepsPass::vtkRenderStepsPass()
{
  this->CameraPass = vtkCameraPass::New();
  this->LightsPass = vtkLightsPass::New();
  this->OpaquePass = vtkOpaquePass::New();
  this->TranslucentPass = vtkTranslucentPass::New();
  this->VolumetricPass = vtkVolumetricPass::New();
  this->OverlayPass = vtkOverlayPass::New();
  this->SequencePass = vtkSequencePass::New();
  vtkRenderPassCollection *rpc = vtkRenderPassCollection::New();
  this->SequencePass->SetPasses(rpc);
  rpc->Delete();
  this->CameraPass->SetDelegatePass(this->SequencePass);
  this->PostProcessPass = NULL;
}

// ----------------------------------------------------------------------------
vtkRenderStepsPass::~vtkRenderStepsPass()
{
  if (this->CameraPass)
  {
    this->CameraPass->Delete();
    this->CameraPass = 0;
  }
  if (this->LightsPass)
  {
    this->LightsPass->Delete();
    this->LightsPass = 0;
  }
  if (this->OpaquePass)
  {
    this->OpaquePass->Delete();
    this->OpaquePass = 0;
  }
  if (this->TranslucentPass)
  {
    this->TranslucentPass->Delete();
    this->TranslucentPass = 0;
  }
  if (this->VolumetricPass)
  {
    this->VolumetricPass->Delete();
    this->VolumetricPass = 0;
  }
  if (this->OverlayPass)
  {
    this->OverlayPass->Delete();
    this->OverlayPass = 0;
  }
  if (this->PostProcessPass)
  {
    this->PostProcessPass->Delete();
    this->PostProcessPass = 0;
  }
  if (this->SequencePass)
  {
    this->SequencePass->Delete();
    this->SequencePass = 0;
  }
}

// ----------------------------------------------------------------------------
void vtkRenderStepsPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CameraPass:";
  if (this->CameraPass != 0)
  {
    this->CameraPass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" <<endl;
  }
  os << indent << "LightsPass:";
  if (this->LightsPass != 0)
  {
    this->LightsPass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" <<endl;
  }
  os << indent << "opaquePass:";
  if (this->OpaquePass != 0)
  {
    this->OpaquePass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" <<endl;
  }
  os << indent << "TranslucentPass:";
  if (this->TranslucentPass != 0)
  {
    this->TranslucentPass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" <<endl;
  }
  os << indent << "VolumetricPass:";
  if (this->VolumetricPass != 0)
  {
    this->VolumetricPass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" <<endl;
  }
  os << indent << "OverlayPass:";
  if (this->OverlayPass != 0)
  {
    this->OverlayPass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" <<endl;
  }
  os << indent << "PostProcessPass:";
  if (this->PostProcessPass != 0)
  {
    this->PostProcessPass->PrintSelf(os, indent);
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
void vtkRenderStepsPass::Render(const vtkRenderState *s)
{
  assert("pre: s_exists" && s != 0);

  vtkRenderPassCollection *passes =
    this->SequencePass->GetPasses();
  passes->RemoveAllItems();

  if (this->LightsPass)
  {
    passes->AddItem(this->LightsPass);
  }
  if (this->OpaquePass)
  {
    passes->AddItem(this->OpaquePass);
  }
  if (this->TranslucentPass)
  {
    passes->AddItem(this->TranslucentPass);
  }
  if (this->VolumetricPass)
  {
    passes->AddItem(this->VolumetricPass);
  }
  if (this->OverlayPass)
  {
    passes->AddItem(this->OverlayPass);
  }

  this->NumberOfRenderedProps = 0;
  if (this->CameraPass)
  {
    this->CameraPass->Render(s);
    this->NumberOfRenderedProps += this->CameraPass->GetNumberOfRenderedProps();
  }
  if (this->PostProcessPass)
  {
    this->PostProcessPass->Render(s);
    this->NumberOfRenderedProps += this->PostProcessPass->GetNumberOfRenderedProps();
  }
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkRenderStepsPass::ReleaseGraphicsResources(vtkWindow *w)
{
  assert("pre: w_exists" && w != 0);

  if (this->CameraPass)
  {
    this->CameraPass->ReleaseGraphicsResources(w);
  }
  if (this->LightsPass)
  {
    this->LightsPass->ReleaseGraphicsResources(w);
  }
  if (this->OpaquePass)
  {
    this->OpaquePass->ReleaseGraphicsResources(w);
  }
  if (this->TranslucentPass)
  {
    this->TranslucentPass->ReleaseGraphicsResources(w);
  }
  if (this->VolumetricPass)
  {
    this->VolumetricPass->ReleaseGraphicsResources(w);
  }
  if (this->OverlayPass)
  {
    this->OverlayPass->ReleaseGraphicsResources(w);
  }
  if (this->PostProcessPass)
  {
    this->PostProcessPass->ReleaseGraphicsResources(w);
  }
}
