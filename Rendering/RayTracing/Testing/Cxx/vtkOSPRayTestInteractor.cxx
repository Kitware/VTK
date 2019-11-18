/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayTestInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOSPRayTestInteractor.h"
#include "vtkObjectFactory.h"

#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkOSPRayLightNode.h"
#include "vtkOSPRayPass.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkOpenGLRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRendererCollection.h"

#include <string>
#include <vector>

namespace
{
static std::vector<std::string> ActorNames;
}

//----------------------------------------------------------------------------
class vtkOSPRayTestLooper : public vtkCommand
{
  // for progressive rendering
public:
  vtkTypeMacro(vtkOSPRayTestLooper, vtkCommand);

  static vtkOSPRayTestLooper* New()
  {
    vtkOSPRayTestLooper* self = new vtkOSPRayTestLooper;
    self->RenderWindow = nullptr;
    self->ProgressiveCount = 0;
    return self;
  }

  void Execute(
    vtkObject* vtkNotUsed(caller), unsigned long eventId, void* vtkNotUsed(callData)) override
  {
    if (eventId == vtkCommand::TimerEvent)
    {
      if (this->RenderWindow)
      {
        vtkRenderer* renderer = this->RenderWindow->GetRenderers()->GetFirstRenderer();
        int maxframes = vtkOSPRayRendererNode::GetMaxFrames(renderer);
        if (this->ProgressiveCount < maxframes)
        {
          this->ProgressiveCount++;
          this->RenderWindow->Render();
        }
      }
    }
    else
    {
      this->ProgressiveCount = 0;
    }
  }
  vtkRenderWindow* RenderWindow;
  int ProgressiveCount;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOSPRayTestInteractor);

//----------------------------------------------------------------------------
vtkOSPRayTestInteractor::vtkOSPRayTestInteractor()
{
  this->SetPipelineControlPoints(nullptr, nullptr, nullptr);
  this->VisibleActor = -1;
  this->VisibleLight = -1;
  this->Looper = vtkOSPRayTestLooper::New();
}

//----------------------------------------------------------------------------
vtkOSPRayTestInteractor::~vtkOSPRayTestInteractor()
{
  this->Looper->Delete();
}

//----------------------------------------------------------------------------
void vtkOSPRayTestInteractor::SetPipelineControlPoints(
  vtkRenderer* g, vtkRenderPass* _O, vtkRenderPass* _G)
{
  this->GLRenderer = g;
  this->O = _O;
  this->G = _G;
}

