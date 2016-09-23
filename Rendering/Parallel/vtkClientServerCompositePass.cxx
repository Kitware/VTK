/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClientServerCompositePass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkClientServerCompositePass.h"

#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"
#include "vtkSynchronizedRenderers.h"
#include "vtkRenderState.h"

vtkStandardNewMacro(vtkClientServerCompositePass);
vtkCxxSetObjectMacro(vtkClientServerCompositePass, Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkClientServerCompositePass, RenderPass, vtkRenderPass);
vtkCxxSetObjectMacro(vtkClientServerCompositePass, PostProcessingRenderPass, vtkRenderPass);
//----------------------------------------------------------------------------
vtkClientServerCompositePass::vtkClientServerCompositePass()
{
  this->Controller = 0;
  this->RenderPass = 0;
  this->PostProcessingRenderPass = 0;
  this->ServerSideRendering = true;
  this->ProcessIsServer = false;
}

//----------------------------------------------------------------------------
vtkClientServerCompositePass::~vtkClientServerCompositePass()
{
  this->SetController(0);
  this->SetRenderPass(0);
  this->SetPostProcessingRenderPass(0);
}

//----------------------------------------------------------------------------
void vtkClientServerCompositePass::ReleaseGraphicsResources(vtkWindow *w)
{
  this->Superclass::ReleaseGraphicsResources(w);
  if (this->RenderPass)
  {
    this->RenderPass->ReleaseGraphicsResources(w);
  }
  if (this->PostProcessingRenderPass)
  {
    this->PostProcessingRenderPass->ReleaseGraphicsResources(w);
  }
}

//----------------------------------------------------------------------------
void vtkClientServerCompositePass::Render(const vtkRenderState *s)
{
  if (!this->ServerSideRendering  || this->ProcessIsServer)
  {
    if (this->RenderPass)
    {
      this->RenderPass->Render(s);
    }
    else
    {
      vtkWarningMacro("No render pass set.");
    }
  }

  if (this->ServerSideRendering)
  {
    if (!this->Controller)
    {
      vtkErrorMacro("Cannot do remote rendering with a controller.");
    }
    else if (this->ProcessIsServer)
    {
      // server.
      vtkSynchronizedRenderers::vtkRawImage rawImage;
      rawImage.Capture(s->GetRenderer());
      int header[4];
      header[0] = rawImage.IsValid()? 1 : 0;
      header[1] = rawImage.GetWidth();
      header[2] = rawImage.GetHeight();
      header[3] = rawImage.IsValid()?
        rawImage.GetRawPtr()->GetNumberOfComponents() : 0;
      // send the image to the client.
      this->Controller->Send(header, 4, 1, 0x023430);
      if (rawImage.IsValid())
      {
        this->Controller->Send(rawImage.GetRawPtr(), 1, 0x023430);
      }
    }
    else
    {
      // client.
      vtkSynchronizedRenderers::vtkRawImage rawImage;
      int header[4];
      this->Controller->Receive(header, 4, 1, 0x023430);
      if (header[0] > 0)
      {
        rawImage.Resize(header[1], header[2], header[3]);
        this->Controller->Receive(rawImage.GetRawPtr(), 1, 0x023430);
        rawImage.MarkValid();
      }
      rawImage.PushToViewport(s->GetRenderer());
    }
  }

  if (this->PostProcessingRenderPass)
  {
    this->PostProcessingRenderPass->Render(s);
  }
}

//----------------------------------------------------------------------------
void vtkClientServerCompositePass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: ";
  if(this->Controller==0)
  {
    os << "(none)" << endl;
  }
  else
  {
    os << this->Controller << endl;
  }

  os << indent << "ServerSideRendering: " << this->ServerSideRendering << endl;
  os << indent << "ProcessIsServer: " << this->ProcessIsServer << endl;

  os << indent << "RenderPass: ";
  if(this->RenderPass==0)
  {
    os << "(none)" << endl;
  }
  else
  {
    os << this->RenderPass << endl;
  }
  os << indent << "PostProcessingRenderPass: ";
  if(this->PostProcessingRenderPass==0)
  {
    os << "(none)" << endl;
  }
  else
  {
    os << this->PostProcessingRenderPass << endl;
  }

}

