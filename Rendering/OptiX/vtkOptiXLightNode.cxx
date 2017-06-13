/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOptiXLightNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOptiXLightNode.h"

#include "vtkCollectionIterator.h"
#include "vtkLight.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOptiXRendererNode.h"

#include <optixu/optixpp_namespace.h>
#include <CUDA/Light.h>

#include <vector>

//============================================================================
double vtkOptiXLightNode::LightScale = 1.0;

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkOptiXLightNode);

//------------------------------------------------------------------------------
vtkOptiXLightNode::vtkOptiXLightNode()
{
}

//------------------------------------------------------------------------------
vtkOptiXLightNode::~vtkOptiXLightNode()
{
}

//------------------------------------------------------------------------------
void vtkOptiXLightNode::SetLightScale(double s)
{
  vtkOptiXLightNode::LightScale = s;
}

//------------------------------------------------------------------------------
double vtkOptiXLightNode::GetLightScale()
{
  return vtkOptiXLightNode::LightScale;
}

//------------------------------------------------------------------------------
void vtkOptiXLightNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOptiXLightNode::Render(bool prepass)
{
  if (prepass)
  {
    vtkOptiXRendererNode *orn =
      static_cast<vtkOptiXRendererNode *>(
        this->GetFirstAncestorOfType("vtkOptiXRendererNode"));

    vtkLight *vlight = vtkLight::SafeDownCast(this->GetRenderable());

    if( vlight->GetSwitch() == 0 ||
      vtkOptiXLightNode::LightScale <= 0.0f ||
      vlight->GetIntensity() <= 0.0 )
    {
      // Ignoring light
      return;
    }

    const optix::float3 color = optix::make_float3(
      static_cast<float>( vlight->GetDiffuseColor()[0] ),
      static_cast<float>( vlight->GetDiffuseColor()[1] ),
      static_cast<float>( vlight->GetDiffuseColor()[2] )
      );

    const float intensity = static_cast<float>(
      vtkOptiXLightNode::LightScale *
      vlight->GetIntensity()
      );

    vtkopt::Light light;
    light.color = color*intensity;
    light.pos   = optix::make_float3( 0.0f );
    light.dir   = optix::make_float3( 0.0f );

    if (vlight->GetPositional())
    {
      double px, py, pz;
      vlight->GetTransformedPosition(px, py, pz);

      light.type = vtkopt::Light::POSITIONAL;
      light.pos  = optix::make_float3(
        static_cast<float>(px),
        static_cast<float>(py),
        static_cast<float>(pz)
        );
    }
    else
    {
      double px, py, pz;
      vlight->GetTransformedPosition(px, py, pz);

      double fx, fy, fz;
      vlight->GetTransformedFocalPoint(fx, fy, fz);

      light.type = vtkopt::Light::DIRECTIONAL;
      light.dir  = optix::make_float3(
        static_cast<float>( fx - px ),
        static_cast<float>( fy - py ),
        static_cast<float>( fz - pz )
        );
      light.dir = optix::normalize( light.dir );
    }
    orn->AddLight( light );
  }
}
