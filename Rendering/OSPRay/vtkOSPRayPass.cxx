/*=========================================================================

  Prograxq:   Visualization Toolkit
  Module:    vtkOSPRayPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCamera.h"
#include "vtkCameraPass.h"
#include "vtkLightsPass.h"
#include "vtkObjectFactory.h"
#include "vtkOSPRayPass.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkOSPRayViewNodeFactory.h"
#include "vtkOverlayPass.h"
#include "vtkOpaquePass.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderState.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSequencePass.h"
#include "vtkSequencePass.h"
#include "vtkVolumetricPass.h"

#include "ospray/ospray.h"

#include <sstream>
#include <stdexcept>

class vtkOSPRayPassInternals : public vtkRenderPass
{
public:
  static vtkOSPRayPassInternals *New();
  vtkTypeMacro(vtkOSPRayPassInternals,vtkRenderPass);
  vtkOSPRayPassInternals()
  {
    this->Factory = 0;
  }
  ~vtkOSPRayPassInternals()
  {
    this->Factory->Delete();
  }
  void Render(const vtkRenderState *s) override
  {
    this->Parent->RenderInternal(s);
  }

  vtkOSPRayViewNodeFactory *Factory;
  vtkOSPRayPass *Parent;
};

int vtkOSPRayPass::OSPDeviceRefCount = 0;

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOSPRayPassInternals);

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOSPRayPass);

// ----------------------------------------------------------------------------
vtkOSPRayPass::vtkOSPRayPass()
{
  this->SceneGraph = nullptr;

  vtkOSPRayPass::OSPInit();

  vtkOSPRayViewNodeFactory *vnf = vtkOSPRayViewNodeFactory::New();
  this->Internal = vtkOSPRayPassInternals::New();
  this->Internal->Factory = vnf;
  this->Internal->Parent = this;

  this->CameraPass = vtkCameraPass::New();
  this->LightsPass = vtkLightsPass::New();
  this->SequencePass = vtkSequencePass::New();
  this->VolumetricPass = vtkVolumetricPass::New();
  this->OverlayPass = vtkOverlayPass::New();

  this->RenderPassCollection = vtkRenderPassCollection::New();
  this->RenderPassCollection->AddItem(this->LightsPass);
  this->RenderPassCollection->AddItem(this->Internal);
  this->RenderPassCollection->AddItem(this->OverlayPass);

  this->SequencePass->SetPasses(this->RenderPassCollection);
  this->CameraPass->SetDelegatePass(this->SequencePass);
}

// ----------------------------------------------------------------------------
vtkOSPRayPass::~vtkOSPRayPass()
{
  this->SetSceneGraph(nullptr);
  this->Internal->Delete();
  this->Internal = 0;
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

  vtkOSPRayPass::OSPShutdown();
}

// ----------------------------------------------------------------------------
void vtkOSPRayPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

// ----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkOSPRayPass, SceneGraph, vtkOSPRayRendererNode)

// ----------------------------------------------------------------------------
void vtkOSPRayPass::Render(const vtkRenderState *s)
{
  if (!this->SceneGraph)
  {
    vtkRenderer *ren = s->GetRenderer();
    if (ren)
    {
      this->SceneGraph = vtkOSPRayRendererNode::SafeDownCast
        (this->Internal->Factory->CreateNode(ren));
    }
  }
  this->CameraPass->Render(s);
}

// ----------------------------------------------------------------------------
void vtkOSPRayPass::RenderInternal(const vtkRenderState *s)
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
    int right = 0;
    if (rwin)
    {
      if (rwin->GetStereoRender() == 1)
      {
        if (rwin->GetStereoType() == VTK_STEREO_CRYSTAL_EYES)
        {
          vtkCamera *camera = ren->GetActiveCamera();
          if (camera)
          {
            if (!camera->GetLeftEye())
            {
              right = 1;
            }
          }
        }
      }
    }
    ren->GetTiledSizeAndOrigin(&viewportWidth,&viewportHeight,
                                &viewportX,&viewportY);
    vtkOSPRayRendererNode* oren= vtkOSPRayRendererNode::SafeDownCast
      (this->SceneGraph->GetViewNodeFor(ren));
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
        0, vtkOSPRayRendererNode::GetCompositeOnGL(ren), right);
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
         0, right);
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
         0, vtkOSPRayRendererNode::GetCompositeOnGL(ren), right);
      delete[] ontoZ;
      delete[] ontoRGBA;
    }
  }
}

// ----------------------------------------------------------------------------
void vtkOSPRayPass::OSPInit()
{
  if (OSPDeviceRefCount == 0)
  {
    ospInit(nullptr, nullptr);
  }
  OSPDeviceRefCount++;
}

// ----------------------------------------------------------------------------
void vtkOSPRayPass::OSPShutdown()
{
  --OSPDeviceRefCount;
  if (OSPDeviceRefCount == 0)
  {
    ospShutdown();
  }
}
