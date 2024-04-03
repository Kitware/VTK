// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAnariTestInteractor.h"
#include "vtkObjectFactory.h"

#include "vtkAnariLightNode.h"
#include "vtkAnariPass.h"
#include "vtkAnariRendererNode.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
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
class vtkAnariTestLooper : public vtkCommand
{
  // for progressive rendering
public:
  vtkTypeMacro(vtkAnariTestLooper, vtkCommand);

  static vtkAnariTestLooper* New()
  {
    vtkAnariTestLooper* self = new vtkAnariTestLooper;
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
        this->RenderWindow->Render();
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
vtkStandardNewMacro(vtkAnariTestInteractor);

//----------------------------------------------------------------------------
vtkAnariTestInteractor::vtkAnariTestInteractor()
{
  this->SetPipelineControlPoints(nullptr, nullptr, nullptr);
  this->VisibleActor = -1;
  this->VisibleLight = -1;
  this->Looper = vtkAnariTestLooper::New();
}

//----------------------------------------------------------------------------
vtkAnariTestInteractor::~vtkAnariTestInteractor()
{
  this->Looper->Delete();
}

//----------------------------------------------------------------------------
void vtkAnariTestInteractor::SetPipelineControlPoints(
  vtkRenderer* g, vtkRenderPass* _O, vtkRenderPass* _G)
{
  this->GLRenderer = g;
  this->O = _O;
  this->G = _G;
}

//----------------------------------------------------------------------------
void vtkAnariTestInteractor::OnKeyPress()
{
  if (this->GLRenderer == nullptr)
  {
    return;
  }

  // Get the keypress
  vtkRenderWindowInteractor* rwi = this->Interactor;
  char* ckey = rwi->GetKeySym();
  std::string key = ckey != nullptr ? ckey : "";

  if (key == "c")
  {
    vtkRenderPass* current = this->GLRenderer->GetPass();

    if (current == this->G)
    {
      cerr << "ANARI rendering " << this->O << endl;
      this->GLRenderer->SetPass(this->O);
      vtkAnariRendererNode::SetLibraryName("environment", this->GLRenderer);
      vtkAnariRendererNode::SetSamplesPerPixel(4, this->GLRenderer);
      vtkAnariRendererNode::SetLightFalloff(.5, this->GLRenderer);
      vtkAnariRendererNode::SetUseDenoiser(1, this->GLRenderer);
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

  if (key == "2")
  {
    int spp = vtkAnariRendererNode::GetSamplesPerPixel(this->GLRenderer);
    cerr << "samples now " << spp + 1 << endl;
    vtkAnariRendererNode::SetSamplesPerPixel(spp + 1, this->GLRenderer);
    this->GLRenderer->GetRenderWindow()->Render();
  }
  if (key == "1")
  {
    vtkAnariRendererNode::SetSamplesPerPixel(1, this->GLRenderer);
    cerr << "samples now " << 1 << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "D")
  {
    int aoSamples = vtkAnariRendererNode::GetAmbientSamples(this->GLRenderer) + 2;
    if (aoSamples > 64)
    {
      aoSamples = 64;
    }
    vtkAnariRendererNode::SetAmbientSamples(aoSamples, this->GLRenderer);
    cerr << "aoSamples " << aoSamples << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "d")
  {
    int aosamples = vtkAnariRendererNode::GetAmbientSamples(this->GLRenderer);
    aosamples = aosamples / 2;
    vtkAnariRendererNode::SetAmbientSamples(aosamples, this->GLRenderer);
    cerr << "aoSamples " << aosamples << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if (key == "I" || key == "i")
  {
    vtkLightCollection* lights = this->GLRenderer->GetLights();

    for (int i = 0; i < lights->GetNumberOfItems(); i++)
    {
      vtkLight* light = vtkLight::SafeDownCast(lights->GetItemAsObject(i));
      double intens = vtkAnariLightNode::GetLightScale(light) * 1.5;
      vtkAnariLightNode::SetLightScale(intens, light);
      cerr << "intensity " << intens << endl;
    }

    this->GLRenderer->GetRenderWindow()->Render();
  }

  // Forward events
  vtkInteractorStyleTrackballCamera::OnKeyPress();
}

//------------------------------------------------------------------------------
void vtkAnariTestInteractor::AddName(const char* name)
{
  ActorNames.push_back(std::string(name));
}

//------------------------------------------------------------------------------
vtkCommand* vtkAnariTestInteractor::GetLooper(vtkRenderWindow* rw)
{
  rw->Render();
  vtkAnariTestLooper::SafeDownCast(this->Looper)->RenderWindow = rw;
  return this->Looper;
}
