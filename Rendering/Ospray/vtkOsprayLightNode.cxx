/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOsprayLightNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOsprayLightNode.h"

#include "vtkCollectionIterator.h"
#include "vtkLight.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOsprayRendererNode.h"

#include "ospray/ospray.h"
#include <vector>


//============================================================================
double vtkOsprayLightNode::LightScale = 1.0;

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOsprayLightNode);

//----------------------------------------------------------------------------
vtkOsprayLightNode::vtkOsprayLightNode()
{
}

//----------------------------------------------------------------------------
vtkOsprayLightNode::~vtkOsprayLightNode()
{
}

//----------------------------------------------------------------------------
void vtkOsprayLightNode::SetLightScale(double s)
{
  vtkOsprayLightNode::LightScale = s;
}

//----------------------------------------------------------------------------
double vtkOsprayLightNode::GetLightScale()
{
  return vtkOsprayLightNode::LightScale;
}

//----------------------------------------------------------------------------
void vtkOsprayLightNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOsprayLightNode::Render(bool prepass)
{
  if (prepass)
    {
    vtkOsprayRendererNode *orn =
      static_cast<vtkOsprayRendererNode *>(
        this->GetFirstAncestorOfType("vtkOsprayRendererNode"));
    OSPRenderer oRenderer = orn->GetORenderer();

    vtkLight *light = vtkLight::SafeDownCast(this->GetRenderable());

    float color[3] = {0.0,0.0,0.0};
    if (light->GetSwitch())
      {
      color[0] = static_cast<float>(light->GetDiffuseColor()[0]);
      color[1] = static_cast<float>(light->GetDiffuseColor()[1]);
      color[2] = static_cast<float>(light->GetDiffuseColor()[2]);
      }
    if (light->GetPositional())
      {
      OSPLight ospLight = ospNewLight(oRenderer, "PointLight");
      ospSet3f(ospLight, "color",
               color[0],
               color[1],
               color[2]);
      float fI = static_cast<float>(vtkOsprayLightNode::LightScale*light->GetIntensity()*0.2);//TODO: why so bright?
      ospSet1f(ospLight, "intensity", fI);
      ospSet3f(ospLight, "position",
               light->GetPosition()[0],
               light->GetPosition()[1],
               light->GetPosition()[2]);
      ospCommit(ospLight);
      orn->AddLight(ospLight);
      }
    else
      {
      double direction[3];
      direction[0] = light->GetPosition()[0] - light->GetFocalPoint()[0];
      direction[1] = light->GetPosition()[1] - light->GetFocalPoint()[1];
      direction[2] = light->GetPosition()[2] - light->GetFocalPoint()[2];
      OSPLight ospLight = ospNewLight(oRenderer, "DirectionalLight");
      ospSet3f(ospLight, "color",
               color[0],
               color[1],
               color[2]);
      ospSet1f(ospLight, "intensity", vtkOsprayLightNode::LightScale*light->GetIntensity());
      vtkMath::Normalize(direction);
      ospSet3f(ospLight, "direction",
               -direction[0],-direction[1],-direction[2]);
      ospCommit(ospLight);
      orn->AddLight(ospLight);
      }
    }
}
