/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOptiXCameraNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOptiXCameraNode.h"

#include "vtkCamera.h"
#include "vtkCollectionIterator.h"
#include "vtkObjectFactory.h"
#include "vtkOptiXRendererNode.h"
#include "vtkRenderer.h"
#include "vtkViewNodeCollection.h"
#include "vtkOptiXPtxLoader.h"

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_namespace.h>

//============================================================================

//============================================================================
vtkStandardNewMacro(vtkOptiXCameraNode);

//------------------------------------------------------------------------------
vtkOptiXCameraNode::vtkOptiXCameraNode()
{
}

//------------------------------------------------------------------------------
vtkOptiXCameraNode::~vtkOptiXCameraNode()
{
}

//------------------------------------------------------------------------------
void vtkOptiXCameraNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOptiXCameraNode::Render(bool prepass)
{
  if (prepass)
  {
    vtkOptiXRendererNode *orn =
      static_cast<vtkOptiXRendererNode *>(
        this->GetFirstAncestorOfType("vtkOptiXRendererNode"));

    optix::Context context = orn->GetOptiXContext();
    if( !context->getRayGenerationProgram(0) )
    {
      context->setRayGenerationProgram( 0, orn->GetOptiXPtxLoader()->RayGenProgram);
    }

    vtkRenderer* ren = vtkRenderer::SafeDownCast(orn->GetRenderable());
    int tiledSize[2];
    int tiledOrigin[2];
    ren->GetTiledSizeAndOrigin(
      &tiledSize[0], &tiledSize[1],
      &tiledOrigin[0], &tiledOrigin[1]);

    vtkCamera* cam = static_cast<vtkCamera *>(this->Renderable);
    const float  fovy   = cam->GetViewAngle();
    const float  aspect =
      static_cast<float>( tiledSize[0] ) /
      static_cast<float>( tiledSize[1] );

    const optix::float3 pos = optix::make_float3(
      cam->GetPosition()[0],
      cam->GetPosition()[1],
      cam->GetPosition()[2]
      );

    const optix::float3 dir = optix::make_float3(
      cam->GetDirectionOfProjection()[0],
      cam->GetDirectionOfProjection()[1],
      cam->GetDirectionOfProjection()[2]
      );

    const optix::float3 up = optix::make_float3(
      cam->GetViewUp()[0],
      cam->GetViewUp()[1],
      cam->GetViewUp()[2]
      );

    const float vlen = tanf( 0.5f * fovy * M_PIf / 180.0f );
    const float ulen = vlen * aspect;
    const optix::float3 cam_W = optix::normalize( dir );
    const optix::float3 cam_U = optix::normalize( optix::cross( dir, up ) );
    const optix::float3 cam_V = optix::normalize( optix::cross( cam_U, cam_W ) );

    optix::Program rayGenProgram = orn->GetOptiXPtxLoader()->RayGenProgram;
    rayGenProgram[ "pos" ]->setFloat( pos );
    rayGenProgram[ "U"   ]->setFloat( cam_U*ulen );
    rayGenProgram[ "V"   ]->setFloat( cam_V*vlen );
    rayGenProgram[ "W"   ]->setFloat( cam_W );
  }
}
