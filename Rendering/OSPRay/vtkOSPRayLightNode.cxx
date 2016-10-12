/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayLightNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOSPRayLightNode.h"

#include "vtkCollectionIterator.h"
#include "vtkLight.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOSPRayRendererNode.h"

#include "ospray/ospray.h"
#include <vector>


//============================================================================
double vtkOSPRayLightNode::LightScale = 1.0;

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOSPRayLightNode);

//----------------------------------------------------------------------------
vtkOSPRayLightNode::vtkOSPRayLightNode()
{
}

//----------------------------------------------------------------------------
vtkOSPRayLightNode::~vtkOSPRayLightNode()
{
}

//----------------------------------------------------------------------------
void vtkOSPRayLightNode::SetLightScale(double s)
{
  vtkOSPRayLightNode::LightScale = s;
}

//----------------------------------------------------------------------------
double vtkOSPRayLightNode::GetLightScale()
{
  return vtkOSPRayLightNode::LightScale;
}

//----------------------------------------------------------------------------
void vtkOSPRayLightNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOSPRayLightNode::Render(bool prepass)
{
  if (prepass)
  {
    vtkOSPRayRendererNode *orn =
      static_cast<vtkOSPRayRendererNode *>(
        this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));
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
      double px, py, pz;
      light->GetTransformedPosition(px, py, pz);
      OSPLight ospLight = ospNewLight(oRenderer, "PointLight");
      ospSet3f(ospLight, "color", color[0], color[1], color[2]);
      float fI = static_cast<float>
        (vtkOSPRayLightNode::LightScale*
         light->GetIntensity()*
         vtkMath::Pi() //since OSP 0.10.0
         );
      ospSet1f(ospLight, "intensity", fI);
      ospSet3f(ospLight, "position", px, py, pz);
      ospCommit(ospLight);
      orn->AddLight(ospLight);
    }
    else
    {
      double px, py, pz;
      double fx, fy, fz;
      light->GetTransformedPosition(px, py, pz);
      light->GetTransformedFocalPoint(fx, fy, fz);
      double direction[3];
      direction[0] = fx - px;
      direction[1] = fy - py;
      direction[2] = fz - pz;
      OSPLight ospLight = ospNewLight(oRenderer, "DirectionalLight");
      ospSet3f(ospLight, "color", color[0], color[1], color[2]);
      float fI = static_cast<float>
        (vtkOSPRayLightNode::LightScale*
         light->GetIntensity()*
         vtkMath::Pi()); //since OSP 0.10.0
      ospSet1f(ospLight, "intensity", fI);
      vtkMath::Normalize(direction);
      ospSet3f(ospLight, "direction",
               direction[0], direction[1], direction[2]);
      ospCommit(ospLight);
      orn->AddLight(ospLight);
    }
  }
}
