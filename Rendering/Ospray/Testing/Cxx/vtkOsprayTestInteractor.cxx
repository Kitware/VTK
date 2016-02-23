/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOsprayTestInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOsprayTestInteractor.h"
#include "vtkObjectFactory.h"

#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOsprayLightNode.h"
#include "vtkOsprayRendererNode.h"
#include "vtkOsprayPass.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

#include <vector>
#include <string>

namespace {
  static std::vector<std::string> ActorNames;
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOsprayTestInteractor);

//----------------------------------------------------------------------------
vtkOsprayTestInteractor::vtkOsprayTestInteractor()
{
  this->SetPipelineControlPoints(NULL,NULL,NULL);
  this->VisibleActor = -1;
  this->VisibleLight = -1;
}

//----------------------------------------------------------------------------
void vtkOsprayTestInteractor::SetPipelineControlPoints
  (vtkOpenGLRenderer *g,
   vtkRenderPass *_O,
   vtkRenderPass *_G)
{
  this->GLRenderer = g;
  this->O = _O;
  this->G = _G;
}

//----------------------------------------------------------------------------
void vtkOsprayTestInteractor::OnKeyPress()
{
  if (this->GLRenderer == NULL)
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
    int maxframes = vtkOsprayRendererNode::GetMaxFrames(this->GLRenderer) + 4;
    if (maxframes>64)
      {
      maxframes=64;
      }
    vtkOsprayRendererNode::SetMaxFrames(maxframes, this->GLRenderer);
    cerr << "frames " << maxframes << endl;
    this->GLRenderer->GetRenderWindow()->Render();
    }

  if(key == "p")
    {
    int maxframes = vtkOsprayRendererNode::GetMaxFrames(this->GLRenderer);
    if (maxframes>1)
      {
      maxframes=maxframes/2;
      }
    vtkOsprayRendererNode::SetMaxFrames(maxframes, this->GLRenderer);
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
    int spp = vtkOsprayRendererNode::GetSamplesPerPixel(this->GLRenderer);
    cerr << "samples now " << spp+1 << endl;
    vtkOsprayRendererNode::SetSamplesPerPixel(spp+1, this->GLRenderer);
    this->GLRenderer->GetRenderWindow()->Render();
    }
  if(key == "1")
    {
    vtkOsprayRendererNode::SetSamplesPerPixel(1, this->GLRenderer);
    cerr << "samples now " << 1 << endl;
    this->GLRenderer->GetRenderWindow()->Render();
    }

  if(key == "D")
    {
    int aoSamples = vtkOsprayRendererNode::GetAmbientSamples(this->GLRenderer) + 2;
    if (aoSamples>64)
      {
      aoSamples=64;
      }
    vtkOsprayRendererNode::SetAmbientSamples(aoSamples, this->GLRenderer);
    cerr << "aoSamples " << aoSamples << endl;
    this->GLRenderer->GetRenderWindow()->Render();
    }

  if(key == "d")
    {
    int aosamples = vtkOsprayRendererNode::GetAmbientSamples(this->GLRenderer);
    aosamples=aosamples/2;
    vtkOsprayRendererNode::SetAmbientSamples(aosamples, this->GLRenderer);
    cerr << "aoSamples " << aosamples << endl;
    this->GLRenderer->GetRenderWindow()->Render();
    }

  if(key == "I")
    {
    double intens = vtkOsprayLightNode::GetLightScale()*1.5;
    vtkOsprayLightNode::SetLightScale(intens);
    cerr << "intensity " << intens << endl;
    this->GLRenderer->GetRenderWindow()->Render();
    }

  if(key == "i")
    {
    double intens = vtkOsprayLightNode::GetLightScale()/1.5;
    vtkOsprayLightNode::SetLightScale(intens);
    cerr << "intensity " << intens << endl;
    this->GLRenderer->GetRenderWindow()->Render();
    }

  // Forward events
  vtkInteractorStyleTrackballCamera::OnKeyPress();
}

//------------------------------------------------------------------------------
void vtkOsprayTestInteractor::AddName(const char *name)
{
  ActorNames.push_back(std::string(name));
}
