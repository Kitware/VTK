/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOptiXPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCameraPass.h"
#include "vtkLightsPass.h"
#include "vtkObjectFactory.h"
#include "vtkOptiXPass.h"
#include "vtkOptiXRendererNode.h"
#include "vtkOptiXViewNodeFactory.h"
#include "vtkOverlayPass.h"
#include "vtkOpaquePass.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderState.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSequencePass.h"
#include "vtkSequencePass.h"
#include "vtkVolumetricPass.h"

#include <stdexcept>

class vtkOptiXPassInternals : public vtkRenderPass
{
public:
  static vtkOptiXPassInternals *New();
  vtkTypeMacro(vtkOptiXPassInternals,vtkRenderPass);
  vtkOptiXPassInternals()
  {
    this->Factory = 0;
  }
  ~vtkOptiXPassInternals()
  {
    this->Factory->Delete();
  }
  void Render(const vtkRenderState *s)
  {
    this->Parent->RenderInternal(s);
  }

  vtkOptiXViewNodeFactory *Factory;
  vtkOptiXPass *Parent;
};

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOptiXPassInternals);

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOptiXPass);

//------------------------------------------------------------------------------
vtkOptiXPass::vtkOptiXPass()
{
  this->SceneGraph = nullptr;

  vtkOptiXViewNodeFactory *vnf = vtkOptiXViewNodeFactory::New();
  this->Internals = vtkOptiXPassInternals::New();
  this->Internals->Factory = vnf;
  this->Internals->Parent = this;


  this->CameraPass = vtkCameraPass::New();
  this->LightsPass = vtkLightsPass::New();
  this->SequencePass = vtkSequencePass::New();
  this->VolumetricPass = vtkVolumetricPass::New();
  this->OverlayPass = vtkOverlayPass::New();

  this->RenderPassCollection = vtkRenderPassCollection::New();
  this->RenderPassCollection->AddItem(this->LightsPass);
  this->RenderPassCollection->AddItem(this->Internals);
  //  this->RenderPassCollection->AddItem(vtkOpaquePass::New());
  this->RenderPassCollection->AddItem(this->VolumetricPass);
  this->RenderPassCollection->AddItem(this->OverlayPass);

  this->SequencePass->SetPasses(this->RenderPassCollection);
  this->CameraPass->SetDelegatePass(this->SequencePass);
}

//------------------------------------------------------------------------------
vtkOptiXPass::~vtkOptiXPass()
{
  this->SetSceneGraph(nullptr);
  this->Internals->Delete();
  this->Internals = 0;
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
  if (this->SequencePass)
  {
    this->SequencePass->Delete();
    this->SequencePass = 0;
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
  if (this->RenderPassCollection)
  {
    this->RenderPassCollection->Delete();
    this->RenderPassCollection = 0;
  }
}

//------------------------------------------------------------------------------
void vtkOptiXPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//------------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkOptiXPass, SceneGraph, vtkOptiXRendererNode)

//------------------------------------------------------------------------------
void vtkOptiXPass::Render(const vtkRenderState *s)
{
  if (!this->SceneGraph)
  {
    vtkRenderer *ren = s->GetRenderer();
    if (ren)
    {
      this->SceneGraph = vtkOptiXRendererNode::SafeDownCast
        (this->Internals->Factory->CreateNode(ren));
    }
  }
  this->CameraPass->Render(s);
}

//------------------------------------------------------------------------------
void vtkOptiXPass::RenderInternal(const vtkRenderState *s)
{
  this->NumberOfRenderedProps=0;

  if (this->SceneGraph)
  {
    this->SceneGraph->TraverseAllPasses();

    // copy the result to the window
    vtkRenderer *ren = s->GetRenderer();
    vtkRenderWindow *rwin =
      vtkRenderWindow::SafeDownCast(ren->GetVTKWindow());
    int viewportX, viewportY;
    int viewportWidth, viewportHeight;
    ren->GetTiledSizeAndOrigin(&viewportWidth,&viewportHeight,
      &viewportX,&viewportY);
    int layer = ren->GetLayer();
    if (layer == 0)
    {
      rwin->SetZbufferData(
        viewportX,  viewportY,
        viewportX+viewportWidth-1,
        viewportY+viewportHeight-1,
        this->SceneGraph->GetZBuffer());
      rwin->SetRGBACharPixelData(
        viewportX,  viewportY,
        viewportX+viewportWidth-1,
        viewportY+viewportHeight-1,
        this->SceneGraph->GetBuffer(),
        0, 0 );
    }
    else
    {
      float *ontoZ = rwin->GetZbufferData
        (viewportX,  viewportY,
         viewportX+viewportWidth-1,
         viewportY+viewportHeight-1);
      unsigned char *ontoRGBA = rwin->GetRGBACharPixelData
        (viewportX,  viewportY,
         viewportX+viewportWidth-1,
         viewportY+viewportHeight-1,
         0);
      vtkOptiXRendererNode* oren= vtkOptiXRendererNode::SafeDownCast
        (this->SceneGraph->GetViewNodeFor(ren));
      oren->WriteLayer(ontoRGBA, ontoZ, viewportWidth, viewportHeight, layer);
      rwin->SetZbufferData(
        viewportX,  viewportY,
        viewportX+viewportWidth-1,
        viewportY+viewportHeight-1,
        ontoZ);
      rwin->SetRGBACharPixelData(
        viewportX,  viewportY,
        viewportX+viewportWidth-1,
        viewportY+viewportHeight-1,
        ontoRGBA,
        0, 0 );
      delete[] ontoZ;
      delete[] ontoRGBA;
    }
  }
}