//----------------------------------------------------------------------------
void vtkOSPRayTestInteractor::OnKeyPress()
{
  if (this->GLRenderer == nullptr)
  {
    return;
  }

  // Get the keypress
  vtkRenderWindowInteractor* rwi = this->Interactor;
  std::string key = rwi->GetKeySym();

  if (key == "c")
  {
    vtkRenderPass* current = this->GLRenderer->GetPass();
    if (current == this->G)
    {
      cerr << "OSPRAY rendering " << this->O << endl;
      this->GLRenderer->SetPass(this->O);
      this->GLRenderer->GetRenderWindow()->Render();
    }
    else if (current == this->O)
    {
      cerr << "GL rendering " << this->G << endl;
      this->GLRenderer->SetPass(this->G);
      this->GLRenderer->GetRenderWindow()->Render();
    }
  }

  if (key == "n")
  {
    vtkActorCollection* actors = this->GLRenderer->GetActors();

    this->VisibleActor++;
    cerr << "VISIBLE " << this->VisibleActor;
    if (this->VisibleActor == actors->GetNumberOfItems())
    {
      this->VisibleActor = -1;
    }
    for (int i = 0; i < actors->GetNumberOfItems(); i++)
    {
      if (this->VisibleActor == -1 || this->VisibleActor == i)
      {
        if (i < static_cast<int>(ActorNames.size()))
        {
          cerr << " : " << ActorNames[i] << " ";
        }
        vtkActor::SafeDownCast(actors->GetItemAsObject(i))->SetVisibility(1);
      }
      else
      {
        vtkActor::SafeDownCast(actors->GetItemAsObject(i))->SetVisibility(0);
      }
    }
    cerr << endl;
    this->GLRenderer->ResetCamera();
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "l")
  {
    vtkLightCollection* lights = this->GLRenderer->GetLights();

    this->VisibleLight++;
    if (this->VisibleLight == lights->GetNumberOfItems())
    {
      this->VisibleLight = -1;
    }
    cerr << "LIGHT " << this->VisibleLight << "/" << lights->GetNumberOfItems() << endl;
    for (int i = 0; i < lights->GetNumberOfItems(); i++)
    {
      if (this->VisibleLight == -1 || this->VisibleLight == i)
      {
        vtkLight::SafeDownCast(lights->GetItemAsObject(i))->SwitchOn();
      }
      else
      {
        vtkLight::SafeDownCast(lights->GetItemAsObject(i))->SwitchOff();
      }
    }
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "P")
  {
    int maxframes = vtkOSPRayRendererNode::GetMaxFrames(this->GLRenderer) + 16;
    if (maxframes > 256)
    {
      maxframes = 256;
    }
    vtkOSPRayRendererNode::SetMaxFrames(maxframes, this->GLRenderer);
    cerr << "frames " << maxframes << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "p")
  {
    int maxframes = vtkOSPRayRendererNode::GetMaxFrames(this->GLRenderer);
    if (maxframes > 1)
    {
      maxframes = maxframes / 2;
    }
    vtkOSPRayRendererNode::SetMaxFrames(maxframes, this->GLRenderer);
    cerr << "frames " << maxframes << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "s")
  {
    bool shadows = !(this->GLRenderer->GetUseShadows() == 0);
    cerr << "shadows now " << (!shadows ? "ON" : "OFF") << endl;
    this->GLRenderer->SetUseShadows(!shadows);
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "t")
  {
    std::string type = vtkOSPRayRendererNode::GetRendererType(this->GLRenderer);
    if (type == std::string("scivis"))
    {
      vtkOSPRayRendererNode::SetRendererType("pathtracer", this->GLRenderer);
    }
    else if (type == std::string("pathtracer"))
    {
      vtkOSPRayRendererNode::SetRendererType("optix pathtracer", this->GLRenderer);
    }
    else if (type == std::string("optix pathtracer"))
    {
      vtkOSPRayRendererNode::SetRendererType("scivis", this->GLRenderer);
    }
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "2")
  {
    int spp = vtkOSPRayRendererNode::GetSamplesPerPixel(this->GLRenderer);
    cerr << "samples now " << spp + 1 << endl;
    vtkOSPRayRendererNode::SetSamplesPerPixel(spp + 1, this->GLRenderer);
    this->GLRenderer->GetRenderWindow()->Render();
  }
  if (key == "1")
  {
    vtkOSPRayRendererNode::SetSamplesPerPixel(1, this->GLRenderer);
    cerr << "samples now " << 1 << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "D")
  {
    int aoSamples = vtkOSPRayRendererNode::GetAmbientSamples(this->GLRenderer) + 2;
    if (aoSamples > 64)
    {
      aoSamples = 64;
    }
    vtkOSPRayRendererNode::SetAmbientSamples(aoSamples, this->GLRenderer);
    cerr << "aoSamples " << aoSamples << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "d")
  {
    int aosamples = vtkOSPRayRendererNode::GetAmbientSamples(this->GLRenderer);
    aosamples = aosamples / 2;
    vtkOSPRayRendererNode::SetAmbientSamples(aosamples, this->GLRenderer);
    cerr << "aoSamples " << aosamples << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "I")
  {
    double intens = vtkOSPRayLightNode::GetLightScale() * 1.5;
    vtkOSPRayLightNode::SetLightScale(intens);
    cerr << "intensity " << intens << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "i")
  {
    double intens = vtkOSPRayLightNode::GetLightScale() / 1.5;
    vtkOSPRayLightNode::SetLightScale(intens);
    cerr << "intensity " << intens << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "N")
  {
    bool set = vtkOSPRayRendererNode::GetEnableDenoiser(this->GLRenderer);
    vtkOSPRayRendererNode::SetEnableDenoiser(!set, this->GLRenderer);
    cerr << "denoiser " << (!set ? "ON" : "OFF") << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  // Forward events
  vtkInteractorStyleTrackballCamera::OnKeyPress();
}

//------------------------------------------------------------------------------
void vtkOSPRayTestInteractor::AddName(const char* name)
{
  ActorNames.push_back(std::string(name));
}

//------------------------------------------------------------------------------
vtkCommand* vtkOSPRayTestInteractor::GetLooper(vtkRenderWindow* rw)
{
  rw->Render();
  vtkOSPRayRendererNode::SetMaxFrames(128, this->GLRenderer);
  vtkOSPRayTestLooper::SafeDownCast(this->Looper)->RenderWindow = rw;
  return this->Looper;
}
