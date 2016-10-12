/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayPass.cxx

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
  void Render(const vtkRenderState *s)
  {
    this->Parent->RenderInternal(s);
  }

  vtkOSPRayViewNodeFactory *Factory;
  vtkOSPRayPass *Parent;
};

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOSPRayPassInternals);

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOSPRayPass);

// ----------------------------------------------------------------------------
vtkOSPRayPass::vtkOSPRayPass()
{
  this->SceneGraph = NULL;

  int ac = 1;
  const char* envArgs = getenv("VTKOSPRAY_ARGS");
  if (envArgs)
  {
    std::stringstream ss(envArgs);
    std::string arg;
    std::vector<std::string> args;
    while (ss >> arg)
    {
      args.push_back(arg);
    }
    int ac =args.size()+1;
    const char** av = new const char*[ac];
    av[0] = "pvOSPRay";
    for(int i=1;i < ac; i++)
    {
      av[i] = args[i - 1].c_str();
    }
    try
    {
      ospInit(&ac, av);
    }
    catch (std::runtime_error &vtkNotUsed(e))
    {
      //todo: request addition of ospFinalize() to ospray
    }
    delete [] av;
  }
  else
  {
    const char* av[] = {"pvOSPRay\0"};
    try
    {
      ospInit(&ac, av);
    }
    catch (std::runtime_error &vtkNotUsed(e))
    {
      //todo: request addition of ospFinalize() to ospray
    }
  }

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
  this->SetSceneGraph(NULL);
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
        0, vtkOSPRayRendererNode::GetCompositeOnGL(ren) );
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
         0, vtkOSPRayRendererNode::GetCompositeOnGL(ren) );
      delete[] ontoZ;
      delete[] ontoRGBA;
    }
  }
}
