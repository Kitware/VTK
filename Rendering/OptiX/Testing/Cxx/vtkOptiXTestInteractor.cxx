/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOptiXTestInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOptiXTestInteractor.h"
#include "vtkObjectFactory.h"

#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOptiXLightNode.h"
#include "vtkOptiXRendererNode.h"
#include "vtkOptiXPass.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

#include <vector>
#include <string>

namespace
{
  static std::vector<std::string> ActorNames;
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkOptiXTestInteractor);

//------------------------------------------------------------------------------
vtkOptiXTestInteractor::vtkOptiXTestInteractor()
{
  this->SetPipelineControlPoints(nullptr,nullptr,nullptr);
  this->VisibleActor = -1;
  this->VisibleLight = -1;
}

//------------------------------------------------------------------------------
void vtkOptiXTestInteractor::SetPipelineControlPoints
  (vtkOpenGLRenderer *g,
   vtkRenderPass *_O,
   vtkRenderPass *_G)
{
  this->GLRenderer = g;
  this->O = _O;
  this->G = _G;
}

//------------------------------------------------------------------------------
void vtkOptiXTestInteractor::OnKeyPress()
{
  if (this->GLRenderer == nullptr)
  {
    return;
  }

  // Get the keypress
  vtkRenderWindowInteractor *rwi = this->Interactor;
  std::string key = rwi->GetKeySym();

  if(key == "c")
  {
    vtkRenderPass * current = this->GLRenderer->GetPass();
    if (current == this->G)
    {
      cerr << "OptiX rendering " << this->O << endl;
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

  if(key == "n")
  {
    vtkActorCollection * actors = this->GLRenderer->GetActors();

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
        if (i < ActorNames.size())
        {
          cerr << " : " << ActorNames[i] << " ";
        }
        vtkActor::SafeDownCast(actors->GetItemAsObject(i))->
          SetVisibility(1);
      }
      else
      {
        vtkActor::SafeDownCast(actors->GetItemAsObject(i))->
              SetVisibility(0);
      }
    }
    cerr << endl;
    this->GLRenderer->ResetCamera();
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if(key == "l")
  {
    vtkLightCollection * lights = this->GLRenderer->GetLights();

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
        vtkLight::SafeDownCast(lights->GetItemAsObject(i))->
          SwitchOn();
      }
      else
      {
        vtkLight::SafeDownCast(lights->GetItemAsObject(i))->
              SwitchOff();
      }
    }
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if(key == "P")
  {
    int maxframes = vtkOptiXRendererNode::GetMaxFrames(this->GLRenderer) + 4;
    if (maxframes>64)
    {
      maxframes=64;
    }
    vtkOptiXRendererNode::SetMaxFrames(maxframes, this->GLRenderer);
    cerr << "frames " << maxframes << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if(key == "p")
  {
    int maxframes = vtkOptiXRendererNode::GetMaxFrames(this->GLRenderer);
    if (maxframes>1)
    {
      maxframes=maxframes/2;
    }
    vtkOptiXRendererNode::SetMaxFrames(maxframes, this->GLRenderer);
    cerr << "frames " << maxframes << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if(key == "s")
  {
    bool shadows = !(this->GLRenderer->GetUseShadows()==0);
    cerr << "shadows now " << (!shadows?"ON":"OFF") << endl;
    this->GLRenderer->SetUseShadows(!shadows);
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if(key == "2")
  {
    int spp = vtkOptiXRendererNode::GetSamplesPerPixel(this->GLRenderer);
    cerr << "samples now " << spp+1 << endl;
    vtkOptiXRendererNode::SetSamplesPerPixel(spp+1, this->GLRenderer);
    this->GLRenderer->GetRenderWindow()->Render();
  }
  if(key == "1")
  {
    vtkOptiXRendererNode::SetSamplesPerPixel(1, this->GLRenderer);
    cerr << "samples now " << 1 << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if(key == "D")
  {
    int aoSamples = vtkOptiXRendererNode::GetAmbientSamples(this->GLRenderer) + 2;
    if (aoSamples>64)
    {
      aoSamples=64;
    }
    vtkOptiXRendererNode::SetAmbientSamples(aoSamples, this->GLRenderer);
    cerr << "aoSamples " << aoSamples << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if(key == "d")
  {
    int aosamples = vtkOptiXRendererNode::GetAmbientSamples(this->GLRenderer);
    aosamples=aosamples/2;
    vtkOptiXRendererNode::SetAmbientSamples(aosamples, this->GLRenderer);
    cerr << "aoSamples " << aosamples << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if(key == "I")
  {
    double intens = vtkOptiXLightNode::GetLightScale()*1.5;
    vtkOptiXLightNode::SetLightScale(intens);
    cerr << "intensity " << intens << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if(key == "i")
  {
    double intens = vtkOptiXLightNode::GetLightScale()/1.5;
    vtkOptiXLightNode::SetLightScale(intens);
    cerr << "intensity " << intens << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  // Forward events
  vtkInteractorStyleTrackballCamera::OnKeyPress();
}

//------------------------------------------------------------------------------
void vtkOptiXTestInteractor::AddName(const char *name)
{
  ActorNames.push_back(std::string(name));
}
