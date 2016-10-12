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
#include "vtkOpenGLRenderer.h"
#include "vtkOSPRayLightNode.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkOSPRayPass.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

#include <vector>
#include <string>

namespace {
  static std::vector<std::string> ActorNames;
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOSPRayTestInteractor);

//----------------------------------------------------------------------------
vtkOSPRayTestInteractor::vtkOSPRayTestInteractor()
{
  this->SetPipelineControlPoints(NULL,NULL,NULL);
  this->VisibleActor = -1;
  this->VisibleLight = -1;
}

//----------------------------------------------------------------------------
void vtkOSPRayTestInteractor::SetPipelineControlPoints
  (vtkOpenGLRenderer *g,
   vtkRenderPass *_O,
   vtkRenderPass *_G)
{
  this->GLRenderer = g;
  this->O = _O;
  this->G = _G;
}

//----------------------------------------------------------------------------
void vtkOSPRayTestInteractor::OnKeyPress()
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
    int maxframes = vtkOSPRayRendererNode::GetMaxFrames(this->GLRenderer) + 4;
    if (maxframes>64)
    {
      maxframes=64;
    }
    vtkOSPRayRendererNode::SetMaxFrames(maxframes, this->GLRenderer);
    cerr << "frames " << maxframes << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if(key == "p")
  {
    int maxframes = vtkOSPRayRendererNode::GetMaxFrames(this->GLRenderer);
    if (maxframes>1)
    {
      maxframes=maxframes/2;
    }
    vtkOSPRayRendererNode::SetMaxFrames(maxframes, this->GLRenderer);
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

  if(key == "t")
  {
    std::string type = vtkOSPRayRendererNode::GetRendererType(this->GLRenderer);
    if (type == std::string("scivis"))
    {
      vtkOSPRayRendererNode::SetRendererType("pathtracer", this->GLRenderer);
    }
    else
    {
      vtkOSPRayRendererNode::SetRendererType("scivis", this->GLRenderer);
    }
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if(key == "2")
  {
    int spp = vtkOSPRayRendererNode::GetSamplesPerPixel(this->GLRenderer);
    cerr << "samples now " << spp+1 << endl;
    vtkOSPRayRendererNode::SetSamplesPerPixel(spp+1, this->GLRenderer);
    this->GLRenderer->GetRenderWindow()->Render();
  }
  if(key == "1")
  {
    vtkOSPRayRendererNode::SetSamplesPerPixel(1, this->GLRenderer);
    cerr << "samples now " << 1 << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if(key == "D")
  {
    int aoSamples = vtkOSPRayRendererNode::GetAmbientSamples(this->GLRenderer) + 2;
    if (aoSamples>64)
    {
      aoSamples=64;
    }
    vtkOSPRayRendererNode::SetAmbientSamples(aoSamples, this->GLRenderer);
    cerr << "aoSamples " << aoSamples << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if(key == "d")
  {
    int aosamples = vtkOSPRayRendererNode::GetAmbientSamples(this->GLRenderer);
    aosamples=aosamples/2;
    vtkOSPRayRendererNode::SetAmbientSamples(aosamples, this->GLRenderer);
    cerr << "aoSamples " << aosamples << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if(key == "I")
  {
    double intens = vtkOSPRayLightNode::GetLightScale()*1.5;
    vtkOSPRayLightNode::SetLightScale(intens);
    cerr << "intensity " << intens << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  if(key == "i")
  {
    double intens = vtkOSPRayLightNode::GetLightScale()/1.5;
    vtkOSPRayLightNode::SetLightScale(intens);
    cerr << "intensity " << intens << endl;
    this->GLRenderer->GetRenderWindow()->Render();
  }

  // Forward events
  vtkInteractorStyleTrackballCamera::OnKeyPress();
}

//------------------------------------------------------------------------------
void vtkOSPRayTestInteractor::AddName(const char *name)
{
  ActorNames.push_back(std::string(name));
}
